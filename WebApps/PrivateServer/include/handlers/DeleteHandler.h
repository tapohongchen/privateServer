#pragma once

#include "../../../HttpServer/include/router/RouterHandler.h"
#include "../PrivateServer.h"


class DeleteHandler : public http::router::RouterHandler
{
public:
    explicit DeleteHandler(PrivateServer* server) : server_(server) {}

    void handle(const http::HttpRequest& req, http::HttpResponse* resp) override;
private:
    PrivateServer* server_;
};