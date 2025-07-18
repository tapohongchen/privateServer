#pragma once
#include "../../../../HttpServer/include/router/RouterHandler.h"
#include "../PrivateServer.h"

class FileCenterHandler : public http::router::RouterHandler {
public:
    explicit FileCenterHandler(PrivateServer* server) : server_(server) {}
    void handle(const http::HttpRequest& req, http::HttpResponse* resp) override;

private:
    PrivateServer* server_;
};
