#ifndef EPOLLER_H
#define EPOLLER_H

#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <vector>

class Epoller {
public:
    explicit Epoller(int max_events = 1024);
    ~Epoller();

    void addfd(int fd, bool one_shot);
    void removefd(int fd);
    void modfd(int fd, uint32_t ev);

    int wait(int timeout = -1);

    int getEventsFd(size_t i);

    uint32_t getEvents(size_t i);

private:
    int epoll_fd;
    std::vector<struct epoll_event> events;

    int setnonblocking(int fd);  // used by addfd
};

#endif
