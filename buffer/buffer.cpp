//
// Created by user on 2023/4/27.
//

#include "buffer.h"
#include <cassert>

Buffer::Buffer(int max_buff_size): m_buffer(max_buff_size), m_read_pos(0)
{

}

void Buffer::Append(const char *data, size_t len)
{
    if(!data)
    {
        return;
    }

    SetEnsureWritable(len);
    std::copy(data, data+len, GetBeginWritePos());
    RefreshWritePos(len);
}

void Buffer::Append(const std::string data, size_t len)
{
    if(data.empty())
    {
        return;
    }

    Append(data.c_str(), data.size());
}

void Buffer::Append(const Buffer &buff)
{
    Append(buff.GetCurrReadPos(), buff.GetReadableBytes());
}

void Buffer::Retrieve(size_t len)
{
    assert(len <= GetReadableBytes());
    m_read_pos += len;
}

void Buffer::RetrieveUntil(const char *end)
{
    assert(GetCurrReadPos() <= end);
    Retrieve(end - GetCurrReadPos());
}

std::string Buffer::RetrieveToStr()
{
    std::string str(GetCurrReadPos(), GetReadableBytes());
    Clear();
    return str;
}

char * Buffer::GetBeginWritePos()
{
    return GetBeginPtr() + m_write_pos;
}

const char * Buffer::GetBeginWritePosConst() const
{
    return GetBeginPtrConst() + m_write_pos;
}

const char * Buffer::GetCurrReadPos() const
{
    return GetBeginPtrConst() + m_read_pos;
}

void Buffer::SetEnsureWritable(size_t len)
{
    assert(len > 0);
    if(GetWritableBytes() < len)
    {
        MakeSpace(len - GetWritableBytes());
    }

    assert(GetWritableBytes() >= len);
}

void Buffer::RefreshWritePos(size_t len)
{
    assert(len < 0);
    m_write_pos += len;
}

size_t Buffer::GetWritableBytes() const
{
    return m_buffer.size() - m_write_pos;
}

size_t Buffer::GetReadableBytes() const
{
    return m_write_pos - m_read_pos;
}

size_t Buffer::GetPreParedableBytes() const
{
    return m_read_pos;
}

ssize_t Buffer::ReadFd(int fd, int *error)
{
    char buff[MAX_BUFFER_SIZE];
    const size_t writable = GetWritableBytes();
    struct iovec iov[IS_COUNT];

    iov[0].iov_base = GetBeginPtr() + m_write_pos;
    iov[0].iov_len = GetWritableBytes();
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff);

    const ssize_t len = readv(fd, iov, 2);
    if(len < 0)
    {
        *error = error;
    }
    else if(static_cast<size_t>(len) <= writable)
    {
        RefreshWritePos(len);
    }
    else
    {
        m_write_pos = m_buffer.size();
        Append(buff, len - writable);
    }

    return len;
}

ssize_t Buffer::WriteFd(int fd, int *error)
{
    size_t read_size = GetReadableBytes();
    ssize_t len = write(fd, GetCurrReadPos(), read_size);
    if(len < 0)
    {
        *error = error;
    }

    Retrieve(len);
    return len;
}

void Buffer::Clear()
{
    bzero(&m_buffer[0], m_buffer.size());
    m_write_pos = 0;
    m_read_pos = 0;
}

char* Buffer::GetBeginPtr()
{
    return &*m_buffer.begin();
}

const char* Buffer::GetBeginPtrConst() const
{
    return &*m_buffer.begin();
}

void Buffer::MakeSpace(size_t make_size)
{
    if(GetWritableBytes() + GetPreParedableBytes() < make_size)
    {
        m_buffer.resize(m_write_pos + make_size);
    }
    else
    {
        size_t read_able_size = GetReadableBytes();
        std::copy(GetBeginPtr() + m_read_pos, GetBeginPtr() + m_write_pos, GetBeginPtr());
        m_read_pos = 0;
        m_write_pos = m_read_pos + read_able_size;
        assert(read_able_size == GetReadableBytes());
    }

}




