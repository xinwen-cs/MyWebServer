#include "http_conn.h"

void modfd(int epollfd, int fd, int ev) {
    epoll_event event;
    event.data.fd = fd;
    event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

int http_conn::m_user_count = 0;
int http_conn::m_epollfd = -1;

void http_conn::init(int sockfd, const sockaddr_in& addr) {
    m_sockfd = sockfd;
    m_address = addr;

    int error = 0;
    socklen_t len = sizeof(error);
    getsockopt(m_sockfd, SOL_SOCKET, SO_ERROR, &error, &len);

    // reuse address to avoid TIME_WAIT (for debug)
    int reuse = 1;
    setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    ++m_user_count;

    init();
}

// used by multiple functions
void http_conn::init() {
    m_read_idx = 0;
    m_write_idx = 0;

    memset(m_read_buf, '\0', READ_BUFFER_SIZE);
    memset(m_write_buf, '\0', WRITE_BUFFER_SIZE);
}

void http_conn::close_conn(bool real_close) {
    if (real_close && (m_sockfd != -1)) {
        close(m_sockfd);
        m_sockfd = -1;
        m_user_count--;
    }
}

bool http_conn::read() {
    if (m_read_idx >= READ_BUFFER_SIZE) {
        return false;
    }

    int bytes_read = 0;
    while (true) {
        bytes_read = recv(m_sockfd, m_read_buf + m_read_idx, READ_BUFFER_SIZE - m_read_idx, 0);
        if (bytes_read == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            return false;
        } else if (bytes_read == 0) {
            return false;
        }

        m_read_idx += bytes_read;
    }

    return true;
}

bool http_conn::write() {
    int temp = 0;
    int bytes_have_send = 0;
    int bytes_to_send = m_write_idx;

    // if (bytes_to_send == 0) {
    //     modfd(m_epollfd, m_sockfd, EPOLLIN);
    //     init();
    //     return true;
    // }

    while (1) {
        temp = writev(m_sockfd, m_iv, m_iv_count);
        if (temp <= -1) {
            if (errno == EAGAIN) {
                modfd(m_epollfd, m_sockfd, EPOLLOUT);
                return true;
            }
            response.unmap();
            return false;
        }

        bytes_to_send -= temp;
        bytes_have_send += temp;

        if (bytes_to_send <= bytes_have_send) {
            response.unmap();
            if (request.getKeepAlive()) {
                // init();
                // return true;
            } else {
                return false;
            }
        }

    }
}

void http_conn::process() {
    int ret = request.process_read(m_read_buf, m_read_idx);

    // FIXME
    ret = 1;

    if (ret == 0) {  // NO
        // continue read
        modfd(m_epollfd, m_sockfd, EPOLLIN);
        return;
    } else if (ret == 1) {  // GET
        response.init(request.getPath(), request.getKeepAlive(), 200);
    } else if (ret == 2) {  // BAD
        response.init(request.getPath(), request.getKeepAlive(), 400);
    }

    bool write_ret = response.process_write(m_write_buf, &m_write_idx);
    // false when write_buffer overflow
    if (!write_ret) {
        close_conn();
    }

    prepare_writev();

    // tell epoll_wait, i want to write
    modfd(m_epollfd, m_sockfd, EPOLLOUT);
}

void http_conn::prepare_writev() {
    m_iv[0].iov_base = m_write_buf;
    m_iv[0].iov_len = m_write_idx;
    m_iv_count = 1;

    if (response.getFileAddr() && response.getFileLen() > 0) {
        m_iv[1].iov_base = response.getFileAddr();
        m_iv[1].iov_len = response.getFileLen();
        m_iv_count = 2;
    }
}
