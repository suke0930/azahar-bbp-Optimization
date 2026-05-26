// Copyright Citra Emulator Project / Azahar Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <json.hpp>

namespace Core {
class System;
}

namespace Remote {

nlohmann::json HandleCheatsList(Core::System& /*system*/, const nlohmann::json& /*body*/) {
    return {{"cheats", nlohmann::json::array()}};
}

nlohmann::json HandleCheatsEnable(Core::System& /*system*/, const nlohmann::json& /*body*/) {
    return {{"status", "ok"}};
}

nlohmann::json HandleCheatsDisable(Core::System& /*system*/, const nlohmann::json& /*body*/) {
    return {{"status", "ok"}};
}

} // namespace Remote
