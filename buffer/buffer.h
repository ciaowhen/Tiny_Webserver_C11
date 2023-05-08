//
// Created by user on 2023/4/27.
//

#ifndef ADVANCECODE_BUFFER_H
#define ADVANCECODE_BUFFER_H

#include <string>
#include <unistd.h>
#include <vector>
#include <atomic>

#define MAX_BUFFER_SIZE 65535

class Buffer
{
public:
    enum IOV_STATUS
    {
        IS_READ = 0,
        IS_WRITE,
        IS_COUNT,
    };

    Buffer(int max_buff_size = 1024);
    ~Buffer() = default;

    void Append(const char* data, size_t len);          //向缓冲区写入数据
    void Append(const std::string data, size_t len);
    void Append(const Buffer &buff);

    void Retrieve(size_t len);                          //更新已读字节数
    void RetrieveUntil(const char *end);
    std::string RetrieveToStr();

    char* GetBeginWritePos();                           //获取当前读/写的位置
    const char* GetBeginWritePosConst() const;
    const char* GetCurrReadPos() const;

    void SetEnsureWritable(size_t len);                 //更新已写字节数
    void RefreshWritePos(size_t len);

    size_t GetWritableBytes() const;
    size_t GetReadableBytes() const;
    size_t GetPreParedableBytes() const;                 //已读多少字节

    ssize_t ReadFd(int fd, int *error);                 //读取文件
    ssize_t WriteFd(int fd, int *error);                //写入文件

    void Clear();

private:
    char *GetBeginPtr();
    const char* GetBeginPtrConst() const;
    void MakeSpace(size_t make_size);

    std::vector<char> m_buffer;
    std::atomic<std::size_t> m_read_pos;
    std::atomic<std::size_t> m_write_pos;
};


#endif //ADVANCECODE_BUFFER_H
