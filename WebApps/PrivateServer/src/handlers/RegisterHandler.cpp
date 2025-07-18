#include "../include/handlers/RegisterHandler.h"

void RegisterHandler::handle(const http::HttpRequest& req, http::HttpResponse* resp)
{
    // 解析body(json格式)
    json parsed = json::parse(req.getBody());
    std::string username = parsed["username"];
    std::string password = parsed["password"];

    // 判断用户是否已经存在，如果存在则注册失败
    int userId = insertUser(username, password);
    if (userId != -1)
    {
        // 插入成功
        // 判断名字是否合法
        if(isValid(username)){
            // 创建用户目录
            std::string userRootDir = "/home/li/file_use/file/" + username;
            LOG_ERROR << userRootDir;
            boost::filesystem::path usrdir(userRootDir);
            // 判断目录是否存在，不存在则创建
            if (!boost::filesystem::exists(usrdir))
            {
                try{
                    boost::filesystem::create_directories(usrdir);
                    LOG_ERROR << "user:" << username << " 创建成功：" << userRootDir;
                    boost::filesystem::path infofile = usrdir/"introduce.txt";
                    std::ofstream out(infofile.string());
                    if (out.is_open()) {
                        out << "该服务器包含 AI 聊天、文件上传下载 和 五子棋对战 三个功能。\n";
                        out.close();
                        
                    } else {
                        LOG_ERROR << "无法创建说明文件: " << infofile.string();
                    }
                }
                catch (const boost::filesystem::filesystem_error & e){
                    LOG_ERROR << "创建失败：" << e.what();
                    json failureResp;
                    failureResp["status"] = "error";
                    failureResp["message"] = "username is invalid";
                    std::string failureBody = failureResp.dump(4);

                    resp->setStatusLine(req.getVersion(), http::HttpResponse::k409Conflict, "Conflict");
                    resp->setCloseConnection(false);
                    resp->setContentType("application/json");
                    resp->setContentLength(failureBody.size());
                    resp->setBody(failureBody);
                }
            }
        }

        // 封装成功响应
        json successResp;   
        successResp["status"] = "success";
        successResp["message"] = "Register successful";
        successResp["userId"] = userId;
        std::string successBody = successResp.dump(4);

        resp->setStatusLine(req.getVersion(), http::HttpResponse::k200Ok, "OK");
        resp->setCloseConnection(false);
        resp->setContentType("application/json");
        resp->setContentLength(successBody.size());
        resp->setBody(successBody);
    }
    else
    {
        // 插入失败
        json failureResp;
        failureResp["status"] = "error";
        failureResp["message"] = "username already exists";
        std::string failureBody = failureResp.dump(4);

        resp->setStatusLine(req.getVersion(), http::HttpResponse::k409Conflict, "Conflict");
        resp->setCloseConnection(false);
        resp->setContentType("application/json");
        resp->setContentLength(failureBody.size());
        resp->setBody(failureBody);
    }
}

int RegisterHandler::insertUser(const std::string &username, const std::string &password)
{
    // 判断用户是否存在，如果存在则返回-1，否则返回用户id
    if (!isUserExist(username))
    {
        // 用户不存在，插入用户
        std::string sql = "INSERT INTO users (username, password) VALUES ('" + username + "', '" + password + "')";
        mysqlUtil_.executeUpdate(sql);
        std::string sql2 = "SELECT id FROM users WHERE username = '" + username + "'";
        sql::ResultSet* res = mysqlUtil_.executeQuery(sql2);
        if (res->next())
        {
            return res->getInt("id");
        }
    }
    return -1;
}

bool RegisterHandler::isUserExist(const std::string &username)
{
    std::string sql = "SELECT id FROM users WHERE username = '" + username + "'";
    sql::ResultSet* res = mysqlUtil_.executeQuery(sql);
    if (res->next())
    {
        return true;
    }
    return false;
}

bool RegisterHandler::isValid(const std::string& username)
{
    return std::regex_match(username, std::regex("^[a-zA-Z0-9_]{2,16}$"));
}