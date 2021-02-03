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
#include <string>

#include "../buffer/buffer.h"

class http_response {
public:
    void init(char* url, bool keep_alive, int code);

    bool process_write(Buffer& buf);

    char* getFileAddr();
    int getFileLen();
    void unmap();

private:
    // bool add_response(const char* format, ...);
    void add_content(Buffer& buf, const char* content);
    void add_status_line(Buffer& buf);
    void add_headers(Buffer& buf, int content_length);
    void add_content_length(Buffer& buf, int content_length);
    // bool add_linger();
    // bool add_blank_line();

    // open file and mmap to memory
    void do_response();

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
