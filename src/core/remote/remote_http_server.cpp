// Copyright Azahar Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <chrono>
#include <future>
#include <utility>

#include <httplib.h>

#include "common/logging/log.h"
#include "common/thread.h"
#include "core/remote/remote_http_server.h"

namespace Remote {

HttpServer::HttpServer(u16 port_, RequestHandler handler_, std::string bind_address_)
    : port(port_), bind_address(std::move(bind_address_)), handler(std::move(handler_)) {}

HttpServer::~HttpServer() {
    Stop();
}

static bool IsValidBindAddress(const std::string& addr) {
    return addr == "127.0.0.1" || addr == "::1" || addr == "localhost";
}

bool HttpServer::Start() {
    if (!IsValidBindAddress(bind_address)) {
        LOG_ERROR(Remote,
                  "Invalid bind address '{}': only loopback addresses (127.0.0.1, ::1, localhost) "
                  "are allowed",
                  bind_address);
        return false;
    }

    server = std::make_unique<httplib::Server>();
    server->set_payload_max_length(1024 * 1024); // Limit request body to 1MB

    server->set_pre_routing_handler([](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        if (req.method == "OPTIONS") {
            res.status = 204;
            return httplib::Server::HandlerResponse::Handled;
        }
        return httplib::Server::HandlerResponse::Unhandled;
    });

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
        try {
            handler(remote_req, remote_res);
        } catch (const std::exception& e) {
            LOG_ERROR(Remote, "Unhandled exception in HTTP handler: {}", e.what());
            res.status = 500;
            res.set_content(R"({"error":"Internal server error","code":"internal_error"})",
                            "application/json");
            return;
        } catch (...) {
            LOG_ERROR(Remote, "Unknown unhandled exception in HTTP handler");
            res.status = 500;
            res.set_content(R"({"error":"Internal server error","code":"internal_error"})",
                            "application/json");
            return;
        }

        res.status = remote_res.status_code;
        res.set_content(remote_res.body, remote_res.content_type.c_str());
    };

    server->Get(".*", make_handler);
    server->Post(".*", make_handler);

    std::promise<bool> start_promise;
    auto start_future = start_promise.get_future();

    server_thread = std::jthread([this, start_promise = std::move(start_promise)](
                                     std::stop_token) mutable {
        Common::SetCurrentThreadName("RemoteHttp");
        LOG_INFO(Remote, "HTTP server listening on port {}", port);
        bool listen_ok = server->listen(bind_address, port);
        if (!listen_ok) {
            LOG_ERROR(Remote, "HTTP server failed to listen on port {}", port);
        }
        start_promise.set_value(listen_ok);
    });

    auto status = start_future.wait_for(std::chrono::seconds(5));
    if (status == std::future_status::timeout) {
        LOG_ERROR(Remote, "HTTP server startup timed out on port {}", port);
        server->stop();
        return false;
    }
    return start_future.get();
}

void HttpServer::Stop() {
    if (!server) {
        return;
    }
    server->stop();
    server_thread = {};
    server.reset();
}

} // namespace Remote
