#include <mysql/mysql.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <list>
#include <pthread.h>
#include <iostream>
#include "sql_connection_pool.h"

using namespace std;

connection_pool::connection_pool()
{
	m_CurConn = 0;
	m_FreeConn = 0;
}

// 实例的初始化放在函数内部，是单例模式中的懒汉模式（在第一次被使用才会进行初始化）
connection_pool *connection_pool::GetInstance()
{
	static connection_pool connPool;
	return &connPool;
}

//构造初始化
void connection_pool::init(string url, string User, string PassWord, string DBName, int Port, int MaxConn, int close_log)
{
	m_url = url;
	m_Port = Port;
	m_User = User;
	m_PassWord = PassWord;
	m_DatabaseName = DBName;
	m_close_log = close_log;

	for (int i = 0; i < MaxConn; i++)
	{
		MYSQL *con = NULL;
		con = mysql_init(con);

		if (con == NULL)
		{
			LOG_ERROR("MySQL Error");
			exit(1);
		}
		// 建立数据库连接
		con = mysql_real_connect(con, url.c_str(), User.c_str(), PassWord.c_str(), DBName.c_str(), Port, NULL, 0);

		if (con == NULL)
		{
			LOG_ERROR("MySQL Error");
			exit(1);
		}
		connList.push_back(con);
		++m_FreeConn;
	}

	reserve = sem(m_FreeConn);	// 将当前可使用的连接数量作为信号量的初始值

	m_MaxConn = m_FreeConn;
}


//当有请求时，从数据库连接池中返回一个可用连接，更新使用和空闲连接数
MYSQL *connection_pool::GetConnection()
{
	MYSQL *con = NULL;

	if (0 == connList.size())
		return NULL;

	reserve.wait();
	
	lock.lock();

	con = connList.front();
	connList.pop_front();

	--m_FreeConn;
	++m_CurConn;

	lock.unlock();
	return con;
}

//释放当前使用的连接
bool connection_pool::ReleaseConnection(MYSQL *con)
{
	if (NULL == con)
		return false;

	lock.lock();

	connList.push_back(con);	// 将当前连接存入连接池，表示已可用
	++m_FreeConn;
	--m_CurConn;

	lock.unlock();

	reserve.post();		// 通知正在等待连接的线程
	return true;
}

//销毁数据库连接池
void connection_pool::DestroyPool()
{

	lock.lock();
	if (connList.size() > 0)
	{
		list<MYSQL *>::iterator it;
		for (it = connList.begin(); it != connList.end(); ++it)
		{
			MYSQL *con = *it;
			mysql_close(con);
		}
		m_CurConn = 0;
		m_FreeConn = 0;
		connList.clear();
	}

	lock.unlock();
}

//当前空闲的连接数
int connection_pool::GetFreeConn()
{
	return this->m_FreeConn;
}

connection_pool::~connection_pool()
{
	DestroyPool();
}

// 通过构造函数从地址池里面取出一个数据库连接
connectionRAII::connectionRAII(MYSQL **SQL, connection_pool *connPool){
	*SQL = connPool->GetConnection();
	
	conRAII = *SQL;		// 两者指向同一个数据库连接（两个指针存放的地址相同）
	poolRAII = connPool;
}

// 通过析构函数释放当前数据库连接
connectionRAII::~connectionRAII(){
	poolRAII->ReleaseConnection(conRAII);
}

/*
 * 下面是对数据库进行操作的类
*/
// 查询并返回所有博客记录
vector<Blog> sql_blog_tool::select_all_blog(){
	connection_pool* connpool = connection_pool::GetInstance();

	vector<Blog> blogs;
	MYSQL* mysql = NULL;
	// 从数据库连接池中取出一个连接
	connectionRAII mysqlcon(&mysql, connpool);

	// 查询博客数据
	if(mysql_query(mysql, "SELECT blogId, title, content, userId, postTime FROM blog")){
		return {};
	}

	// 获取结果集
	MYSQL_RES* result = mysql_store_result(mysql);

	// 检查结果集是否为空
	if(!result){
		return {};
	}

	// 获取字段数量
	int num_fields = mysql_num_fields(result);

	// 从结果集中逐行获取数据
	while(MYSQL_ROW row = mysql_fetch_row(result)){
		Blog blog;
		blog.set_blog_id(stoi(row[0]));
		blog.set_blog_title(row[1]);
		blog.set_blog_content(row[2]);
		blog.set_user_id(stoi(row[3]));
		blog.set_blog_postTime(row[4]);

		// 将博客数据存入容器
		blogs.push_back(blog);
	}

	// 释放结果集
	mysql_free_result(result);
	return blogs;
}

/*
 *下面的内容为用户类
*/
void User::set_userid(int userid){
	this->m_userId = userid;
}

void User::set_username(string username){
	this->m_username = username;
}

void User::set_password(string password){
	this->m_password = password;
}

/*
 *下面的内容为博客类
*/
void Blog::set_blog_id(int blog_id){
	this->m_blog_id = blog_id;
}

int Blog::get_blog_id()
{
    return this->m_blog_id;
}

void Blog::set_blog_title(string blog_title){
	this->m_blog_title = blog_title;
}

string Blog::get_blog_title()
{
    return this->m_blog_title;
}

void Blog::set_blog_content(string content){
	this->m_bolg_content = content;
}

string Blog::get_blog_content()
{
    return this->m_bolg_content;
}

void Blog::set_user_id(int user_id){
	this->m_user_id = user_id;
}

int Blog::get_user_id()
{
    return this->m_user_id;
}

void Blog::set_blog_postTime(string blog_postTime){
	this->m_bolg_postTime = blog_postTime;
}

string Blog::get_blog_postTime()
{
    return this->m_bolg_postTime;
}
