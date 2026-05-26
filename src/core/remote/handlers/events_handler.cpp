// Copyright Citra Emulator Project / Azahar Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <json.hpp>

namespace Core {
class System;
}

namespace Remote {

nlohmann::json HandleEvents(Core::System& /*system*/, const nlohmann::json& /*body*/) {
    return {{"status", "not_implemented"}};
}

} // namespace Remote
