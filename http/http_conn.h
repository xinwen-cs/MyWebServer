#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <atomic>

#include "../buffer/buffer.h"
#include "../webserver/epoller.h"
#include "http_request.h"
#include "http_response.h"

class http_conn {
public:
    http_conn() {}
    ~http_conn() {}

public:
    void init(int sockfd, const sockaddr_in& addr);
    // wtf why use real_close?
    void close_conn(bool real_close = true);

    bool read();
    bool write();

    void process();

    // FIXME
    // util_timer* timer;

    static Epoller* epoller;
    static std::atomic<int> m_user_count;

private:
    void init();
    void prepare_writev();

    int m_sockfd;
    sockaddr_in m_address;

    struct iovec m_iv[2];
    int m_iv_count;

    // FIXME
    int bytes_to_send;
    int bytes_have_send;

    Buffer m_read_buf;
    Buffer m_write_buf;

    http_request request;
    http_response response;
};

#endif
