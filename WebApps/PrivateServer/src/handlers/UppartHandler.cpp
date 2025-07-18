#include "../include/handlers/UppartHandler.h"

void UppartHandler::handle(const http::HttpRequest& req, http::HttpResponse* resp)
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
        auto it = server_->PrivateServer::uploadSessions_.find(username);
        if(it == server_->PrivateServer::uploadSessions_.end()){
            json error = {{"status", "error"}, {"message", "上传回话不存在"}};
            std::string body = error.dump(4);
            resp->setStatusLine(req.getVersion(), http::HttpResponse::k400BadRequest, "Not Found");
            resp->setCloseConnection(false);
            resp->setContentType("application/json");
            resp->setContentLength(body.size());
            resp->setBody(body);
            return;
        }
        PrivateServer::UploadSession& status = it->second;
        const std::string& chunkIdx = req.getHeader("chunkIndex");
        if(chunkIdx.empty()){
            json error = {{"status", "error"}, {"message", "缺少分片序号"}};
            std::string body = error.dump(4);
            resp->setStatusLine(req.getVersion(), http::HttpResponse::k400BadRequest, "Not Found");
            resp->setCloseConnection(false);
            resp->setContentType("application/json");
            resp->setContentLength(body.size());
            resp->setBody(body);
            return;
        }
        size_t chindex = std::stoi(chunkIdx);
        if(chindex >= status.chunkCount){
            json error = {{"status", "error"}, {"message", "分片序号超出范围"}};
            std::string body = error.dump(4);
            resp->setStatusLine(req.getVersion(), http::HttpResponse::k400BadRequest, "Not Found");
            resp->setCloseConnection(false);
            resp->setContentType("application/json");
            resp->setContentLength(body.size());
            resp->setBody(body);
            return;
        }
        std::string chunkPath = status.tempdir + "/" + chunkIdx;
        std::ofstream ofs(chunkPath, std::ios::binary);
        ofs.write(req.getBody().data(), req.getBody().size());
        {
            std::lock_guard<std::mutex> lock(server_->uploadMutex_);
            status.receivedChunks[chindex] = true;
        }
        json okRsep = {
            {"success", "true"}
        };
        std::string okBody = okRsep.dump(4);
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
