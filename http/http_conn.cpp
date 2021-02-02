#include "http_conn.h"

std::atomic<int> http_conn::m_user_count;
Epoller* http_conn::epoller = NULL;

void http_conn::init(int sockfd, const sockaddr_in& addr) {
    m_sockfd = sockfd;
    m_address = addr;

    int error = 0;
    socklen_t len = sizeof(error);
    getsockopt(m_sockfd, SOL_SOCKET, SO_ERROR, &error, &len);

    // reuse address to avoid TIME_WAIT (for debug)
    int reuse = 1;
    setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    int on = 1;
    setsockopt(m_sockfd, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on));

    ++m_user_count;

    init();
}

// used by multiple functions
void http_conn::init() {
    m_read_idx = 0;
    m_write_idx = 0;

    memset(m_read_buf, '\0', READ_BUFFER_SIZE);
    memset(m_write_buf, '\0', WRITE_BUFFER_SIZE);

    // reset m_checked_idx and check_state
    request.init();
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

    while (1) {
        temp = writev(m_sockfd, m_iv, m_iv_count);

        if (temp <= -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                epoller->modfd(m_sockfd, EPOLLOUT);
                return true;
            }
            response.unmap();
            return false;
        }

        bytes_to_send -= temp;
        bytes_have_send += temp;

        if (static_cast<unsigned>(bytes_have_send) >= m_iv[0].iov_len) {
            m_iv[0].iov_len = 0;
            m_iv[1].iov_base = response.getFileAddr() + (bytes_have_send - m_write_idx);
            m_iv[1].iov_len = bytes_to_send;
        } else {
            m_iv[0].iov_base = m_write_buf + bytes_have_send;
            m_iv[0].iov_len = m_iv[0].iov_len - bytes_have_send;
        }

        if (bytes_to_send <= 0) {
            response.unmap();
            if (request.getKeepAlive()) {
                epoller->modfd(m_sockfd, EPOLLIN);
                init();
                return true;
            } else {
                return false;
            }
        }
    }
}

void http_conn::process() {
    int ret = request.process_read(m_read_buf, m_read_idx);

    if (ret == 0) {  // NO
        epoller->modfd(m_sockfd, EPOLLIN);
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
    epoller->modfd(m_sockfd, EPOLLOUT);
}

void http_conn::prepare_writev() {
    m_iv[0].iov_base = m_write_buf;
    m_iv[0].iov_len = m_write_idx;
    m_iv_count = 1;

    bytes_have_send = 0;
    bytes_to_send = m_write_idx;

    if (response.getFileAddr() && response.getFileLen() > 0) {
        m_iv[1].iov_base = response.getFileAddr();
        m_iv[1].iov_len = response.getFileLen();
        m_iv_count = 2;
        bytes_to_send = m_write_idx + response.getFileLen();
    }
}
