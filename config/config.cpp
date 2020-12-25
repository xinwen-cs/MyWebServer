#include "config.h"

Config::Config() {
    port = 12345;
    thread_num = 8;
}

void Config::parse_args(int argc, char* argv[]) {
    int opt;

    const char* pattern = "p:t:";

    while ((opt = getopt(argc, argv, pattern)) != -1) {
        switch (opt) {
            case 'p': {
                port = atoi(optarg);
                break;
            }

            case 't': {
                thread_num = atoi(optarg);
                break;
            }
        }
    }
}
