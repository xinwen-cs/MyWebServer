#include "log.h"

Log::Log() {}

Log::~Log() {
    if (m_fp != NULL) {
        fclose(m_fp);
        m_fp = NULL;
    }

    if (m_buf != NULL) {
        delete[] m_buf;
        m_buf = NULL;
    }
}


bool Log::init(const char* file_name, int log_buf_size, int split_line, int max_queue_size) {
    m_log_buf_size = log_buf_size;

    m_buf = new char[m_log_buf_size];
    memset(m_buf, '\0', m_log_buf_size);

    m_log_queue = std::make_unique<block_queue<std::string>>(max_queue_size);

    pthread_t tid;
    pthread_create(&tid, NULL, write_log_thread, NULL);

    m_split_line = split_line;

    // create log file
    time_t t = time(NULL);
    struct tm* sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;

    char log_full_name[256] = {0};
    snprintf(log_full_name, 255, "%d_%02d_%02d_%s", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, file_name);

    m_fp = fopen(log_full_name, "a");
    if (m_fp == NULL) {
        return false;
    }

    printf("success create log filename: %s\n", log_full_name);
    return true;
}

// two way to use non-static member in a static function:
// 1. singleton static member
// 2. pass object as argument
void* Log::write_log_thread(void*) {
    Log* log = Log::get_instance();
    std::string log_str;

    while (log->m_log_queue->pop(log_str)) {
        log->m_mutex.lock();
        fputs(log_str.c_str(), log->m_fp);
        if (log->m_count % 100000 == 0) {
            fflush(log->m_fp);
        }

        log->m_mutex.unlock();
    }

    return NULL;
}

void Log::write_log(int level, const char* format, ...) {
    struct timeval now = {0, 0};
    gettimeofday(&now, NULL);
    time_t t = now.tv_sec;
    struct tm* sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;

    char s[16] = {0};

    switch (level) {
        case 0: {
            strcpy(s, "[debug]:");
            break;
        }
        case 1: {
            strcpy(s, "[info]:");
            break;
        }
        case 2: {
            strcpy(s, "[warn]:");
            break;
        }
        case 3: {
            strcpy(s, "[error]:");
            break;
        }
        default: { strcpy(s, "[info]:"); }
    }

    ++m_count;

    va_list valst;

    va_start(valst, format);

    m_mutex.lock();

    int n = snprintf(m_buf, 48, "%d-%02d-%02d %02d:%02d:%02d.%06ld %s ", my_tm.tm_year + 1900, my_tm.tm_mon + 1,
                     my_tm.tm_mday, my_tm.tm_hour, my_tm.tm_min, my_tm.tm_sec, now.tv_usec, s);

    // 内容格式化，用于向字符串中打印数据、数据格式用户自定义，返回写入到字符数组str中的字符个数(不包含终止符)
    int m = vsnprintf(m_buf + n, m_log_buf_size - 1, format, valst);
    m_buf[n + m] = '\n';
    m_buf[n + m + 1] = '\0';

    std::string log_str(m_buf);

    m_mutex.unlock();

    va_end(valst);

    if (!m_log_queue->full()) {
        m_log_queue->push(log_str);
    }
}

// void Log::flush() {
//     m_mutex.lock();
//     fflush(m_fp);
//     m_mutex.unlock();
// }
