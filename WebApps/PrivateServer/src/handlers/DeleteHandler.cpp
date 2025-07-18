#include "../include/handlers/DeleteHandler.h"

void DeleteHandler::handle(const http::HttpRequest& req, http::HttpResponse* resp){
        // JSON 解析使用 try catch 捕获异常
    try
    {
        // 检查用户是否已登录
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
        std::string username = session->getValue("username");
        json requestJson = json::parse(req.getBody());
        bool isFolder = requestJson["isFolder"];
        std::string path = requestJson["path"];
        std::string target = requestJson["target"];
        std::string fullPath = "/home/li/file_use/file/" + username;
        if(!path.empty()){
            fullPath += "/" + path;
        }
        if(!target.empty()){
            fullPath += "/" + target;
        }
        if(!boost::filesystem::exists(fullPath)){
            json error = {{"status", "error"}, {"message", "目标不存在"}};
            std::string body = error.dump(4);
            resp->setStatusLine(req.getVersion(), http::HttpResponse::k400BadRequest, "Not Found");
            resp->setCloseConnection(false);
            resp->setContentType("application/json");
            resp->setContentLength(body.size());
            resp->setBody(body);
        }
        bool success = false;
        if(isFolder){
            success = boost::filesystem::remove_all(fullPath) > 0;
        }
        else{
            success = boost::filesystem::remove(fullPath);
        }
        if(success){
            json ok = {{"success", true}};
            std::string body = ok.dump(4);
            resp->setStatusLine(req.getVersion(), http::HttpResponse::k200Ok, "ok");
            resp->setCloseConnection(false);
            resp->setContentType("application/json");
            resp->setContentLength(body.size());
            resp->setBody(body);
        }
        else{
            json err = {{"status", "error"}, {"message", "删除失败"}};
            std::string body = err.dump(4);
            resp->setStatusLine(req.getVersion(), http::HttpResponse::k500InternalServerError, "ok");
            resp->setCloseConnection(false);
            resp->setContentType("application/json");
            resp->setContentLength(body.size());
            resp->setBody(body);
        }
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