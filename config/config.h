#ifndef CONFIG_H
#define CONFIG_H

#include <stdlib.h>
#include <unistd.h>

class Config {
public:
    Config();
    ~Config(){};

    void parse_args(int argc, char* argv[]);

public:
    int port;
    int thread_num;
};

#endif
