#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../config/config.h"
#include "../http/http_conn.h"
#include "../threadpool/threadpool.h"

#define MAX_FD 65536
#define MAX_EVENT_NUMBER 10000

class WebServer {
public:
    WebServer(Config& config);
    ~WebServer();

    void eventLoop();

private:
    int m_port;

    http_conn* m_users;
    threadpool<http_conn>* m_pool;

    int m_listenfd;
    int m_epollfd;

    epoll_event events[MAX_EVENT_NUMBER];
};

#endif
