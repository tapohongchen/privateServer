#include "../include/handlers/PreviewHandler.h"

void PreviewHandler::handle(const http::HttpRequest& req, http::HttpResponse* resp)
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

        // 获取用户信息
    try
    {
        json parsed = json::parse(req.getBody());
        std::string filename = parsed["filename"];
        std::string userId = server_->getSessionManager()->getSession(req, resp)->getValue("userId");

        if (filename.empty() || userId.empty())
        {
            json errorResp;
            errorResp["success"] = "error";
            errorResp["message"] = "Missing filename or userId";
            std::string body = errorResp.dump(4);
            resp->setStatusLine(req.getVersion(), http::HttpResponse::k400BadRequest, "Bad Request");
            resp->setCloseConnection(true);
            resp->setContentType("application/json");
            resp->setBody(body);
            resp->setContentLength(body.size());
            return;
        }

        // 文件路径：建议统一放入某个静态文件目录，例如 /static/user123/files/xxx.txt
        std::string fileName = parsed.value("filename", "");
        std::string relativePath = parsed.value("path", "");
        if (fileName.empty()) {
            json res = {{"success", false}, {"message", "文件名不能为空"}};
            std::string body = res.dump(4);
            resp->setStatusLine(req.getVersion(), http::HttpResponse::k400BadRequest, "Bad Request");
            resp->setCloseConnection(false);
            resp->setContentType("application/json");
            resp->setContentLength(body.size());
            resp->setBody(body);
            return;
        }
        std::string userName = session->getValue("username");
        std::string basePath = "/home/li/file_use/file/" + userName;
        boost::filesystem::path fullPath = basePath;
        if (!relativePath.empty())
            fullPath /= relativePath;
        fullPath /= fileName;

        if (!boost::filesystem::exists(fullPath))
        {
            json res = {{"success", false}, {"message", "文件未找到"}};
            std::string body = res.dump(4);
            resp->setStatusLine(req.getVersion(), http::HttpResponse::k404NotFound, "Not Found");
            resp->setCloseConnection(false);
            resp->setContentType("application/json");
            resp->setBody(body);
            resp->setContentLength(body.size());
            return;
        }

        // MIME 类型识别（这里只是示例，可自行扩展）
        std::string extension = "bin";
        auto dotPos = fileName.find_last_of('.');
        if (dotPos != std::string::npos && dotPos + 1 < fileName.size())
            extension = fileName.substr(dotPos + 1);
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
            {".zip", "application/zip"},
            {".mp4", "video/mp4"}
        };

        std::string mimeType = mimeTypes.count(extension) ? mimeTypes[extension] : "application/octet-stream";

        // 构造预览URL
        size_t pos = 0;
        while(pos < relativePath.size() && relativePath[pos] == '/'){
            ++pos;
        }
        relativePath = "/" + relativePath.substr(pos);
        std::string previewUrl = "/static?path=" + userName + relativePath + "/" + filename;

        json successResp;
        successResp["success"] = true;
        successResp["url"] = previewUrl;
        successResp["type"] = mimeType;

        std::string body = successResp.dump(4);
        resp->setStatusLine(req.getVersion(), http::HttpResponse::k200Ok, "OK");
        resp->setCloseConnection(false);
        resp->setContentType("application/json");
        resp->setContentLength(body.size());
        resp->setBody(body);
    }
    catch (const std::exception &e)
    {
        json failureResp;
        failureResp["success"] = false;
        failureResp["error"] = e.what();
        std::string body = failureResp.dump(4);

        resp->setStatusLine(req.getVersion(), http::HttpResponse::k400BadRequest, "Bad Request");
        resp->setCloseConnection(true);
        resp->setContentType("application/json");
        resp->setContentLength(body.size());
        resp->setBody(body);
    }
}
