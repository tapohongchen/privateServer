#include "../include/handlers/ChatBotMessageHandler.h"
#include <curl/curl.h>
#include <iostream> // 添加日志输出

using json = nlohmann::json;

// 回调函数用于处理CURL的响应数据
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

void ChatBotMessageHandler::handle(const http::HttpRequest &req, http::HttpResponse *resp) {
    // 1. 验证请求内容类型
    auto contentType = req.getHeader("Content-Type");
    if (contentType != "application/json" || req.getBody().empty()) {
        json errorResp;
        errorResp["status"] = "error";
        errorResp["message"] = "Invalid content type or empty body";
        
        resp->setStatusLine(req.getVersion(), http::HttpResponse::k400BadRequest, "Bad Request");
        resp->setContentType("application/json");
        resp->setBody(errorResp.dump(4));
        return;
    }

    try {
        // 2. 解析JSON请求体
        json parsed = json::parse(req.getBody());
        std::string question = parsed["question"];
        std::string model = parsed["model"];
        
        // 3. 验证用户会话
        auto session = server_->getSessionManager()->getSession(req, resp);
        if (!session || session->getValue("isLoggedIn") != "true") {
            json errorResp;
            errorResp["status"] = "error";
            errorResp["message"] = "User not logged in";
            
            resp->setStatusLine(req.getVersion(), http::HttpResponse::k401Unauthorized, "Unauthorized");
            resp->setContentType("application/json");
            resp->setBody(errorResp.dump(4));
            return;
        }
        
        std::string response;
        // 4. 调用AI聊天服务
        if(model == "gpt-3.5-turbo"){
            response = getchatgptResponse(question, model);
        }
        else if(model == "qianfan"){
            response = getbaiduResponse(question, model);
        }    
        else{
            json errorResp;
            errorResp["status"] = "error";
            errorResp["message"] = "unsupport model";
            
            resp->setStatusLine(req.getVersion(), http::HttpResponse::k400BadRequest, "Bad Request");
            resp->setContentType("application/json");
            resp->setBody(errorResp.dump(4));
            return;
        }
        
        // 5. 返回成功响应
        json successResp;
        successResp["status"] = "success";
        successResp["response"] = response;
        
        resp->setStatusLine(req.getVersion(), http::HttpResponse::k200Ok, "OK");
        resp->setContentType("application/json");
        resp->setBody(successResp.dump(4));
    }
    catch (const std::exception &e) {
        // 输出详细错误信息到日志
        std::cerr << "ChatBotMessageHandler error: " << e.what() << std::endl;
        
        // 6. 异常处理
        json errorResp;
        errorResp["status"] = "error";
        errorResp["message"] = "Internal server error";
        
        resp->setStatusLine(req.getVersion(), http::HttpResponse::k500InternalServerError, "Internal Server Error");
        resp->setContentType("application/json");
        resp->setBody(errorResp.dump(4));
    }
}

std::string ChatBotMessageHandler::getchatgptResponse(const std::string& question, const std::string& model) {
    CURL* curl = curl_easy_init();
    std::string readBuffer;
    
    if (!curl) {
        throw std::runtime_error("Failed to initialize CURL");
    }

    try {
        // 1. 设置请求头
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, ("Authorization: Bearer " + apiKey_.at(model)).c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");
        
        // 2. 构建请求体
        json requestBody = {
            {"model", model},
            {"messages", {{{"role", "user"}, {"content", question}}}}
        };
        std::string data = requestBody.dump();
        
        // 3. 配置CURL选项
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_URL, apiUrl_.at(model).c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30); // 30秒超时
        
        // 4. 执行请求
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            throw std::runtime_error("CURL request failed: " + 
                                     std::string(curl_easy_strerror(res)));
        }
        
        // 输出原始响应用于调试
        std::cerr << "OpenAI API response: " << readBuffer << std::endl;
        
        // 5. 解析响应
        json jsonResponse = json::parse(readBuffer);
        if (jsonResponse.contains("choices") && 
            !jsonResponse["choices"].empty() &&
            jsonResponse["choices"][0].contains("message") &&
            jsonResponse["choices"][0]["message"].contains("content")) {
            
            return jsonResponse["choices"][0]["message"]["content"];
        }
        
        throw std::runtime_error("Invalid API response: " + jsonResponse.dump());
    }
    catch (const std::exception& e) {
        // 6. 清理资源并重新抛出异常
        curl_easy_cleanup(curl);
        throw;
    }
    
    // 7. 清理资源
    curl_easy_cleanup(curl);
    return "";
}

std::string ChatBotMessageHandler::getbaiduResponse(const std::string& question, const std::string& model)
{
    CURL* curl = curl_easy_init();
    std::string readBuffer;

    if (!curl) {
        throw std::runtime_error("Failed to initialize CURL");
    }

    try {
        // 百度API请求体结构
        json requestBody = {
            {"model", "ernie-3.5-8k"},
            {"messages", {
                {
                    {"role", "user"},
                    {"content", question}
                }
            }}
        };

        std::string data = requestBody.dump();

        struct curl_slist* headers = nullptr;
        // 只设置必要的头
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, ("Authorization: Bearer " + apiKey_.at(model)).c_str());

        // 禁用SSL验证
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        
        curl_easy_setopt(curl, CURLOPT_URL, apiUrl_.at(model).c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30); // 增加超时时间

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            throw std::runtime_error("CURL request failed: " + std::string(curl_easy_strerror(res)));
        }
        
        // 输出原始响应用于调试
        std::cerr << "Baidu API response: " << readBuffer << std::endl;

        // 解析百度API响应
        json jsonResponse = json::parse(readBuffer);
        
        // 百度文心一言的响应格式
        if (jsonResponse.contains("result")) {
            return jsonResponse["result"].get<std::string>();
        }
        // 兼容其他可能的格式
        else if (jsonResponse.contains("choices") && 
                 !jsonResponse["choices"].empty() &&
                 jsonResponse["choices"][0].contains("message") &&
                 jsonResponse["choices"][0]["message"].contains("content")) {
            return jsonResponse["choices"][0]["message"]["content"];
        }

        throw std::runtime_error("Invalid Baidu API response: " + jsonResponse.dump());
    }
    catch (const std::exception& e) {
        curl_easy_cleanup(curl);
        throw;
    }
    
    // 确保资源被清理
    curl_easy_cleanup(curl);
    return "";
}