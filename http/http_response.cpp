#include "http_response.h"

const std::unordered_map<std::string, std::string> http_response::SUFFIX_TYPE = {
    { ".html",  "text/html" },
    { ".xml",   "text/xml" },
    { ".xhtml", "application/xhtml+xml" },
    { ".txt",   "text/plain" },
    { ".rtf",   "application/rtf" },
    { ".pdf",   "application/pdf" },
    { ".word",  "application/msword" },
    { ".png",   "image/png" },
    { ".gif",   "image/gif" },
    { ".jpg",   "image/jpeg" },
    { ".jpeg",  "image/jpeg" },
    { ".au",    "audio/basic" },
    { ".mpeg",  "video/mpeg" },
    { ".mpg",   "video/mpeg" },
    { ".avi",   "video/x-msvideo" },
    { ".gz",    "application/x-gzip" },
    { ".tar",   "application/x-tar" },
    { ".css",   "text/css "},
    { ".js",    "text/javascript "},
};

const std::unordered_map<int, std::string> http_response::CODE_STATUS = {
    {200, "OK"},
    {400, "Bad Request"},
    {403, "Forbidden"},
    {404, "Not Found"},
};

const std::unordered_map<int, std::string> ERROR_HTML = {
    {400, "/400.html"},
    {403, "/403.html"},
    {404, "/404.html"}
};

const char* http_response::doc_root = "/var/www/html";

void http_response::init(char* url, bool keep_alive, int code) {
    m_url = url;
    m_keep_alive = keep_alive;
    m_code = code;  // 200 or 400
    m_file_address = NULL;
}

std::string http_response::getFileType() {
    if (m_url == NULL) {
        return "text/plain";
    }

    std::string path = std::string(m_url);
    std::string::size_type idx = path.find_last_of('.');

    if (idx == std::string::npos) {
        return "text/plain";
    }

    std::string suffix = path.substr(idx);
    if (SUFFIX_TYPE.count(suffix)) {
        return SUFFIX_TYPE.find(suffix)->second;
    }

    // warning: control reaches end of non-void function [-Wreturn-type]
    return "text/plain";
}

void http_response::add_status_line(Buffer& buf) {
    std::string status = CODE_STATUS.find(m_code)->second;
    buf.Append("HTTP/1.1 " + std::to_string(m_code) + " " + status.c_str() + "\r\n");
}

void http_response::add_headers(Buffer& buf, int content_len) {
    buf.Append("Content-Length: " + std::to_string(content_len) + "\r\n");

    buf.Append("Connection: ");
    buf.Append((m_keep_alive == true) ? "keep-alive" : "close");
    buf.Append("\r\n");

    buf.Append("Content-type: " + getFileType() + "\r\n");
    buf.Append("\r\n");
}

void http_response::add_content(Buffer& buf, const char* content) {
    buf.Append(content);
}

void http_response::do_response() {
    const int FILENAME_LEN = 200;
    char m_real_file[FILENAME_LEN];

    strcpy(m_real_file, doc_root);
    int len = strlen(doc_root);
    strncpy(m_real_file + len, m_url, FILENAME_LEN - len - 1);

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

bool http_response::process_write(Buffer& buf) {
    // read and map file to memory
    if (m_code == 200) {
        do_response();
    }

    if (m_code == 200) {
        add_status_line(buf);
        add_headers(buf, m_file_stat.st_size);
    } else {
        // FIXME
        add_status_line(buf);
        add_headers(buf, strlen("You do not have permission to get file from this server.\n"));
        add_content(buf, "You do not have permission to get file from this server.\n");
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
