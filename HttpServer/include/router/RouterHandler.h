#pragma once
#include <string>
#include <memory>
#include "../http/HttpRequest.h"
#include "../http/HttpResponse.h"

namespace http
{
namespace router
{

class RouterHandler 
{
public:
    virtual ~RouterHandler() = default;
    //这里handle用于后面自己实现的功能去重载
    virtual void handle(const HttpRequest& req, HttpResponse* resp) = 0;
};

} // namespace router
} // namespace http