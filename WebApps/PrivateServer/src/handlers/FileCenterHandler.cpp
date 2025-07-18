#include "../include/handlers/FileCenterHandler.h"
#include <boost/filesystem.hpp>

void FileCenterHandler::handle(const http::HttpRequest& req, http::HttpResponse* resp)
{
    // 获取 session 中的用户名
    auto session = server_->getSessionManager()->getSession(req, resp);
    std::string username = session->getValue("username");

    if (username.empty())
    {
        resp->setStatusLine(req.getVersion(), http::HttpResponse::k401Unauthorized, "Unauthorized");
        json failureResp = {{"success", false}, {"message", "用户未登录"}};
        std::string body = failureResp.dump(4);
        resp->setCloseConnection(false);
        resp->setContentType("application/json");
        resp->setContentLength(body.size());
        resp->setBody(body);
        return;
    }

    // 获取查询参数中的路径 path
    std::string pathParam = req.getQueryParameters("path"); // e.g., path=文件夹1/子文件夹2
    std::string userRoot = "/home/li/file_use/file/" + username;

    std::string absolutePath = userRoot;
    if (!pathParam.empty()) {
        absolutePath += "/" + pathParam;
    }

    json response;
    std::vector<json> items;

    boost::filesystem::path dirPath(absolutePath);
    if (!boost::filesystem::exists(dirPath) || !boost::filesystem::is_directory(dirPath))
    {
        response["success"] = false;
        response["message"] = "路径不存在或不是目录";
    }
    else
    {
        for (const auto& entry : boost::filesystem::directory_iterator(dirPath))
        {
            json item;
            item["name"] = entry.path().filename().string();
            if (boost::filesystem::is_directory(entry))
                item["type"] = "folder";
            else if (boost::filesystem::is_regular_file(entry))
                item["type"] = "file";
            else
                continue;

            items.push_back(item);
        }

        response["success"] = true;
        response["items"] = items;
    }

    std::string body = response.dump(4);
    resp->setStatusLine(req.getVersion(), http::HttpResponse::k200Ok, "OK");
    resp->setCloseConnection(false);
    resp->setContentType("application/json");
    resp->setContentLength(body.size());
    resp->setBody(body);
}
