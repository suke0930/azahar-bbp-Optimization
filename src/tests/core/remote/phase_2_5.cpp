// Copyright Citra Emulator Project / Azahar Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <catch2/catch_test_macros.hpp>

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <json.hpp>
#include "core/cheats/cheat_base.h"
#include "core/cheats/cheats.h"
#include "core/core.h"
#include "core/remote/remote_handler.h"
#include "core/remote/remote_types.h"
#include "core/savestate.h"

namespace Remote {
nlohmann::json BuildStateListResponse(const std::vector<Core::SaveStateInfo>& savestates);
nlohmann::json BuildCheatsListResponse(std::span<const Cheats::CheatSnapshot> cheats);
nlohmann::json ToggleCheatByIndex(const nlohmann::json& body, bool enabled, bool can_persist,
                                  const std::function<bool(std::size_t, bool)>& set_enabled,
                                  const std::function<void()>& persist);
} // namespace Remote

namespace {

class FakeCheat final : public Cheats::CheatBase {
public:
    FakeCheat(std::string name, std::string type, std::string code, bool enabled)
        : name(std::move(name)), type(std::move(type)), code(std::move(code)), enabled(enabled) {}

    void Execute(Core::System& /*system*/, u32 /*process_id*/) const override {}

    bool IsEnabled() const override {
        return enabled;
    }

    void SetEnabled(bool value) override {
        enabled = value;
    }

    std::string GetComments() const override {
        return {};
    }

    std::string GetName() const override {
        return name;
    }

    std::string GetType() const override {
        return type;
    }

    std::string GetCode() const override {
        return code;
    }

    std::string ToString() const override {
        return name;
    }

private:
    std::string name;
    std::string type;
    std::string code;
    bool enabled;
};

std::shared_ptr<Cheats::CheatBase> MakeCheat(std::string name, std::string code,
                                             bool enabled = false) {
    return std::make_shared<FakeCheat>(std::move(name), "Gateway", std::move(code), enabled);
}

void RequireError(const nlohmann::json& response, int http_status, const std::string& code) {
    REQUIRE(response.at("_http_status") == http_status);
    REQUIRE(response.at("code") == code);
}

void RequireNoRawCheatFields(const nlohmann::json& cheat) {
    REQUIRE(cheat.size() == 5);
    REQUIRE(cheat.contains("index"));
    REQUIRE(cheat.contains("name"));
    REQUIRE(cheat.contains("type"));
    REQUIRE(cheat.contains("enabled"));
    REQUIRE(cheat.contains("code_line_count"));
    REQUIRE_FALSE(cheat.contains("code"));
    REQUIRE_FALSE(cheat.contains("codes"));
    REQUIRE_FALSE(cheat.contains("body"));
    REQUIRE_FALSE(cheat.contains("raw"));
}

} // namespace

TEST_CASE("Remote state list response returns 12 ascending slots", "[core][remote]") {
    const auto response = Remote::BuildStateListResponse({});

    REQUIRE(response.at("status") == "ok");
    const auto& states = response.at("states");
    REQUIRE(states.is_array());
    REQUIRE(states.size() == 12);

    for (std::size_t slot = 0; slot < states.size(); ++slot) {
        REQUIRE(states[slot].at("slot") == slot);
    }
}

TEST_CASE("Remote state list marks missing slots with nullable metadata", "[core][remote]") {
    const auto response = Remote::BuildStateListResponse({});
    const auto& missing = response.at("states").at(3);

    REQUIRE(missing.at("slot") == 3);
    REQUIRE(missing.at("exists") == false);
    REQUIRE(missing.at("time").is_null());
    REQUIRE(missing.at("build_name").is_null());
    REQUIRE(missing.at("status").is_null());
}

