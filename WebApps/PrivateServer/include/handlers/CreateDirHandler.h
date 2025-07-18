#pragma once

#include "../../../HttpServer/include/router/RouterHandler.h"
#include "../PrivateServer.h"


class CreateDirHandler : public http::router::RouterHandler
{
public:
    explicit CreateDirHandler(PrivateServer* server) : server_(server) {}

    void handle(const http::HttpRequest& req, http::HttpResponse* resp) override;
private:
    PrivateServer* server_;
};