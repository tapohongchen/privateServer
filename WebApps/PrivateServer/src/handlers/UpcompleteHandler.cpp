#include "../include/handlers/UpcompleteHandler.h"
#include "../include/handlers/Codingtoutf8.h"

void UpcompleteHandler::handle(const http::HttpRequest& req, http::HttpResponse* resp)
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
        std::vector<int> missingChunks;
        for(int i=0; i<status.chunkCount; i++){
            if(!status.receivedChunks[i]){
                missingChunks.push_back(static_cast<int>(i));
            }
        }
        if(!missingChunks.empty()){
            json error = {{"success", "false"}, {"message", "尚有未上传的分片"}, {"missingchuke", missingChunks}};
            std::string body = error.dump(4);
            resp->setStatusLine(req.getVersion(), http::HttpResponse::k400BadRequest, "Not Found");
            resp->setCloseConnection(false);
            resp->setContentType("application/json");
            resp->setContentLength(body.size());
            resp->setBody(body);
            return;
        }
        std::string final_path = "/home/li/file_use/file/" + username + status.realativePath + "/" + status.filename;
        LOG_ERROR << final_path;
        std::ofstream output(final_path, std::ios::binary);
        for(size_t i=0; i < status.chunkCount; i++){
            std::string chunkpath = status.tempdir + "/" + std::to_string(i);
            std::ifstream input(chunkpath, std::ios::binary);
            output<<input.rdbuf();
        }
        output.close();
        // 2. 如果是文本文件，则检测编码并转为 UTF-8
        if (isTextFile(status.filename)) {
            std::ifstream in(final_path, std::ios::binary);
            std::ostringstream ss;
            ss << in.rdbuf();
            std::string content = ss.str();
            in.close();

            std::string encoding = detectEncoding(content);
            if (!encoding.empty() && encoding != "UTF-8") {
                std::string utf8Content = convertToUtf8(content, encoding);
                if (!utf8Content.empty()) {
                    std::ofstream out(final_path, std::ios::binary | std::ios::trunc);
                    out.write(utf8Content.data(), utf8Content.size());
                    out.close();
                    LOG_INFO << "文件转码成功：" << encoding << " -> UTF-8";
                }
            }
        }
        boost::filesystem::remove_all(status.tempdir);
        server_->uploadSessions_.erase(username);
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
