#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <unordered_map>
#include <memory>

#include "../config/config.h"
#include "../http/http_conn.h"
#include "../threadpool/threadpool.h"
#include "../timer/lst_timer.h"
#include "epoller.h"
#include "../log/log.h"

#define TIMESLOT 5

class WebServer {
public:
    WebServer(Config& config);
    ~WebServer();

    void eventLoop();

    void dealListen();

    void addClient(int fd, sockaddr_in addr);
    void closeConn(int fd);

    void extentTimer(int fd);
private:
    static const int MAX_FD = 65536;

    int m_port;

    std::unordered_map<int, http_conn> m_users;

    std::unique_ptr<threadpool<http_conn>> m_pool;

    int m_listenfd;

    Epoller* epoller;

    bool stop_server = false;

    bool timeout = false;
    std::unique_ptr<sort_timer_lst> timer_lst;
};

#endif
