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

WebServer::WebServer(Config& config) : m_pool(new threadpool<http_conn>(config.thread_num)), timer_(new HeapTimer) {
    timeoutMS_ = 12000;

    m_port = config.port;

    addsig(SIGPIPE, SIG_IGN);
    // addsig(SIGALRM, sig_handler);
    // addsig(SIGTERM, sig_handler);

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

    epoller->addfd(m_listenfd, false);

    // FIXME
    http_conn::epoller = epoller;

    printf("webserver init at localhost:%d\n", m_port);

    Log::get_instance()->init("serverlog", 8192, 1000000, 10000);
}

void WebServer::eventLoop() {
    int timeMS = -1;
    while (!stop_server) {
        timeMS = timer_->GetNextTick();

        int number = epoller->wait(timeMS);

        if ((number < 0) && (errno != EINTR)) {
            printf("epoll failure\n");
            break;
        }

        for (int i = 0; i < number; i++) {
            int sockfd = epoller->getEventsFd(i);
            uint32_t events = epoller->getEvents(i);

            if (sockfd == m_listenfd) {
                dealListen();
            } else if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                closeConn(sockfd);
            } else if (events & EPOLLIN) {
                if (m_users[sockfd].read()) {
                    extentTimer(sockfd);
                    m_pool->append(&m_users[sockfd]);
                } else {
                    closeConn(sockfd);
                }
            } else if (events & EPOLLOUT) {
                if (!m_users[sockfd].write()) {
                    closeConn(sockfd);
                } else {
                    extentTimer(sockfd);
                }
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
    LOG_INFO("%s: %d", "new client", fd);

    if (timeoutMS_ > 0) {
        timer_->add(fd, timeoutMS_, std::bind(&WebServer::closeConn, this, fd));
    }
}

void WebServer::extentTimer(int fd) {
    if (timeoutMS_ > 0) {
        timer_->adjust(fd, timeoutMS_);
    }
}

void WebServer::closeConn(int fd) {
    m_users[fd].close_conn();
    epoller->removefd(fd);
    LOG_INFO("%s: %d", "close client", fd);
}

WebServer::~WebServer() {
    delete epoller;
    close(m_listenfd);
}
