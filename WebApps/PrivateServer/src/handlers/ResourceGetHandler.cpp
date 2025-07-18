#include "../include/handlers/ResourceGetHandler.h"

void ResourceGetHandler::handle(const http::HttpRequest &req, http::HttpResponse *resp)
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
        std::string relativePath = req.getQueryParameters("path");
        if (relativePath.find("..") != std::string::npos) {
            json errorResp;
            errorResp["status"] = "error";
            errorResp["message"] = "Forbidden";
            std::string errorBody = errorResp.dump(4);

            server_->packageResp(req.getVersion(), http::HttpResponse::k403Forbidden,
                                "Forbidden", true, "application/json", errorBody.size(),
                                 errorBody, resp);
            return;
        }
        LOG_ERROR << relativePath;
        boost::filesystem::path realPath = "/home/li/file_use/file";
        realPath /= relativePath;
        if (!boost::filesystem::exists(realPath)) {
            LOG_ERROR << "not found resource";
            resp->setStatusLine(req.getVersion(), http::HttpResponse::k404NotFound, "Not Found");
            resp->setBody("Not Found");
            return;
        }
        std::string extension = realPath.extension().string();
        std::unordered_map<std::string, std::string> mimeTypes = {
            {".txt", "text/plain"},
            {".html", "text/html"},
            {".css", "text/css"},
            {".js", "application/javascript"},
            {".json", "application/json"},
            {".png", "image/png"},
            {".jpg", "image/jpeg"},
            {".jpeg", "image/jpeg"},
            {".gif", "image/gif"},
            {".pdf", "application/pdf"},
            {".zip", "application/zip"}
        };
        std::string mimeType = mimeTypes.count(extension) ? mimeTypes[extension] : "application/octet-stream";
        auto isTextType = [](const std::string& type) {
            return type.find("text/") == 0 || 
                type == "application/json" || 
                type == "application/javascript";
        };
        if (isTextType(mimeType)) {
            mimeType += "; charset=utf-8";
        }
        LOG_ERROR << "success found resource";
        std::ifstream in(realPath.string(), std::ios::binary);
        std::ostringstream ss;
        ss << in.rdbuf();
        std::string content = ss.str();
        resp->setStatusLine(req.getVersion(), http::HttpResponse::k200Ok, "OK");
        resp->setCloseConnection(false);
        resp->setContentType(mimeType);
        resp->setContentLength(content.size());
        resp->setBody(content);
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
