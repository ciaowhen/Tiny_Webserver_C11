//
// Created by user on 2023/4/27.
//

#ifndef ADVANCECODE_SQLCONNPOOL_H
#define ADVANCECODE_SQLCONNPOOL_H

#include <mysql/mysql.h>
#include <string>
#include <mutex>
#include <thread>
#include <queue>
#include <semaphore.h>

class SqlConnPool
{
public:
    void Init(const char *host, int port, const char *username, const char *password, const char *dbname, int conn_num);
    void Close();

    static SqlConnPool *Instance();
    MYSQL *GetSqlConn();
    void FreeConn(MYSQL *sql);
    int GetFreeConnCount();

private:
    SqlConnPool();
    ~SqlConnPool();

    int m_conn_max_num;     //连接池连接数量
    int m_free_conn_num;    //连接池当前空闲连接数
    int m_use_conn_num;     //连接池当前已用连接数

    std::queue<MYSQL *> m_sql_queue;
    sem_t m_sem;
    std::mutex m_mutex;
};


#endif //ADVANCECODE_SQLCONNPOOL_H
