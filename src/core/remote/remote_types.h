// Copyright Azahar Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <string>

#include <json.hpp>

namespace Remote {

struct RemoteRequest {
    std::string method;
    std::string path;
    std::string body;
};

struct RemoteResponse {
    int status_code = 200;
    std::string body;
    std::string content_type = "application/json";
};

inline RemoteResponse BuildJsonResponse(const nlohmann::json& data, int status = 200) {
    RemoteResponse response;
    response.status_code = status;
    response.body = data.dump();
    response.content_type = "application/json";
    return response;
}

inline RemoteResponse BuildErrorResponse(const std::string& error, int status = 400) {
    RemoteResponse response;
    response.status_code = status;
    response.content_type = "application/json";
    response.body = nlohmann::json{{"error", error}}.dump();
    return response;
}

} // namespace Remote
