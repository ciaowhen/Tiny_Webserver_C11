//
// Created by user on 2023/4/27.
//

#include "sqlconnpool.h"
#include "../log/log.h"
#include <cassert>

SqlConnPool::SqlConnPool():m_conn_max_num(0), m_free_conn_num(0), m_use_conn_num(0)
{

}

SqlConnPool::~SqlConnPool()
{
    Close();
}

void SqlConnPool::Init(const char *host, int port, const char *username, const char *password, const char *dbname, int conn_num)
{
    assert(conn_num > 0);
    assert(port > 1025 && port < 65535);
    assert(host != NULL && username != NULL && password != NULL && dbname != NULL);

    for(int i = 0; i < conn_num; ++i)
    {
        MYSQL *sql = nullptr;
        sql = mysql_init(sql);
        if(!sql)
        {
            LOG_ERROR("Mysql Init Error");
            assert(sql);
        }

        sql = mysql_real_connect(sql, host, username, password, dbname, port, nullptr, 0);
        if(!sql)
        {
            LOG_ERROR("Mysql Connect Error");
            assert(sql);
        }

        m_sql_queue.emplace(sql);
    }

    sem_init(&m_sem, 0, conn_num);
    m_conn_max_num = conn_num;
    m_free_conn_num = conn_num;
}

void SqlConnPool::Close()
{
    std::lock_guard<std::mutex> locker(m_mutex);
    while(!m_sql_queue.empty())
    {
        auto close_sql = m_sql_queue.front();
        m_sql_queue.pop();
        mysql_close(close_sql);
    }

    mysql_library_end();
}

SqlConnPool* SqlConnPool::Instance()
{
    static SqlConnPool sql_pool;
    return &sql_pool;
}

MYSQL* SqlConnPool::GetSqlConn()
{
    MYSQL *sql_conn = nullptr;
    if(m_sql_queue.empty())
    {
        LOG_WARN("SqlConnPool is Busy");
        return nullptr;
    }

    sem_wait(&m_sem);
    {
        std::lock_guard<std::mutex> locker(m_mutex);
        sql_conn = m_sql_queue.front();
        m_sql_queue.pop();
        m_use_conn_num++;
    }

    return sql_conn;
}

void SqlConnPool::FreeConn(MYSQL *sql)
{
    if(!sql)
    {
        return;
    }

    {
        std::lock_guard<std::mutex> locker(m_mutex);
        m_sql_queue.emplace(sql);
        m_free_conn_num++;
        sem_post(&m_sem);
    }

    return;
}

int SqlConnPool::GetFreeConnCount()
{
    std::lock_guard<std::mutex> locker(m_mutex);
    return m_sql_queue.size();
}


