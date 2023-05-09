//
// Created by user on 2023/5/5.
//

#include "log.h"
#include <stdarg.h>

Log::Log():m_path(nullptr),m_suffix(nullptr), m_max_lines(0),m_line_count(0),m_today(0),m_level(0),is_open(false), is_async(false),m_file(nullptr),m_block_deque(nullptr),m_write_thread(nullptr)
{

}

Log::~Log()
{
        if(m_write_thread && m_write_thread->joinable())
        {
            while(!m_block_deque->IsEmpty())
            {
                m_block_deque->Flush();
            }

            m_block_deque->Close();
            m_write_thread->join();
        }

        if(m_file)
        {
            std::lock_guard<std::mutex> locker(m_mutex);
            Flush();
            fclose(m_file);
        }
}

void Log::Init(int level, const char *path, const char *suffix, int max_queue_capacity)
{
    is_open = true;
    m_level = level;
    if(max_queue_capacity > 0)
    {
        is_async = true;
        if(!m_block_deque)
        {
            std::unique_ptr<BlockDeque<std::string>> new_deque(new BlockDeque<std::string>);
            new_deque = std::move(new_deque);
            std::unique_ptr<std::thread> new_thread(new std::thread(FlushLogThread));
            m_write_thread = std::move(new_thread);
        }
        else
        {
            is_async = false;
        }
    }

    m_line_count = 0;
    time_t now_time = time(nullptr);
    struct tm *sys_time = localtime(&now_time);
    m_path = path;
    m_suffix = suffix;
    char file_name[LOG_NAME_LEN] = {0};
    snprintf(file_name,LOG_NAME_LEN - 1, "%s%04d_%02d_%02d%s", m_path, sys_time->tm_year + 1990, sys_time->tm_mon + 1, sys_time->tm_mday, m_suffix);
    m_today = sys_time->tm_mday;

    {
        std::lock_guard<std::mutex> locker(m_mutex);
        m_buff.Clear();
        if(m_file)
        {
            Flush();
            fclose(m_file);
        }

        m_file = fopen(file_name, "a");
        if(!m_file)
        {
            mkdir(m_path, 0777);
            m_file = fopen(file_name, "a");
        }

        assert(m_file != nullptr);
    }
}

void Log::Write(int level, const char *format, ...)
{
    time_t now_time = time(0);
    struct tm *sys_time = localtime(&now_time);
    va_list valist;

    if(m_today != sys_time->tm_mday || (m_line_count && (m_line_count % LOG_MAX_LINES == 0)))
    {
        std::unique_lock<std::mutex> locker(m_mutex);
        locker.unlock();

        char new_file_name[LOG_NAME_LEN];
        char tail[36] = {0};
        std::snprintf(tail, 36, "%04d_%02d_%02d", sys_time->tm_year + 1900, sys_time->tm_mon + 1, sys_time->tm_mday);

        if(m_today != sys_time->tm_mday)
        {
            snprintf(new_file_name, LOG_NAME_LEN - 72, "%s/%s%s", m_path,tail, m_suffix);
            m_today = sys_time->tm_mday;
            m_line_count = 0;
        }
        else
        {
            snprintf(new_file_name, LOG_NAME_LEN - 72, "%s/%s-%d%s", m_path, tail, m_line_count / LOG_MAX_LINES, m_suffix);
        }

        locker.lock();
        Flush();
        fclose(m_file);
        m_file = fopen(new_file_name, "a");
        assert(m_file != nullptr);
    }

    {
        std::unique_lock<std::mutex> locker(m_mutex);
        m_line_count++;
        int n = snprintf(m_buff.GetBeginWritePos(), 128, "%d-%02d-%02d %02d:%02d:%02d",
                         sys_time->tm_year + 1990, sys_time->tm_mon + 1, sys_time->tm_mday, sys_time->tm_hour, sys_time->tm_min, sys_time->tm_sec);

        m_buff.RefreshWritePos(n);
        AppendLogLevelTitle(m_level);

        va_start(valist, format);
        int m = snprintf(m_buff.GetBeginWritePos(), m_buff.GetWritableBytes(), format, valist);
        va_end(valist);
        m_buff.RefreshWritePos(m);
        m_buff.Append("\n\0", 2);

        if(is_async && m_block_deque && !m_block_deque->IsFull())
        {
            m_block_deque->PushBack(m_buff.RetrieveToStr());
        }
        else
        {
            fputs(m_buff.GetCurrReadPos(), m_file);
        }

        m_buff.Clear();
    }
}

Log* Log::Instance()
{
    static Log m_log;
    return &m_log;
}

void Log::FlushLogThread()
{
    Log::Instance()->AsyncWrite();
}

void Log::Flush()
{
    if(is_async)
    {
        m_block_deque->Flush();
    }

    fflush(m_file);
}

int Log::GetLevel()
{
    std::lock_guard<std::mutex> locker(m_mutex);
    return m_level;
}

void Log::SetLevel(int level)
{
    std::lock_guard<std::mutex> locker(m_mutex);
    m_level = level;
}

void Log::AsyncWrite()
{
    std::string str = "";
    while(m_block_deque->PopFront(str))
    {
        std::lock_guard<std::mutex> locker(m_mutex);
        fputs(str.c_str(), m_file);
    }
}

void Log::AppendLogLevelTitle(int level)
{
    switch (level) 
    {
        case 0:
            m_buff.Append("[DEBUG]: ", 9);
            break;
        case 1:
            m_buff.Append("[INFO]: ", 9);
            break;
        case 2:
            m_buff.Append("[WARN]: ", 9);
            break;
        case 3:
            m_buff.Append("[ERROR]: ", 9);
            break;
        default:
            m_buff.Append("[INFO]: ", 9);
            break;
    }
}