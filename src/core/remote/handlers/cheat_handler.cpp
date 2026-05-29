// Copyright Citra Emulator Project / Azahar Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <cctype>
#include <functional>
#include <limits>
#include <span>

#include <json.hpp>
#include "core/core.h"
#include "core/cheats/cheat_base.h"
#include "core/cheats/cheats.h"
#include "core/remote/remote_handler.h"

namespace Remote {

nlohmann::json ToggleCheatByIndex(const nlohmann::json& body, bool enabled, bool can_persist,
                                  const std::function<bool(std::size_t, bool)>& set_enabled,
                                  const std::function<void()>& persist);

namespace {

std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

std::size_t CountNonEmptyLines(const std::string& code) {
    std::size_t count = 0;
    std::size_t line_start = 0;

    while (line_start <= code.size()) {
        const std::size_t line_end = code.find('\n', line_start);
        const std::size_t end = line_end == std::string::npos ? code.size() : line_end;
        if (end > line_start) {
            ++count;
        }
        if (line_end == std::string::npos) {
            break;
        }
        line_start = line_end + 1;
    }

    return count;
}

nlohmann::json HandleCheatToggle(Core::System& system, const nlohmann::json& body,
                                  bool enabled) {
    const bool is_powered_on = system.IsPoweredOn();
    const u64 title_id = is_powered_on ? system.GetTitleId() : 0;

    return ToggleCheatByIndex(
        body, enabled, is_powered_on && title_id != 0,
        [&system](std::size_t index, bool value) {
            return system.CheatEngine().SetCheatEnabled(index, value);
        },
        [&system, title_id] { system.CheatEngine().SaveCheatFile(title_id); });
}

} // namespace

nlohmann::json BuildCheatsListResponse(std::span<const Cheats::CheatSnapshot> cheats) {
    nlohmann::json cheats_json = nlohmann::json::array();

    for (std::size_t index = 0; index < cheats.size(); ++index) {
        const auto& cheat = cheats[index];

        cheats_json.push_back(nlohmann::json{{"index", index},
                                             {"name", cheat.name},
                                             {"type", ToLower(cheat.type)},
                                             {"enabled", cheat.enabled},
                                             {"code_line_count", CountNonEmptyLines(cheat.code)}});
    }

    return {{"status", "ok"}, {"cheats", cheats_json}};
}

nlohmann::json ToggleCheatByIndex(const nlohmann::json& body, bool enabled, bool can_persist,
                                  const std::function<bool(std::size_t, bool)>& set_enabled,
                                  const std::function<void()>& persist) {
    if (!body.contains("index")) {
        return MakeErrorResponse(400, "Missing cheat index", "missing_index");
    }

    const auto& index_json = body.at("index");
    std::size_t index = 0;

    if (index_json.is_number_unsigned()) {
        const auto raw_index = index_json.get<nlohmann::json::number_unsigned_t>();
        if (raw_index > static_cast<nlohmann::json::number_unsigned_t>(
                             std::numeric_limits<std::size_t>::max())) {
            return MakeErrorResponse(400, "Invalid cheat index", "invalid_index");
        }
        index = static_cast<std::size_t>(raw_index);
    } else if (index_json.is_number_integer()) {
        const auto raw_index = index_json.get<nlohmann::json::number_integer_t>();
        if (raw_index < 0) {
            return MakeErrorResponse(400, "Invalid cheat index", "invalid_index");
        }
        const auto unsigned_index = static_cast<nlohmann::json::number_unsigned_t>(raw_index);
        if (unsigned_index > static_cast<nlohmann::json::number_unsigned_t>(
                                 std::numeric_limits<std::size_t>::max())) {
            return MakeErrorResponse(400, "Invalid cheat index", "invalid_index");
        }
        index = static_cast<std::size_t>(unsigned_index);
    } else {
        return MakeErrorResponse(400, "Invalid cheat index", "invalid_index");
    }

    if (!can_persist) {
        return MakeErrorResponse(400, "Emulator is not running", "not_powered_on");
    }

    if (!set_enabled(index, enabled)) {
        return MakeErrorResponse(404, "Cheat not found", "cheat_not_found");
    }

    persist();

    return {{"status", "ok"}, {"index", index}, {"enabled", enabled}};
}

nlohmann::json HandleCheatsList(Core::System& system, const nlohmann::json& /*body*/) {
    const auto cheats = system.CheatEngine().GetCheatSnapshots();
    return BuildCheatsListResponse(cheats);
}

nlohmann::json HandleCheatsEnable(Core::System& system, const nlohmann::json& body) {
    return HandleCheatToggle(system, body, true);
}

nlohmann::json HandleCheatsDisable(Core::System& system, const nlohmann::json& body) {
    return HandleCheatToggle(system, body, false);
}

} // namespace Remote
