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

class RequestDispatcher {
public:
    using HandlerFunc = std::function<nlohmann::json(Core::System&, const nlohmann::json&)>;

    explicit RequestDispatcher(Core::System& system);

    void Dispatch(const RemoteRequest& req, RemoteResponse& res);

private:
    void RegisterHandlers();

    Core::System& system;
    std::map<std::pair<std::string, std::string>, HandlerFunc> routes;
};

} // namespace Remote
