// Copyright Citra Emulator Project / Azahar Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <functional>
#include <map>
#include <string>
#include <utility>

#include <json.hpp>
#include "core/remote/remote_types.h"

namespace Core {
class System;
}

namespace Remote {

/**
 * Build a unified error response JSON object.
 * @param status  HTTP status code (used internally by Dispatch)
 * @param message Human-readable error message
 * @param code    Machine-readable error code
 * @return JSON object: { "error": message, "code": code }
 *
 * The HTTP status code is carried in an internal "_http_status" field
 * that Dispatch strips before sending the response body.
 */
nlohmann::json MakeErrorResponse(int status, const std::string& message, const std::string& code);

class RequestDispatcher {
public:
    using HandlerFunc = std::function<nlohmann::json(Core::System&, const nlohmann::json&)>;
    using ResponseHandlerFunc =
        std::function<void(Core::System&, const nlohmann::json&, RemoteResponse&)>;

    explicit RequestDispatcher(Core::System& system);

    void Dispatch(const RemoteRequest& req, RemoteResponse& res);

private:
    void RegisterHandlers();

    Core::System& system;
    std::map<std::pair<std::string, std::string>, HandlerFunc> routes;
    std::map<std::pair<std::string, std::string>, ResponseHandlerFunc> response_routes;
};

} // namespace Remote
