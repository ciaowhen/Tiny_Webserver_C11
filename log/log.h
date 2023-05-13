//
// Created by ciaowhen on 2023/5/7.
//

#ifndef TINY_WEBSERVER_C11_LOG_H
#define TINY_WEBSERVER_C11_LOG_H

#include <mutex>
#include <string>
#include <thread>
#include "../buffer/buffer.h"
#include "blockqueue.h"
#include <sys/stat.h>
#include <assert.h>

class Log
{
public:
    enum LOG_LEVEL
    {
        LL_DEBUG = 0,
        LL_INFO,
        LL_WARN,
        LL_ERROR,
    };

    void Init(int level = LL_DEBUG, const char *path = "./log", const char *suffix = ".log", int max_queue_capacity = 1024);
    void Write(int level, const char *format, ...);
    static Log *Instance();
    static void FlushLogThread();
    void Flush();
    int GetLevel();
    void SetLevel(int level);
    bool IsOpen(){return is_open;}

private:
    Log();
    virtual ~Log();
    void AsyncWrite();
    void AppendLogLevelTitle(int level);

private:
    static const int LOG_PATH_LEN = 256;
    static const int LOG_NAME_LEN = 256;
    static const int LOG_MAX_LINES = 50000;

    const char* m_path;
    const char* m_suffix;
    int m_max_lines;
    int m_line_count;
    int m_today;
    int m_level;
    bool is_open;
    bool is_async;
    Buffer m_buff;
    FILE *m_file;
    std::unique_ptr<BlockDeque<std::string>> m_block_deque;
    std::unique_ptr<std::thread> m_write_thread;
    std::mutex m_mutex;
};

#define LOG_BASE(level, format, ...) \
    do{\
        Log *log = Log::Instance();  \
        if(log->IsOpen() && log->GetLevel <= level) \
        {                            \
            log->Write(level, format, ##__VA_ARGS__); \
            log->Flush();            \
        }\
    }while(0);

#define LOG_DEBUG(format, ...) do{LOG_BASE(0, format, ##__VA_ARGS__)} while(0);
#define LOG_INFO(format, ...) do{LOG_BASE(1, format, ##__VA_ARGS__)} while(0);
#define LOG_WARN(format, ...) do{LOG_BASE(2, format, ##__VA_ARGS__)} while(0);
#define LOG_ERROR(format, ...) do{LOG_BASE(3, format, ##__VA_ARGS__)} while(0);

#endif //TINY_WEBSERVER_C11_LOG_H
