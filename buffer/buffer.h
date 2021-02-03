#ifndef BUFFER_H
#define BUFFER_H

#include <assert.h>
#include <string.h>
#include <sys/uio.h>
#include <unistd.h>
#include <atomic>
#include <string>
#include <vector>

class Buffer {
public:
    Buffer(int init_buf_size = 1024);
    ~Buffer() = default;

    size_t WritableBytes() const;
    size_t ReadableBytes() const;
    size_t PrependableBytes() const;

    const char* Peek() const;

    void Retrieve(size_t len);
    void RetrieveUntil(const char* end);
    void RetrieveAll();
    std::string RetriveAllToStr();

    const char* BeginWriteConst() const;
    char* BeginWrite();

    void Append(const std::string& str);
    void Append(const void* data, size_t len);
    void Append(const Buffer& buff);
    void Append(const char* str, size_t len);

    ssize_t ReadFd(int fd, int* Errno);
    // ssize_t WriteFd(int fd, int* Errno);

private:
    char* BeginPtr();
    const char* BeginPtr() const;

    void EnsureWritable(size_t len);
    void HasWritten(size_t len);

    void MakeSpace(size_t len);

    std::vector<char> buffer;

    size_t readPos;
    size_t writePos;
};

#endif
