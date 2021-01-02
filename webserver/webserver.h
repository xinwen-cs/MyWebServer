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
#include "../timer/lst_timer.h"
#include "epoller.h"
#include "../log/log.h"

#define MAX_FD 65536

class WebServer {
public:
    WebServer(Config& config);
    ~WebServer();

    void eventLoop();

    void dealListen();

    void addClient(int fd, sockaddr_in addr);
    void closeConn(int fd);

private:
    int m_port;

    http_conn* m_users;
    client_data* users;
    threadpool<http_conn>* m_pool;

    int m_listenfd;

    Epoller* epoller;

    bool stop_server = false;
    bool timeout = false;
};

#endif
