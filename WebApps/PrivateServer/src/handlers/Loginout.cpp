#include "../include/handlers/LogoutHandler.h"

void LogoutHandler::handle(const http::HttpRequest &req, http::HttpResponse *resp)
{
    // JSON 解析使用 try catch 捕获异常
    try
    {
        auto session = server_->getSessionManager()->getSession(req, resp);
        LOG_INFO << "session->getValue(\"isLoggedIn\") = " << session->getValue("isLoggedIn");
        if (session->getValue("isLoggedIn") != "true")
        {
            // 用户未登录，返回未授权错误
            json errorResp;
            errorResp["status"] = "error";
            errorResp["message"] = "Unauthorized";
            std::string errorBody = errorResp.dump(4);

            server_->packageResp(req.getVersion(), http::HttpResponse::k401Unauthorized,
                                "Unauthorized", true, "application/json", errorBody.size(),
                                    errorBody, resp);
            return;
        }
        // 获取用户id
        int userId = std::stoi(session->getValue("userId"));
        // 清除会话数据
        session->clear();
        // 销毁会话
        server_->getSessionManager()->destroySession(session->getId());
        if(server_->aiGames_.find(userId) != server_->aiGames_.end()){
            LOG_ERROR << server_->onlineUsers_[userId];
            auto contentType = req.getHeader("Content-Type");
            if (contentType.empty() || contentType != "application/json" || req.getBody().empty())
            {
                resp->setStatusLine(req.getVersion(), http::HttpResponse::k400BadRequest, "Bad Request");
                resp->setCloseConnection(true);
                resp->setContentType("application/json");
                resp->setContentLength(0);
                resp->setBody("");
                return;
            }
            json parsed = json::parse(req.getBody());
            int gameType = parsed["gameType"]; // fixme: 以后也换成从会话中获取
            
            {   // 释放资源
                std::lock_guard<std::mutex> lock(server_->mutexForOnlineUsers_);
                server_->onlineUsers_.erase(userId);
            }

            if (gameType == PrivateServer::MAN_VS_AI)
            {
                std::lock_guard<std::mutex> lock(server_->mutexForAiGames_);
                server_->aiGames_.erase(userId);
            }
            else if (gameType == PrivateServer::MAN_VS_MAN)
            {
                // 释放相应创造资源，并且通知另一个用户对方已经主动退出游戏
            }
        }
        server_->onlineUsers_[userId] = false;
        // 返回响应报文
        json response;
        response["message"] = "logout successful";
        std::string responseBody = response.dump(4);
        resp->setStatusLine(req.getVersion(), http::HttpResponse::k200Ok, "OK");
        resp->setCloseConnection(true);
        resp->setContentType("application/json");
        resp->setContentLength(responseBody.size());
        resp->setBody(responseBody);
    }
    catch (const std::exception &e)
    {
        // 捕获异常，返回错误信息
        json failureResp;
        failureResp["status"] = "error";
        failureResp["message"] = e.what();
        std::string failureBody = failureResp.dump(4);
        resp->setStatusLine(req.getVersion(), http::HttpResponse::k400BadRequest, "Bad Request");
        resp->setCloseConnection(true);
        resp->setContentType("application/json");
        resp->setContentLength(failureBody.size());
        resp->setBody(failureBody);
    }
}