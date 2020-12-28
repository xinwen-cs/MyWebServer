#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

class http_request {
public:
    enum METHOD { GET = 0, POST, HEAD, PUT, DELETE, TRACE, OPTIONS, CONNECT, PATCH };

    enum CHECK_STATE { CHECK_STATE_REQUESTLINE = 0, CHECK_STATE_HEADER, CHECK_STATE_CONTENT };

    enum HTTP_CODE {
        NO_REQUEST,
        GET_REQUEST,
        BAD_REQUEST,

        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION
    };

    enum LINE_STATUS { LINE_OK = 0, LINE_BAD, LINE_OPEN };

    int process_read(char* buf, int idx);

    // called by http_response
    bool getKeepAlive();
    char* getPath();
private:
    HTTP_CODE parse_request_line(char* text);
    HTTP_CODE parse_headers(char* text);
    HTTP_CODE parse_content(char* text);
    LINE_STATUS parse_line();

    /**********************/
    int m_read_idx;
    char* m_read_buf;
    /**********************/

    int m_checked_idx;
    int m_start_line;

    CHECK_STATE m_check_state;
    METHOD m_method;

    // not used
    char* m_version;
    char* m_host;
    int m_content_length;

    // send to response
    char* m_url;
    bool m_keep_alive;
};

#endif
