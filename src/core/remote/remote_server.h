// Copyright Azahar Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include "common/common_types.h"

namespace Core {
class System;
}

namespace Remote {

class HttpServer;
class RequestDispatcher;

class Server {
public:
    explicit Server(Core::System& system, u16 port,
                    std::string bind_address = "127.0.0.1");
    ~Server();

    void Start();
    void Stop();

private:
    Core::System& system;
    u16 port;
    std::string bind_address;
    std::unique_ptr<HttpServer> http_server;
    std::unique_ptr<RequestDispatcher> request_dispatcher;
};

} // namespace Remote
