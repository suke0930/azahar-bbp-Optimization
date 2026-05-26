// Copyright Azahar Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/core.h"
#include "core/remote/remote_http_server.h"
#include "core/remote/remote_handler.h"
#include "core/remote/remote_server.h"

namespace Remote {

Server::Server(Core::System& system_, u16 port_, std::string bind_address_)
    : system(system_), port(port_), bind_address(std::move(bind_address_)) {}

Server::~Server() {
    Stop();
}

bool Server::Start() {
    request_dispatcher = std::make_unique<RequestDispatcher>(system);
    http_server = std::make_unique<HttpServer>(
        port, [this](const RemoteRequest& req, RemoteResponse& res) {
            request_dispatcher->Dispatch(req, res);
        },
        bind_address);
    if (!http_server->Start()) {
        LOG_ERROR(Remote, "Failed to start HTTP server on port {}", port);
        http_server.reset();
        request_dispatcher.reset();
        return false;
    }
    return true;
}

void Server::Stop() {
    if (http_server) {
        http_server->Stop();
        http_server.reset();
    }
    request_dispatcher.reset();
}

} // namespace Remote
