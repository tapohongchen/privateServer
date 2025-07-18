#include "../include/handlers/CreateDirHandler.h"

void CreateDirHandler::handle(const http::HttpRequest &req, http::HttpResponse *resp)
{
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
        json requestJson = json::parse(req.getBody());
        std::string folderName = requestJson.value("folderName", "");
        std::string relativePath = requestJson.value("path", "");
        if (folderName.empty()) {
            json res = {{"success", false}, {"message", "文件夹名不能为空"}};
            std::string body = res.dump(4);
            resp->setStatusLine(req.getVersion(), http::HttpResponse::k400BadRequest, "Bad Request");
            resp->setCloseConnection(false);
            resp->setContentType("application/json");
            resp->setContentLength(body.size());
            resp->setBody(body);
            return;
        }
        std::string basePath = "/home/li/file_use/file/" + session->getValue("username");
        boost::filesystem::path fullPath = basePath;
        if (!relativePath.empty())
            fullPath /= relativePath;
        fullPath /= folderName;
        if (boost::filesystem::exists(fullPath)) {
            json res = {{"success", false}, {"message", "文件夹已存在"}};
            std::string body = res.dump(4);
            resp->setStatusLine(req.getVersion(), http::HttpResponse::k409Conflict, "Conflict");
            resp->setCloseConnection(false);
            resp->setContentType("application/json");
            resp->setContentLength(body.size());
            resp->setBody(body);
            return;
        }
        boost::filesystem::create_directories(fullPath);
        json res = {{"success", true}, {"message", "文件夹创建成功"}};
        std::string body = res.dump(4);
        resp->setStatusLine(req.getVersion(), http::HttpResponse::k200Ok, "OK");
        resp->setCloseConnection(false);
        resp->setContentType("application/json");
        resp->setContentLength(body.size());
        resp->setBody(body);
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
