#include "../include/handlers/DownloadHandler.h"
#include <boost/filesystem.hpp>
#include <fstream>

std::string getMimeType(const std::string& filename)
{
    static const std::unordered_map<std::string, std::string> mimeTypes = {
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
        {".mp4", "video/mp4"},
        // 默认类型
        {"", "application/octet-stream"}
    };

    std::string ext = boost::filesystem::extension(filename);
    auto it = mimeTypes.find(ext);
    return it != mimeTypes.end() ? it->second : "application/octet-stream";
}

void DownloadHandler::handle(const http::HttpRequest& req, http::HttpResponse* resp)
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
        // 获取用户信息
        int userId = std::stoi(session->getValue("userId"));
        std::string username = session->getValue("username");
        std::string relativePath = req.path().substr(std::string("/download/").size());; // 去掉 /download/
        std::string absolutePath = "/home/li/file_use/file/" + username + "/" + relativePath;
        LOG_ERROR << absolutePath;
        if (!boost::filesystem::exists(absolutePath) || !boost::filesystem::is_regular_file(absolutePath)) {
            resp->setStatusLine(req.getVersion(), http::HttpResponse::k404NotFound, "Not Found");
            resp->setCloseConnection(false);
            return;
        }

        std::ifstream file(absolutePath, std::ios::binary | std::ios::ate);
        if (!file) {
            resp->setStatusLine(req.getVersion(), http::HttpResponse::k500InternalServerError, "File open error");
            resp->setCloseConnection(false);
            return;
        }

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        std::string content(size, '\0');
        if (!file.read(&content[0], size)) {
            resp->setStatusLine(req.getVersion(), http::HttpResponse::k500InternalServerError, "File read error");
            resp->setCloseConnection(false);
            return;
        }
        std::string filename = boost::filesystem::path(relativePath).filename().string();
        resp->setStatusLine(req.getVersion(), http::HttpResponse::k200Ok, "OK");
        resp->setContentLength(size);
        resp->setCloseConnection(false);
        resp->addHeader("Content-Disposition", "attachment; filename=\"" + filename + "\"");
        resp->setContentType(getMimeType(filename));
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


// void DownloadHandler::handle(const http::HttpRequest& req, http::HttpResponse* resp)
// {
//     auto session = server_->getSessionManager()->getSession(req, resp);
//     std::string username = session->getValue("username");
//     LOG_INFO << "username:" << username;
//     LOG_INFO << "path:" << req.path();

//     if (username.empty()) {
//         resp->setStatusLine("HTTP/1.1", http::HttpResponse::k401Unauthorized, "Unauthorized");
//         resp->setContentType("text/plain");
//         resp->setBody("未登录");
//         return;
//     }

//     std::string fileRelPath = req.path().substr(std::string("/download/").size());
//     std::string fullPath = "/home/li/file_use/file/" + username + "/" + fileRelPath;
//     LOG_INFO << "fullpath:" << fullPath;
//     std::ifstream in(fullPath, std::ios::binary);
//     if (!in) {
//         resp->setStatusLine("HTTP/1.1", http::HttpResponse::k404NotFound, "Not Found");
//         resp->setBody("文件未找到");
//         return;
//     }

//     std::ostringstream oss;
//     oss << in.rdbuf();
//     std::string fileData = oss.str();

//     // 设置下载响应头
//     resp->setStatusLine("HTTP/1.1", http::HttpResponse::k200Ok, "OK");
//     resp->setContentType("application/octet-stream");
//     resp->addHeader("Content-Disposition", "attachment; filename=\"" + fileRelPath + "\"");
//     resp->setContentLength(fileData.size());
//     resp->setBody(fileData);
//     LOG_INFO << "setting";
// }
