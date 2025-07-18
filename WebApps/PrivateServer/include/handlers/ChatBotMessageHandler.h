#pragma once
#include "../../../../HttpServer/include/router/RouterHandler.h"
#include "../PrivateServer.h"
#include <curl/curl.h>

class ChatBotMessageHandler : public http::router::RouterHandler 
{
public:
    explicit ChatBotMessageHandler(PrivateServer* server) : server_(server) {
    }
    void handle(const http::HttpRequest& req, http::HttpResponse* resp) override;
private:
    std::string getchatgptResponse(const std::string& question, const std::string& model);
    std::string getbaiduResponse(const std::string& question, const std::string& model);
private:
    PrivateServer* server_;
    const std::unordered_map<std::string, std::string> apiKey_ = {
        // 输入api
        {"qianfan",""},
        {"gpt-3.5-turbo",""}
    };
    const std::unordered_map<std::string, std::string> apiUrl_ = {
        {"qianfan","https://qianfan.baidubce.com/v2/chat/completions"},
        {"gpt-3.5-turbo","https://api.openai.com/v1/chat/completions"}
    };
};