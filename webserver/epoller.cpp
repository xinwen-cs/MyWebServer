#include "epoller.h"

Epoller::Epoller() {
    epoll_fd = epoll_create(512);
}

Epoller::~Epoller() {
    close(epoll_fd);
}

void Epoller::addfd(int fd, bool one_shot) {
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    if (one_shot) {
        event.events |= EPOLLONESHOT;
    }
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

void Epoller::removefd(int fd) {
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, 0);
}

void Epoller::modfd(int fd, uint32_t ev) {
    epoll_event event;
    event.data.fd = fd;
    event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &event);
}

int Epoller::setnonblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

int Epoller::wait(int timeout) {
    return epoll_wait(epoll_fd, events, MAX_EVENT_NUMBER, timeout);
}

int Epoller::getEventFd(size_t i) {
    return events[i].data.fd;
}

uint32_t Epoller::getEvents(size_t i) {
    return events[i].events;
}
