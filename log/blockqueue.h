//
// Created by user on 2023/5/5.
//

#ifndef ADVANCECODE_BLOCKQUEUE_H
#define ADVANCECODE_BLOCKQUEUE_H

#include <deque>
#include <mutex>
#include <condition_variable>
#include <sys/time.h>
#include <cassert>

template<class T>
class BlockDeque
{
public:
    explicit BlockDeque(size_t max_capacity = 1000);
    ~BlockDeque();
    void Clear();
    void Close();
    bool IsEmpty();
    bool IsFull();
    size_t GetQueueSize();
    size_t GetCapacity();
    T GetFront();
    T GetBack();
    void PushBack(const T &item);
    void PushFront(const T &item);
    bool PopFront(T &item);
    bool PopFront(T &item, int timeout);
    void Flush();

private:
    std::deque<T> m_block_queue;
    size_t m_capacity;
    std::mutex m_mutex;
    bool m_close;
    std::condition_variable m_consumer;
    std::condition_variable m_producer;
};

template<class T>
BlockDeque<T>::BlockDeque(size_t max_capacity):m_capacity(max_capacity)
{
    assert(max_capacity > 0);
    m_block_queue.clear();
    m_close = false;
}

template<class T>
BlockDeque<T>::~BlockDeque()
{
    Close();
}

template<class T>
void BlockDeque<T>::Close()
{
    {
        std::lock_guard<std::mutex> locker(m_mutex);
        m_block_queue.clear();
        m_close = true;
    }

    m_producer.notify_all();
    m_consumer.notify_all();
}

template<class T>
void BlockDeque<T>::Clear()
{
    std::lock_guard<std::mutex> locker(m_mutex);
    m_block_queue.clear();
}

template<class T>
bool BlockDeque<T>::IsEmpty()
{
    std::lock_guard<std::mutex> locker(m_mutex);
    return m_block_queue.empty();
}

template<class T>
bool BlockDeque<T>::IsFull()
{
    std::lock_guard<std::mutex> locker(m_mutex);
    return m_block_queue.size() >= m_capacity;
}

template<class T>
size_t BlockDeque<T>::GetQueueSize()
{
    std::lock_guard<std::mutex> locker(m_mutex);
    return m_block_queue.size();
}

template<class T>
size_t BlockDeque<T>::GetCapacity()
{
    std::lock_guard<std::mutex> locker(m_mutex);
    return m_capacity;
}

template<class T>
T BlockDeque<T>::GetFront()
{
    std::lock_guard<std::mutex> locker(m_mutex);
    return m_block_queue.front();
}

template<class T>
T BlockDeque<T>::GetBack()
{
    std::lock_guard<std::mutex> locker(m_mutex);
    return m_block_queue.back();
}

template<class T>
void BlockDeque<T>::PushBack(const T &item)
{
    std::unique_lock<std::mutex> locker(m_mutex);
    while(m_block_queue.size() >= m_capacity)
    {
        m_producer.wait(locker);
    }

    m_block_queue.push_back(item);
    m_consumer.notify_one();
}

template<class T>
void BlockDeque<T>::PushFront(const T &item)
{
    std::unique_lock<std::mutex> locker(m_mutex);
    while(m_block_queue.size() >= m_capacity)
    {
        m_producer.wait(locker);
    }

    m_block_queue.push_front(item);
    m_consumer.notify_one();
}

template<class T>
bool BlockDeque<T>::PopFront(T &item)
{
    std::unique_lock<std::mutex> locker(m_mutex);
    while(m_block_queue.empty())
    {
        m_consumer.wait(locker);
        if(m_close)
        {
            return false;
        }
    }

   item = m_block_queue.pop_front();
    m_block_queue.pop_front();
    m_producer.notify_one();
    return true;
}

template<class T>
bool BlockDeque<T>::PopFront(T &item, int timeout)
{
    std::unique_lock<std::mutex> locker(m_mutex);
    while(m_block_queue.empty())
    {
        if(m_consumer.wait_for(locker, std::chrono::seconds(timeout)) == std::cv_status::timeout)
        {
            return false;
        }

        if(m_close)
        {
            return false;
        }
    }

    item = m_block_queue.pop_front();
    m_block_queue.pop_front();
    m_producer.notify_one();
    return true;
}

template<class T>
void BlockDeque<T>::Flush()
{
    m_consumer.notify_one();
}

#endif //ADVANCECODE_BLOCKQUEUE_H