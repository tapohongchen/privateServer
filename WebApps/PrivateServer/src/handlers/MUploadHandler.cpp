#include "../include/handlers/MUploadHandler.h"

void MUploadHandler::handle(const http::HttpRequest& req, http::HttpResponse* resp)
{
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
        std::string username = session->getValue("username");
        json fileIntro = json::parse(req.getBody());
        std::string fileName = fileIntro["filename"];
        std::string relaPath = fileIntro["path"];
        size_t pos = 0;
        while(pos < relaPath.size() && relaPath[pos] == '/'){
            ++pos;
        }
        relaPath = "/" + relaPath.substr(pos);
        std::string basename = fileName;
        size_t dotPos = basename.find_last_of('.');
        if(dotPos != std::string::npos){
            basename = basename.substr(0, dotPos);
        }
        size_t fileSize = fileIntro["filesize"];
        LOG_ERROR << "filesize ok\n";
        size_t chunkSize = 100 * 1024 * 1024;
        size_t chunkCount = (fileSize + chunkSize - 1) / chunkSize;
        int userId = std::stoi(session->getValue("userId"));
        std::string tempDir = "/home/li/tmp/uploads/" + username + "/" +basename;
        LOG_ERROR << "tempdir ok\n";
        boost::filesystem::create_directories(tempDir);
        LOG_ERROR << "temp dir:" << tempDir;
        
        server_->uploadSessions_[username] = PrivateServer::UploadSession{
            relaPath, fileName, chunkCount, chunkSize, std::vector<bool>(chunkCount, false), tempDir
        };
        std::string uploadId = username + "_" + std::to_string(time(nullptr));
        json ok = {
            {"uploadID", uploadId},
            {"chunkSize", chunkSize},
            {"chunkCount", chunkCount}
        };
        LOG_ERROR << "json ok\n";
        std::string okBody = ok.dump(4);
        resp->setStatusLine(req.getVersion(), http::HttpResponse::k200Ok, "OK");
        resp->setCloseConnection(false);
        resp->setContentType("application/json");
        resp->setContentLength(okBody.size());
        resp->setBody(okBody);
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