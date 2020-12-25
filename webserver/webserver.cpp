#include "webserver.h"

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

    epoller = new Epoller();

    epoller->addfd(m_listenfd, false);

    // FIXME
    http_conn::m_epollfd = epoller->epoll_fd;

    printf("webserver init at localhost:%d\n", m_port);
}

void WebServer::eventLoop() {
    while (true) {
        int number = epoller->wait();

        if ((number < 0) && (errno != EINTR)) {
            printf("epoll failure\n");
            break;
        }

        for (int i = 0; i < number; i++) {
            int sockfd = epoller->getEventFd(i);
            uint32_t events = epoller->getEvents(i);

            if (sockfd == m_listenfd) {
                dealListen();
            } else if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                closeConn(sockfd);

            } else if (events & EPOLLIN) {
                if (m_users[sockfd].read()) {
                    m_pool->append(m_users + sockfd);
                } else {
                    closeConn(sockfd);
                }

            } else if (events & EPOLLOUT) {
                if (!m_users[sockfd].write()) {
                    closeConn(sockfd);
                }

            } else {
                // LOG Unexpected event;
            }
        }
    }
}

void WebServer::dealListen() {
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
            // show_error(connfd, "Internal server busy");
            // LOG_ERR
            break;
        }

        addClient(connfd, client_address);
    }
}

void WebServer::addClient(int fd, sockaddr_in addr) {
    m_users[fd].init(fd, addr);
    epoller->addfd(fd, true);
    // LOG_INFO
}

void WebServer::closeConn(int fd) {
    m_users[fd].close_conn();
    epoller->removefd(fd);
    // LOG_INFO
}

WebServer::~WebServer() {
    delete[] m_users;
    delete m_pool;
    delete epoller;

    close(m_listenfd);
}
