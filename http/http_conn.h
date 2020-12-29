#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
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

private:
    void init();
    void prepare_writev();

public:
    // should not be here it is ugly
    static int m_epollfd;
    static int m_user_count;

    static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 1024;

private:
    int m_sockfd;
    sockaddr_in m_address;

    struct iovec m_iv[2];
    int m_iv_count;

    int m_read_idx;
    int m_write_idx;
    char m_read_buf[READ_BUFFER_SIZE];
    char m_write_buf[WRITE_BUFFER_SIZE];

    http_request request;
    http_response response;
};

#endif
