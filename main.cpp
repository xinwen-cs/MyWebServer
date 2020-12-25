#include "config/config.h"
#include "webserver/webserver.h"

int main(int argc, char* argv[]) {
    Config config;
    config.parse_args(argc, argv);

    WebServer web_server(config);

    web_server.eventLoop();

    return 0;
}
