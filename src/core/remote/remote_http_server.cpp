// Copyright Azahar Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <utility>

#include <httplib.h>

#include "common/logging/log.h"
#include "common/thread.h"
#include "core/remote/remote_http_server.h"

namespace Remote {

HttpServer::HttpServer(u16 port_, RequestHandler handler_)
    : port(port_), handler(std::move(handler_)) {}

HttpServer::~HttpServer() {
    Stop();
}

void HttpServer::Start() {
    server = std::make_unique<httplib::Server>();

    auto make_handler = [this](const httplib::Request& req, httplib::Response& res) {
        RemoteRequest remote_req;
        remote_req.method = req.method;
        remote_req.path = req.path;
        if (const auto pos = remote_req.path.find('?'); pos != std::string::npos) {
            remote_req.path = remote_req.path.substr(0, pos);
        }
        remote_req.body = req.body;
        for (const auto& [key, value] : req.params) {
            remote_req.query_params[key] = value;
        }

        RemoteResponse remote_res;
        handler(remote_req, remote_res);

        res.status = remote_res.status_code;
        res.set_content(remote_res.body, remote_res.content_type.c_str());
    };

    server->Get(".*", make_handler);
    server->Post(".*", make_handler);

    server_thread = std::jthread([this](std::stop_token) {
        Common::SetCurrentThreadName("RemoteHttp");
        LOG_INFO(Remote, "HTTP server listening on port {}", port);
        if (!server->listen("0.0.0.0", port)) {
            LOG_ERROR(Remote, "HTTP server failed to listen on port {}", port);
        }
    });
}

void HttpServer::Stop() {
    if (server) {
        server->stop();
    }
    server_thread = {};
    server.reset();
}

} // namespace Remote
