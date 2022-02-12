#include <mysql/mysql.h>
#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <list>
#include <pthread.h>
#include <iostream>
#include "sql_connection_pool.h"

sql_connection::sql_connection(std::string url, std::string User, std::string PassWord, std::string DBName, int Port)
{
	sql = mysql_init(sql);
	if (sql == NULL)
	{
		LOG_ERROR("MySQL Error");
		exit(1);
	}

	sql = mysql_real_connect(sql, url.c_str(), User.c_str(), PassWord.c_str(), DBName.c_str(), Port, NULL, 0);
	if (sql == NULL)
	{
		LOG_ERROR("MySQL Error");
		exit(1);
	}
}

sql_connection::~sql_connection()
{
	mysql_close(sql);
}

void sql_connection::get_user_table(std::map<std::string, std::string>& user_table) const 
{
    //在user表中检索username，passwd数据，浏览器端输入
    if (mysql_query(sql, "SELECT username,passwd FROM user"))
    {
        LOG_ERROR("SELECT error:%s\n", mysql_error(sql));
    }

    //从表中检索完整的结果集
    MYSQL_RES *result = mysql_store_result(sql);

    //从结果集中获取下一行，将对应的用户名和密码，存入map中
    while (MYSQL_ROW row = mysql_fetch_row(result))
    {
        std::string temp1(row[0]);
        std::string temp2(row[1]);
        user_table[temp1] = temp2;
    }
}

int sql_connection::query(const char* content) const
{
	return mysql_query(sql, content);
}
connection_pool *connection_pool::GetInstance()
{
	static connection_pool connPool;
	return &connPool;
}

//构造初始化
void connection_pool::init(std::string url, std::string User, std::string PassWord, std::string DBName, int Port, int MaxConn, int close_log)
{
	m_url = url;
	m_Port = Port;
	m_User = User;
	m_PassWord = PassWord;
	m_DatabaseName = DBName;
	m_close_log = close_log;

	for (int i = 0; i < MaxConn; i++)
	{
		connList.push_back(std::shared_ptr<sql_connection>(url, User, PassWord, DBName, Port));
		++m_FreeConn;
	}

	reserve = sem(m_FreeConn);

	m_MaxConn = m_FreeConn;
}


//当有请求时，从数据库连接池中返回一个可用连接，更新使用和空闲连接数
std::shared_ptr<sql_connection> connection_pool::GetConnection() const
{
	if (0 == connList.size())
		return NULL;

	reserve.wait();
	
	locker_guard scopeLock(lock);

	auto con = connList.front();
	connList.pop_front();

	--m_FreeConn;
	++m_CurConn;

	return con;
}

//释放当前使用的连接
bool connection_pool::ReleaseConnection(const std::shared_ptr<sql_connection>& con)
{
	if (NULL == con)
		return false;

	locker_guard scopeLock(lock);

	connList.push_back(con);
	++m_FreeConn;
	--m_CurConn;

	reserve.post();
	return true;
}

//当前空闲的连接数
int connection_pool::GetFreeConn() const
{
	return this->m_FreeConn;
}

connection_pool_guard::connection_pool_guard(){
	connection_pool* sqlPool = connection_pool::GetInstance();
	con = sqlPool->GetConnection();
}

connection_pool_guard::~connection_pool_guard(){
	connection_pool* sqlPool = connection_pool::GetInstance();
	sqlPool->ReleaseConnection(con);
}
	
void connection_pool_guard::get_user_table(std::map<std::string, std::string>& user_table) const
{
	con->get_user_table(user_table);
}

int connection_pool_guard::query(const char* content) const
{
	return con->query(content);
}
