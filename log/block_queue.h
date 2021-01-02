#ifndef BLOCK_QUEUE_H
#define BLOCK_QUEUE_H

#include "../locker/locker.h"

template <typename T>
class block_queue {
public:
    block_queue(int max_size = 1000) {
        m_max_size = max_size;
        m_array = new T[m_max_size];

        m_size = 0;
        m_front = -1;
        m_back = -1;
    }

    ~block_queue() {
        delete[] m_array;
    }

    bool full() {
        m_mutex.lock();
        if (m_size >= m_max_size) {
            m_mutex.unlock();
            return true;
        }
        m_mutex.unlock();
        return false;
    }

    // why not using signal other than broadcast, there is only one consumer thread
    bool push(const T& item) {
        m_mutex.lock();
        if (m_size >= m_max_size) {
            m_cond.signal();
            m_mutex.unlock();
            return false;
        }

        m_back = (m_back + 1) % m_max_size;
        m_array[m_back] = item;

        ++m_size;

        m_cond.signal();
        m_mutex.unlock();

        return true;
    }

    bool pop(T& item) {
        m_mutex.lock();

        // only one consumer, using "if" is ok
        if (m_size <= 0) {
            if (!m_cond.wait(m_mutex.get())) {
                m_mutex.unlock();
                return false;
            }
        }

        m_front = (m_front + 1) % m_max_size;
        item = m_array[m_front];
        --m_size;
        m_mutex.unlock();
        return true;
    }

private:
    locker m_mutex;
    cond m_cond;

    T* m_array;

    int m_size;
    int m_max_size;

    int m_front;
    int m_back;
};

#endif
