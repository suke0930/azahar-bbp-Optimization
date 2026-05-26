// Copyright Citra Emulator Project / Azahar Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/remote/remote_handler.h"

namespace Remote {

// Handler declarations (defined in handlers/*.cpp with external linkage)
nlohmann::json HandleEmulatorControl(Core::System& system, const nlohmann::json& body);
nlohmann::json HandleEmulatorSpeed(Core::System& system, const nlohmann::json& body);
nlohmann::json HandleEmulatorStatus(Core::System& system, const nlohmann::json& body);
nlohmann::json HandleStateSave(Core::System& system, const nlohmann::json& body);
nlohmann::json HandleStateLoad(Core::System& system, const nlohmann::json& body);
nlohmann::json HandleStateList(Core::System& system, const nlohmann::json& body);
nlohmann::json HandleCheatsList(Core::System& system, const nlohmann::json& body);
nlohmann::json HandleCheatsEnable(Core::System& system, const nlohmann::json& body);
nlohmann::json HandleCheatsDisable(Core::System& system, const nlohmann::json& body);
nlohmann::json HandleEvents(Core::System& system, const nlohmann::json& body);
nlohmann::json HandleVideoScreenshot(Core::System& system, const nlohmann::json& body);

RequestDispatcher::RequestDispatcher(Core::System& system) : system(system) {
    RegisterHandlers();
}

void RequestDispatcher::RegisterHandlers() {
    routes[{std::string("POST"), std::string("/api/v1/emulator/control")}] = HandleEmulatorControl;
    routes[{std::string("POST"), std::string("/api/v1/emulator/speed")}] = HandleEmulatorSpeed;
    routes[{std::string("GET"), std::string("/api/v1/emulator/status")}] = HandleEmulatorStatus;

    routes[{std::string("POST"), std::string("/api/v1/state/save")}] = HandleStateSave;
    routes[{std::string("POST"), std::string("/api/v1/state/load")}] = HandleStateLoad;
    routes[{std::string("GET"), std::string("/api/v1/state/list")}] = HandleStateList;

    routes[{std::string("GET"), std::string("/api/v1/cheats/list")}] = HandleCheatsList;
    routes[{std::string("POST"), std::string("/api/v1/cheats/enable")}] = HandleCheatsEnable;
    routes[{std::string("POST"), std::string("/api/v1/cheats/disable")}] = HandleCheatsDisable;

    routes[{std::string("GET"), std::string("/api/v1/events")}] = HandleEvents;
    routes[{std::string("GET"), std::string("/api/v1/video/screenshot")}] = HandleVideoScreenshot;
}

void RequestDispatcher::Dispatch(const RemoteRequest& req, RemoteResponse& res) {
    auto it = routes.find({req.method, req.path});
    if (it != routes.end()) {
        try {
            nlohmann::json request_body;
            if (!req.body.empty()) {
                request_body = nlohmann::json::parse(req.body);
            }
            nlohmann::json response_json = it->second(system, request_body);
            res.body = response_json.dump();
            res.status_code = 200;
        } catch (const nlohmann::json::parse_error& e) {
            res.status_code = 400;
            res.body = nlohmann::json{{"error", "invalid json"}}.dump();
        } catch (const std::invalid_argument& e) {
            res.status_code = 400;
            res.body = nlohmann::json{{"error", e.what()}}.dump();
        } catch (const std::exception& e) {
            LOG_ERROR(Remote, "Handler exception: {}", e.what());
            res.status_code = 500;
            res.body = nlohmann::json{{"error", "internal server error"}}.dump();
        }
    } else {
        res.status_code = 404;
        try {
            res.body = nlohmann::json{{"error", "not_found"}}.dump();
        } catch (...) {
            res.body = R"({"error":"not_found"})";
        }
    }
}

} // namespace Remote
