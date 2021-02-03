#include "buffer.h"

Buffer::Buffer(int init_buf_size) : buffer(init_buf_size), readPos(0), writePos(0) {}

size_t Buffer::ReadableBytes() const {
    return writePos - readPos;
}

size_t Buffer::WritableBytes() const {
    return buffer.size() - writePos;
}

size_t Buffer::PrependableBytes() const {
    return readPos;
}

const char* Buffer::Peek() const {
    return BeginPtr() + readPos;
}

void Buffer::Retrieve(size_t len) {
    assert(len <= ReadableBytes());
    readPos += len;
}

void Buffer::RetrieveUntil(const char* end) {
    assert(Peek() <= end);
    Retrieve(end - Peek());
}

void Buffer::RetrieveAll() {
    buffer.clear();
    readPos = 0;
    writePos = 0;
}

// RVO?
std::string Buffer::RetriveAllToStr() {
    std::string str(Peek(), ReadableBytes());
    RetrieveAll();
    return str;
}

const char* Buffer::BeginWriteConst() const {
    return BeginPtr() + writePos;
}

char* Buffer::BeginWrite() {
    return BeginPtr() + writePos;
}

void Buffer::HasWritten(size_t len) {
    writePos += len;
}

void Buffer::Append(const std::string& str) {
    Append(str.data(), str.length());
}

void Buffer::Append(const void* data, size_t len) {
    Append(static_cast<const char*>(data), len);
}

void Buffer::Append(const Buffer& buff) {
    Append(buff.Peek(), buff.ReadableBytes());
}

void Buffer::Append(const char* str, size_t len) {
    assert(str);
    EnsureWritable(len);
    std::copy(str, str + len, BeginWrite());
    HasWritten(len);
}

void Buffer::EnsureWritable(size_t len) {
    if (WritableBytes() < len) {
        MakeSpace(len);
    }
    assert(WritableBytes() >= len);
}

char* Buffer::BeginPtr() {
    return buffer.data();
}

const char* Buffer::BeginPtr() const {
    return buffer.data();
}

void Buffer::MakeSpace(size_t len) {
    if (WritableBytes() + PrependableBytes() < len) {
        buffer.resize(writePos + len + 1);
    } else {
        size_t readable = ReadableBytes();
        std::copy(BeginPtr() + readPos, BeginPtr() + writePos, BeginPtr());
        readPos = 0;
        writePos = readPos + readable;
        assert(readable == ReadableBytes());
    }
}

ssize_t Buffer::ReadFd(int fd, int* saveErrno) {
    // muduo trick
    char buff[65535];
    struct iovec iov[2];
    const size_t writable = WritableBytes();

    iov[0].iov_base = BeginPtr() + writePos;
    iov[0].iov_len = writable;
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff);

    const ssize_t len = readv(fd, iov, 2);
    if (len < 0) {
        *saveErrno = errno;
    } else if (static_cast<size_t>(len) <= writable) {
        writePos += len;
    } else {
        writePos = buffer.size();
        Append(buff, len - writable);
    }
    return len;
}

// ssize_t Buffer::WriteFd(int fd, int* saveErrno) {
//     size_t readSize = ReadableBytes();
//     ssize_t len = write(fd, Peek(), readSize);
//     if (len < 0) {
//         *saveErrno = errno;
//         return len;
//     }
//     readPos += len;
//     return len;
// }