TEST_CASE("Remote state list serializes existing slots and ignores out-of-range entries",
          "[core][remote]") {
    const std::vector<Core::SaveStateInfo> savestates{
        {.slot = 2,
         .time = 1717000000,
         .status = Core::SaveStateInfo::ValidationStatus::OK,
         .build_name = "Azahar 2120"},
        {.slot = 11,
         .time = 1717000011,
         .status = Core::SaveStateInfo::ValidationStatus::RevisionDismatch,
         .build_name = "Azahar old"},
        {.slot = 12,
         .time = 1717000012,
         .status = Core::SaveStateInfo::ValidationStatus::OK,
         .build_name = "ignored"},
    };

    const auto response = Remote::BuildStateListResponse(savestates);
    const auto& states = response.at("states");

    REQUIRE(states.size() == 12);
    REQUIRE(states.at(2).at("exists") == true);
    REQUIRE(states.at(2).at("time") == 1717000000);
    REQUIRE(states.at(2).at("build_name") == "Azahar 2120");
    REQUIRE(states.at(2).at("status") == "ok");

    REQUIRE(states.at(11).at("exists") == true);
    REQUIRE(states.at(11).at("time") == 1717000011);
    REQUIRE(states.at(11).at("build_name") == "Azahar old");
    REQUIRE(states.at(11).at("status") == "revision_mismatch");
}

TEST_CASE("Remote cheats list returns an empty cheats array", "[core][remote]") {
    const std::vector<Cheats::CheatSnapshot> cheats;
    const auto response = Remote::BuildCheatsListResponse(cheats);

    REQUIRE(response.at("status") == "ok");
    REQUIRE(response.at("cheats").is_array());
    REQUIRE(response.at("cheats").empty());
}

TEST_CASE("Remote cheats list serializes metadata only", "[core][remote]") {
    constexpr auto raw_code = "DEADBEEF CAFEBABE\n\n01234567 89ABCDEF\n";
    const std::vector<Cheats::CheatSnapshot> cheats{
        {.name = "Example cheat", .type = "Gateway", .code = raw_code, .enabled = true},
    };

    const auto response = Remote::BuildCheatsListResponse(cheats);
    const auto& cheat = response.at("cheats").at(0);

    REQUIRE(response.at("status") == "ok");
    RequireNoRawCheatFields(cheat);
    REQUIRE(cheat.at("index") == 0);
    REQUIRE(cheat.at("name") == "Example cheat");
    REQUIRE(cheat.at("type") == "gateway");
    REQUIRE(cheat.at("enabled") == true);
    REQUIRE(cheat.at("code_line_count") == 2);

    const std::string body = response.dump();
    REQUIRE(body.find("DEADBEEF") == std::string::npos);
    REQUIRE(body.find("CAFEBABE") == std::string::npos);
    REQUIRE(body.find("01234567") == std::string::npos);
}

TEST_CASE("Cheat engine returns independent value cheat snapshots", "[core][remote]") {
    Core::System system;
    auto& cheat_engine = system.CheatEngine();

    cheat_engine.AddCheat(MakeCheat("Original", "00000000 00000000", false));
    const auto snapshots = cheat_engine.GetCheatSnapshots();

    REQUIRE(cheat_engine.SetCheatEnabled(0, true));
    cheat_engine.UpdateCheat(0, MakeCheat("Replacement", "11111111 11111111", true));

    REQUIRE(snapshots.size() == 1);
    REQUIRE(snapshots.at(0).name == "Original");
    REQUIRE(snapshots.at(0).type == "Gateway");
    REQUIRE(snapshots.at(0).code == "00000000 00000000");
    REQUIRE_FALSE(snapshots.at(0).enabled);

    const auto current_snapshots = cheat_engine.GetCheatSnapshots();
    REQUIRE(current_snapshots.at(0).name == "Replacement");
    REQUIRE(current_snapshots.at(0).code == "11111111 11111111");
    REQUIRE(current_snapshots.at(0).enabled);
}

TEST_CASE("Cheat engine toggles cheats by index under engine ownership", "[core][remote]") {
    Core::System system;
    auto& cheat_engine = system.CheatEngine();

    cheat_engine.AddCheat(MakeCheat("Toggle me", "00000000 00000000", false));

    REQUIRE(cheat_engine.SetCheatEnabled(0, true));
    REQUIRE(cheat_engine.GetCheatSnapshots().at(0).enabled);

    REQUIRE_FALSE(cheat_engine.SetCheatEnabled(1, false));
    REQUIRE(cheat_engine.GetCheatSnapshots().at(0).enabled);
}

