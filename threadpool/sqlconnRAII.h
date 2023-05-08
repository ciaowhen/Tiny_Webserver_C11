//
// Created by user on 2023/4/27.
//

#ifndef ADVANCECODE_SQLCONNRAII_H
#define ADVANCECODE_SQLCONNRAII_H
#include "sqlconnpool.h"
#include <cassert>

class SqlConnRAII
{
public:
    SqlConnRAII(MYSQL **sql, SqlConnPool *conn_pool)
    {
        assert(conn_pool);
        *sql = conn_pool->GetSqlConn();
        m_sql = *sql;
        m_conn_pool = conn_pool;
    }

    ~SqlConnRAII()
    {
        if(m_sql)
        {
            m_conn_pool->FreeConn();
        }
    }

private:
    MYSQL *m_sql;
    SqlConnPool *m_conn_pool;
};

#endif //ADVANCECODE_SQLCONNRAII_H
