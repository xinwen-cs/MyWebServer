#ifndef LOG_H
#define LOG_H

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <string>

#include "../locker/locker.h"
#include "block_queue.h"

class Log {
public:
    static Log* get_instance() {
        static Log instance;
        return &instance;
    }

    bool init(const char* file_name, int log_buf_size, int split_line, int max_queue_size);

    void write_log(int level, const char* format, ...);
    // void flush();
private:
    Log();
    ~Log();
    static void* write_log_thread(void*);

    char* m_buf;
    FILE* m_fp;
    block_queue<std::string>* m_log_queue;
    locker m_mutex;
    long long m_count;
    int m_split_line;
    int m_log_buf_size;
};

#define LOG_DEBUG(format, ...) \
    { Log::get_instance()->write_log(0, format, ##__VA_ARGS__); }
#define LOG_INFO(format, ...) \
    { Log::get_instance()->write_log(1, format, ##__VA_ARGS__); }
#define LOG_WARN(format, ...) \
    { Log::get_instance()->write_log(2, format, ##__VA_ARGS__); }
#define LOG_ERROR(format, ...) \
    { Log::get_instance()->write_log(3, format, ##__VA_ARGS__); }

#endif
