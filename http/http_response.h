#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H

#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <unordered_map>

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
    bool add_status_line();
    void add_headers(int content_length);
    bool add_content_length(int content_length);
    bool add_linger();
    bool add_blank_line();

    // open file and mmap to memory
    void do_response();

    /******************************/
    int* m_write_idx;
    char* m_write_buf;
    /******************************/

    char* m_file_address;
    struct stat m_file_stat;

    static const char* doc_root;

    // HTTP code
    int m_code;
    char* m_url;
    bool m_keep_alive;

    std::string getFileType();
    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;
    static const std::unordered_map<int, std::string> CODE_STATUS;
    static const std::unordered_map<int, std::string> ERROR_HTML;
};

#endif
