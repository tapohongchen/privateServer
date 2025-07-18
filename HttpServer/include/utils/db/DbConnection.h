#pragma once
#include <memory>
#include <string>
#include <mutex>
#include <cppconn/connection.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <mysql_driver.h>
#include <mysql/mysql.h>
#include <muduo/base/Logging.h>
#include "DbException.h"

namespace http 
{
namespace db 
{

class DbConnection 
{
public:
    DbConnection(const std::string& host, 
                const std::string& user,
                const std::string& password,
                const std::string& database);
    ~DbConnection();

    // 禁止拷贝
    DbConnection(const DbConnection&) = delete;
    DbConnection& operator=(const DbConnection&) = delete;

    bool isValid();
    void reconnect();
    void cleanup();

    // sql::ResultSet:SQL 查询结果集:遍历查询结果,按列名或索引获取数据,获取结果集元数据
    template<typename... Args>
    sql::ResultSet* executeQuery(const std::string& sql, Args&&... args)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        try 
        {
            // 直接创建新的预处理语句，不使用缓存
            std::unique_ptr<sql::PreparedStatement> stmt(
                conn_->prepareStatement(sql)
            );
            // 
            bindParams(stmt.get(), 1, std::forward<Args>(args)...);
            return stmt->executeQuery();
        } 
        catch (const sql::SQLException& e) 
        {
            LOG_ERROR << "Query failed: " << e.what() << ", SQL: " << sql;
            throw DbException(e.what());
        }
    }
    
    template<typename... Args>
    int executeUpdate(const std::string& sql, Args&&... args)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        try 
        {
            // 直接创建新的预处理语句，不使用缓存
            // sql::PreparedStatement：预处理语句对象，绑定参数并执行sql
            // prepareStatement(sql)：创建一个预处理语句，可绑定参数并执行SQL，支持占位符：“？”
            // prepareStatement可防止sql攻击
            // sql注入攻击，假设有个登录sql如下：
            //std::string sql = "SELECT * FROM users WHERE username = '" + username + "' AND password = '" + password + "'";
            // 当用户输入password: ' OR '1'='1时，这样语句如下：
            //SELECT * FROM users WHERE username = 'admin' AND password = '' OR '1'='1'
            //此时语句恒成立，就绕过密码登录成功了
            //且PreparedStatement语句在服务器端编译一次，多次执行更快，而statement每次都要重新解析、编译 SQL 语句
            std::unique_ptr<sql::PreparedStatement> stmt(
                conn_->prepareStatement(sql)
            );
            //递归参数绑定，用于更换sql语句中占位符？
            bindParams(stmt.get(), 1, std::forward<Args>(args)...);
            //执行更新操作，返回受影响行数
            return stmt->executeUpdate();
        } 
        catch (const sql::SQLException& e) 
        {
            LOG_ERROR << "Update failed: " << e.what() << ", SQL: " << sql;
            throw DbException(e.what());
        }
    }

    bool ping();  // 添加检测连接是否有效的方法
private:
     // 辅助函数：递归终止条件
    void bindParams(sql::PreparedStatement*, int) {}
    
    // 辅助函数：绑定参数
    template<typename T, typename... Args>
    void bindParams(sql::PreparedStatement* stmt, int index, 
                   T&& value, Args&&... args) 
    {
        stmt->setString(index, std::to_string(std::forward<T>(value)));
        bindParams(stmt, index + 1, std::forward<Args>(args)...);
    }
    
    // 特化 string 类型的参数绑定
    template<typename... Args>
    void bindParams(sql::PreparedStatement* stmt, int index, 
                   const std::string& value, Args&&... args) 
    {
        stmt->setString(index, value);
        bindParams(stmt, index + 1, std::forward<Args>(args)...);
    }

private:
    // sql::Connection:数据库连接对象，（创建 SQL 语句对象，管理事务，设置连接属性，选择数据库）
    std::shared_ptr<sql::Connection> conn_;
    std::string                      host_;
    std::string                      user_;
    std::string                      password_;
    std::string                      database_;
    std::mutex                       mutex_;
};

} // namespace db
} // namespace http