#include "webserver.h"

#define TIMESLOT 5

static int pipefd[2];
static sort_timer_lst timer_lst;
static int epollfd;

int setnonblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void sig_handler(int sig) {
    int save_errno = errno;
    int msg = sig;
    send(pipefd[1], (char*)&msg, 1, 0);
    errno = save_errno;
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

void cb_func(client_data* user_data) {
    assert(user_data);
    epoll_ctl(epollfd, EPOLL_CTL_DEL, user_data->sockfd, 0);
    close(user_data->sockfd);
    printf("close fd %d\n", user_data->sockfd);
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

    // for quick debugging
    int reuse = 1;
    setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    ret = bind(m_listenfd, (struct sockaddr*)&address, sizeof(address));
    assert(ret >= 0);

    ret = listen(m_listenfd, 5);
    assert(ret >= 0);

    epoller = new Epoller();

    epollfd = epoller->epoll_fd;

    epoller->addfd(m_listenfd, false);

    http_conn::epoller = epoller;

    printf("webserver init at localhost:%d\n", m_port);

    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, pipefd);
    assert(ret != -1);

    setnonblocking(pipefd[1]);
    // not 100% correct because should not have RDHUP!!!
    epoller->addfd(pipefd[0], false);

    addsig(SIGALRM, sig_handler);
    addsig(SIGTERM, sig_handler);

    users = new client_data[MAX_FD];

    alarm(TIMESLOT);

    Log::get_instance()->init("serverlog", 8192, 1000000, 10000);
}

void WebServer::eventLoop() {
    while (!stop_server) {
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

                util_timer* timer = users[sockfd].timer;
                if (timer) {
                    timer_lst.del_timer(timer);
                }

            } else if ((sockfd == pipefd[0]) && (events & EPOLLIN)) {
                char signals[16];
                int ret = recv(pipefd[0], signals, sizeof(signals), 0);
                if (ret == -1) {
                    continue;
                } else if (ret == 0) {
                    continue;
                } else {
                    for (int i = 0; i < ret; ++i) {
                        switch (signals[i]) {
                            case SIGALRM: {
                                timeout = true;
                                break;
                            }
                            case SIGTERM: {
                                stop_server = true;
                            }
                        }
                    }
                }
            } else if (events & EPOLLIN) {
                if (m_users[sockfd].read()) {
                    util_timer* timer = users[sockfd].timer;
                    if (timer) {
                        time_t cur = time(NULL);
                        timer->expire = cur + 3 * TIMESLOT;
                        timer_lst.adjust_timer(timer);
                        // printf("adjust timer once after read\n");
                    }

                    m_pool->append(m_users + sockfd);
                } else {
                    closeConn(sockfd);

                    // don't need this
                    // timer->cb_func(&users_timer[sockfd]);
                    util_timer* timer = users[sockfd].timer;
                    if (timer) {
                        timer_lst.del_timer(timer);
                    }
                }

            } else if (events & EPOLLOUT) {
                util_timer* timer = users[sockfd].timer;
                if (!m_users[sockfd].write()) {
                    closeConn(sockfd);

                    // timer->cb_func(&users_timer[sockfd]);
                    if (timer) {
                        timer_lst.del_timer(timer);
                    }

                } else {
                    if (timer) {
                        time_t cur = time(NULL);
                        timer->expire = cur + 3 * TIMESLOT;
                        timer_lst.adjust_timer(timer);
                        // printf("adjust timer once after write\n");
                    }
                }

            } else {
                // LOG Unexpected event;
            }
        }

        if (timeout) {
            timer_lst.tick();
            alarm(TIMESLOT);
            timeout = false;
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

        users[connfd].address = client_address;
        users[connfd].sockfd = connfd;
        util_timer* timer = new util_timer;
        timer->user_data = &users[connfd];
        timer->cb_func = cb_func;
        time_t cur = time(NULL);
        timer->expire = cur + 3 * TIMESLOT;
        users[connfd].timer = timer;
        timer_lst.add_timer(timer);
    }
}

void WebServer::addClient(int fd, sockaddr_in addr) {
    m_users[fd].init(fd, addr);
    epoller->addfd(fd, true);
    LOG_INFO("%s: %d", "new client", fd);
}

void WebServer::closeConn(int fd) {
    m_users[fd].close_conn();
    epoller->removefd(fd);
    LOG_INFO("%s: %d", "close client", fd);
}

WebServer::~WebServer() {
    delete[] m_users;
    delete m_pool;
    delete epoller;

    close(pipefd[1]);
    close(pipefd[0]);

    close(m_listenfd);
}
