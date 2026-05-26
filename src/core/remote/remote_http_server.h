// Copyright Azahar Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <functional>
#include <memory>
#include <string>

#include "common/common_types.h"
#include "common/polyfill_thread.h"
#include "core/remote/remote_types.h"

namespace httplib {
class Server;
} // namespace httplib

namespace Remote {

class HttpServer {
public:
    using RequestHandler = std::function<void(const RemoteRequest&, RemoteResponse&)>;

    explicit HttpServer(u16 port, RequestHandler handler,
                        std::string bind_address = "127.0.0.1");
    ~HttpServer();

    void Start();
    void Stop();

private:
    u16 port;
    std::string bind_address;
    RequestHandler handler;
    std::unique_ptr<httplib::Server> server;
    std::jthread server_thread;
};

} // namespace Remote
