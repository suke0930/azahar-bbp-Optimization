// Copyright Azahar Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/core.h"
#include "core/remote/remote_http_server.h"
#include "core/remote/remote_handler.h"
#include "core/remote/remote_server.h"

namespace Remote {

Server::Server(Core::System& system_, u16 port_)
    : system(system_), port(port_) {}

Server::~Server() {
    Stop();
}

void Server::Start() {
    request_dispatcher = std::make_unique<RequestDispatcher>(system);
    http_server = std::make_unique<HttpServer>(
        port, [this](const RemoteRequest& req, RemoteResponse& res) {
            request_dispatcher->Dispatch(req, res);
        });
    http_server->Start();
}

void Server::Stop() {
    if (http_server) {
        http_server->Stop();
        http_server.reset();
    }
    request_dispatcher.reset();
}

} // namespace Remote
