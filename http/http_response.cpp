#include "http_response.h"

const char* ok_200_title = "OK";
const char* error_400_title = "Bad Request";
const char* error_400_form = "Your request has bad syntax or is inherently impossible to satisfy.\n";
const char* error_403_title = "Forbidden";
const char* error_403_form = "You do not have permission to get file from this server.\n";
const char* error_404_title = "Not Found";
const char* error_404_form = "The requested file was not found on this server.\n";
const char* error_500_title = "Internal Error";
const char* error_500_form = "There was an unusual problem serving the requested file.\n";

const char* doc_root = "/var/www/html";

void http_response::init(char* url, bool keep_alive, int code) {
    m_url = url;
    m_keep_alive = keep_alive;
    m_code = code;  // 200 or 400
}

bool http_response::add_response(const char* format, ...) {
    const int WRITE_BUFFER_SIZE = 1024;

    // will it happen?
    if (*m_write_idx >= WRITE_BUFFER_SIZE) {
        return false;
    }

    va_list arg_list;
    va_start(arg_list, format);

    int len = vsnprintf(m_write_buf + *m_write_idx, WRITE_BUFFER_SIZE - 1 - *m_write_idx, format, arg_list);

    // will it happen?
    if (len >= (WRITE_BUFFER_SIZE - 1 - *m_write_idx)) {
        return false;
    }

    *m_write_idx += len;
    va_end(arg_list);
    return true;
}

bool http_response::add_status_line(int status, const char* title) {
    return add_response("%s %d %s\r\n", "HTTP/1.1", status, title);
}

bool http_response::add_headers(int content_len) {
    add_content_length(content_len);
    add_linger();
    add_blank_line();
}

bool http_response::add_content_length(int content_len) {
    return add_response("Content-Length: %d\r\n", content_len);
}

bool http_response::add_linger() {
    return add_response("Connection: %s\r\n", (m_keep_alive == true) ? "keep-alive" : "close");
}

bool http_response::add_blank_line() {
    return add_response("%s", "\r\n");
}

bool http_response::add_content(const char* content) {
    return add_response("%s", content);
}

// I think it is do_response is more properly
void http_response::do_request() {
    strcpy(m_real_file, doc_root);

    int len = strlen(doc_root);

    // FIXME
    // m_url = "/index.html";
    // m_url = "/";

    // strncpy(m_real_file + len, m_url, FILENAME_LEN - len - 1);

    if (stat(m_real_file, &m_file_stat) < 0) {
        m_code = 404;
        return;
    }

    if (!(m_file_stat.st_mode & S_IROTH)) {
        m_code = 403;
        return;
    }

    if (S_ISDIR(m_file_stat.st_mode)) {
        m_code = 400;
        return;
    }

    int fd = open(m_real_file, O_RDONLY);
    m_file_address = (char*)mmap(0, m_file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
}

bool http_response::process_write(char* buf, int* idx) {
    m_write_buf = buf;
    m_write_idx = idx;

    // read and map file to memory
    if (m_code == 200) {
        do_request();
    }

    switch (m_code) {
        case 500: {
            add_status_line(500, error_500_title);
            add_headers(strlen(error_500_form));
            if (!add_content(error_500_form)) {
                return false;
            }
            break;
        }

        case 400: {
            add_status_line(400, error_400_title);
            add_headers(strlen(error_400_form));
            if (!add_content(error_400_form)) {
                return false;
            }
            break;
        }

        case 404: {
            add_status_line(404, error_404_title);
            add_headers(strlen(error_404_form));
            if (!add_content(error_404_form)) {
                return false;
            }
            break;
        }

        case 403: {
            add_status_line(403, error_403_title);
            add_headers(strlen(error_403_form));
            if (!add_content(error_403_form)) {
                return false;
            }
            break;
        }

        case 200: {
            add_status_line(200, ok_200_title);
            if (m_file_stat.st_size != 0) {
                add_headers(m_file_stat.st_size);
                return true;
            }
            // } else {
            //     const char* ok_string = "<html><body></body></html>";
            //     add_headers(strlen(ok_string));
            //     if (!add_content(ok_string)) {
            //         return false;
            //     }
            // }
        }
        default: { return false; }
    }

    return true;
}

void http_response::unmap() {
    if (m_file_address) {
        munmap(m_file_address, m_file_stat.st_size);
        m_file_address = 0;
    }
}

char* http_response::getFileAddr() {
    return m_file_address;
}

int http_response::getFileLen() {
    return m_file_stat.st_size;
}
