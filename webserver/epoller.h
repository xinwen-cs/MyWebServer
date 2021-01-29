#ifndef EPOLLER_H
#define EPOLLER_H

#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <vector>

#define MAX_EVENT_NUMBER 10000

class Epoller {
public:
    Epoller();
    ~Epoller();

    void addfd(int fd, bool one_shot);
    void removefd(int fd);
    void modfd(int fd, uint32_t ev);

    int wait(int timeout = -1);

    int getEventFd(size_t i);

    uint32_t getEvents(size_t i);

private:
    int epoll_fd;
    epoll_event events[MAX_EVENT_NUMBER];

    int setnonblocking(int fd); // used by addfd
};

#endif
