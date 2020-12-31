#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H

#include <fcntl.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

class http_response {
public:
    void init(char* url, bool keep_alive, int code);

    bool process_write(char* buf, int* idx);

    char* getFileAddr();
    int getFileLen();
    void unmap();

private:
    bool add_response(const char* format, ...);
    bool add_content(const char* content);
    bool add_status_line(int status, const char* title);
    void add_headers(int content_length);
    bool add_content_length(int content_length);
    bool add_linger();
    bool add_blank_line();

    // open file and mmap to memory
    void do_request();

    /******************************/
    int* m_write_idx;
    char* m_write_buf;
    /******************************/

    static const int FILENAME_LEN = 200;
    char m_real_file[FILENAME_LEN];

    char* m_file_address;
    struct stat m_file_stat;

    // HTTP code
    int m_code;
    char* m_url;
    bool m_keep_alive;
};

#endif
