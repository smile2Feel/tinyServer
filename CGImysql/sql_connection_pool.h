#ifndef _CONNECTION_POOL_
#define _CONNECTION_POOL_

#include <stdio.h>
//todo: why not vector? sql connection will overdue?
#include <list>
#include <mysql/mysql.h>
#include <error.h>
#include <iostream>
#include <string>
#include <memory>
#include <map>
#include "../lock/locker.h"
#include "../log/log.h"
#include "../noncopyable.h"

class sql_connection : noncopyable
{
public:
	sql_connection(std::string url, std::string User, std::string PassWord, std::string DBName, int Port);
	~sql_connection();
	void get_user_table(std::map<std::string, std::string>&) const;
	int query(const char*) const;
private:
	MYSQL *sql;
};

class connection_pool : noncopyable
{
public:
	// internal use only.
	// can't return a stack object's reference
	std::shared_ptr<sql_connection> GetConnection() const;
	bool ReleaseConnection(const std::shared_ptr<sql_connection>& con)
	int GetFreeConn();					 //获取连接

	//单例模式
	static connection_pool *GetInstance();

	void init(std::string url, std::string User, std::string PassWord, std::string DataBaseName, int Port, int MaxConn, int close_log); 

private:
	connection_pool() = default;
	~connection_pool() = default;

	int m_MaxConn;  //最大连接数
	int m_CurConn;  //当前已使用的连接数
	int m_FreeConn; //当前空闲的连接数
	mutable locker lock;	//读取函数也需要访问锁
	// fixed number of MYSQL connection pool
	// 资源分配后应该在同一语句中将配置的资源交给handle对象
	std::list<std::shared_ptr<sql_connection> > connList; //连接池
	sem reserve;

public:
	std::string m_url;			 //主机地址
	std::string m_Port;		 //数据库端口号
	std::string m_User;		 //登陆数据库用户名
	std::string m_PassWord;	 //登陆数据库密码
	std::string m_DatabaseName; //使用数据库名
	int m_close_log;	//日志开关
};

class connection_pool_guard : noncopyable{
public:
	connection_pool_guard();
	~connection_pool_guard();
	void get_user_table(std::map<std::string, std::string>&) const;
	int query(const char*) const;

private:
	// pick sql_connection from connection_pool, must be shared_ptr
	// the smart_pointer has been deleted from connection_pool
	std::shared_ptr<sql_connection> con;
};

#endif
