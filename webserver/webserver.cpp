#include "webserver.h"

int setnonblocking_t(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void addfd_t(int epollfd, int fd, bool one_shot) {
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    if (one_shot) {
        event.events |= EPOLLONESHOT;
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking_t(fd);
}

void addsig(int sig, void(handler)(int), bool restart = true) {
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    if (restart) {
        sa.sa_flags |= SA_RESTART;
    }
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

void show_error(int connfd, const char* info) {
    printf("%s", info);
    send(connfd, info, strlen(info), 0);
    close(connfd);
}

WebServer::WebServer(Config& config) {
    m_port = config.port;

    addsig(SIGPIPE, SIG_IGN);

    try {
        m_pool = new threadpool<http_conn>;
    } catch (...) {
        printf("thread pool init failed\n");
        return;
    }

    m_users = new http_conn[MAX_FD];
    assert(m_users);

    m_listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(m_listenfd >= 0);
    // struct linger tmp = {1, 0};
    // setsockopt(m_listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));

    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(m_port);

    ret = bind(m_listenfd, (struct sockaddr*)&address, sizeof(address));
    assert(ret >= 0);

    ret = listen(m_listenfd, 5);
    assert(ret >= 0);

    m_epollfd = epoll_create(5);
    assert(m_epollfd != -1);
    addfd_t(m_epollfd, m_listenfd, false);
    http_conn::m_epollfd = m_epollfd;

    printf("webserver init at localhost:%d\n", m_port);
}

void WebServer::eventLoop() {
    while (true) {
        int number = epoll_wait(m_epollfd, events, MAX_EVENT_NUMBER, -1);

        if ((number < 0) && (errno != EINTR)) {
            printf("epoll failure\n");
            break;
        }

        for (int i = 0; i < number; i++) {
            int sockfd = events[i].data.fd;
            if (sockfd == m_listenfd) {
                struct sockaddr_in client_address;
                socklen_t client_addrlength = sizeof(client_address);

                while (true) {
                    int connfd = accept(m_listenfd, (struct sockaddr*)&client_address, &client_addrlength);
                    if (connfd < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            break;
                        }
                        printf("errno is: %d\n", errno);
                        exit(1);
                    }

                    if (http_conn::m_user_count >= MAX_FD) {
                        show_error(connfd, "Internal server busy");
                        continue;
                    }

                    m_users[connfd].init(connfd, client_address);
                }
            } else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                m_users[sockfd].close_conn();
            } else if (events[i].events & EPOLLIN) {
                if (m_users[sockfd].read()) {
                    m_pool->append(m_users + sockfd);
                } else {
                    m_users[sockfd].close_conn();
                }
            } else if (events[i].events & EPOLLOUT) {
                if (!m_users[sockfd].write()) {
                    m_users[sockfd].close_conn();
                }
            } else {
            }
        }
    }
}

WebServer::~WebServer() {
    delete[] m_users;
    delete m_pool;

    close(m_epollfd);
    close(m_listenfd);
}