TEST_CASE("Remote cheats enable and disable are successful and idempotent", "[core][remote]") {
    const std::vector<std::shared_ptr<Cheats::CheatBase>> cheats{
        MakeCheat("Toggle me", "00000000 00000000", false),
    };
    int persist_count = 0;
    const auto set_enabled = [&cheats](std::size_t index, bool value) {
        if (index >= cheats.size()) {
            return false;
        }
        cheats.at(index)->SetEnabled(value);
        return true;
    };
    const auto persist = [&persist_count] { ++persist_count; };

    auto response = Remote::ToggleCheatByIndex({{"index", 0}}, true, true, set_enabled, persist);
    REQUIRE(response.at("status") == "ok");
    REQUIRE(response.at("index") == 0);
    REQUIRE(response.at("enabled") == true);
    REQUIRE(cheats.at(0)->IsEnabled());

    response = Remote::ToggleCheatByIndex({{"index", 0}}, true, true, set_enabled, persist);
    REQUIRE(response.at("status") == "ok");
    REQUIRE(response.at("enabled") == true);
    REQUIRE(cheats.at(0)->IsEnabled());

    response = Remote::ToggleCheatByIndex({{"index", 0}}, false, true, set_enabled, persist);
    REQUIRE(response.at("status") == "ok");
    REQUIRE(response.at("enabled") == false);
    REQUIRE_FALSE(cheats.at(0)->IsEnabled());

    response = Remote::ToggleCheatByIndex({{"index", 0}}, false, true, set_enabled, persist);
    REQUIRE(response.at("status") == "ok");
    REQUIRE(response.at("enabled") == false);
    REQUIRE_FALSE(cheats.at(0)->IsEnabled());
    REQUIRE(persist_count == 4);
}

TEST_CASE("Remote cheats toggle rejects invalid index requests", "[core][remote]") {
    const std::vector<std::shared_ptr<Cheats::CheatBase>> cheats{
        MakeCheat("Only cheat", "00000000 00000000"),
    };
    const auto set_enabled = [&cheats](std::size_t index, bool value) {
        if (index >= cheats.size()) {
            return false;
        }
        cheats.at(index)->SetEnabled(value);
        return true;
    };
    const auto persist = [] {};

    RequireError(Remote::ToggleCheatByIndex(nlohmann::json::object(), true, true, set_enabled,
                                            persist),
                 400, "missing_index");
    RequireError(Remote::ToggleCheatByIndex({{"index", "0"}}, true, true, set_enabled, persist),
                 400, "invalid_index");
    RequireError(Remote::ToggleCheatByIndex({{"index", 0.5}}, true, true, set_enabled, persist),
                 400, "invalid_index");
    RequireError(Remote::ToggleCheatByIndex({{"index", -1}}, true, true, set_enabled, persist),
                 400, "invalid_index");
    RequireError(Remote::ToggleCheatByIndex({{"index", 1}}, true, true, set_enabled, persist),
                 404, "cheat_not_found");
}

TEST_CASE("Remote cheats toggle rejects valid index without powered title context",
           "[core][remote]") {
    const std::vector<std::shared_ptr<Cheats::CheatBase>> cheats{
        MakeCheat("Only cheat", "00000000 00000000"),
    };
    bool persisted = false;
    bool set_enabled_called = false;

    const auto response = Remote::ToggleCheatByIndex(
        {{"index", 0}}, true, false,
        [&cheats, &set_enabled_called](std::size_t index, bool value) {
            set_enabled_called = true;
            if (index >= cheats.size()) {
                return false;
            }
            cheats.at(index)->SetEnabled(value);
            return true;
        },
        [&persisted] { persisted = true; });

    RequireError(response, 400, "not_powered_on");
    REQUIRE_FALSE(cheats.at(0)->IsEnabled());
    REQUIRE_FALSE(set_enabled_called);
    REQUIRE_FALSE(persisted);
}

TEST_CASE("Remote dispatcher strips handler status and rejects malformed JSON", "[core][remote]") {
    Core::System system;
    Remote::RequestDispatcher dispatcher(system);

    Remote::RemoteRequest request;
    request.method = "POST";
    request.path = "/api/v1/cheats/enable";
    request.body = "{";

    Remote::RemoteResponse response;
    dispatcher.Dispatch(request, response);

    const auto body = nlohmann::json::parse(response.body);
    REQUIRE(response.status_code == 400);
    REQUIRE(body.at("code") == "invalid_json");
    REQUIRE_FALSE(body.contains("_http_status"));
}
