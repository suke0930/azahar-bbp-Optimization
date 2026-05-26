// Copyright Citra Emulator Project / Azahar Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/remote/remote_handler.h"

namespace Remote {

nlohmann::json MakeErrorResponse(int status, const std::string& message, const std::string& code) {
    return {{"error", message}, {"code", code}, {"_http_status", status}};
}

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
    routes[{"POST", "/api/v1/emulator/control"}] = HandleEmulatorControl;
    routes[{"POST", "/api/v1/emulator/speed"}] = HandleEmulatorSpeed;
    routes[{"GET", "/api/v1/emulator/status"}] = HandleEmulatorStatus;

    routes[{"POST", "/api/v1/state/save"}] = HandleStateSave;
    routes[{"POST", "/api/v1/state/load"}] = HandleStateLoad;
    routes[{"GET", "/api/v1/state/list"}] = HandleStateList;

    routes[{"GET", "/api/v1/cheats/list"}] = HandleCheatsList;
    routes[{"POST", "/api/v1/cheats/enable"}] = HandleCheatsEnable;
    routes[{"POST", "/api/v1/cheats/disable"}] = HandleCheatsDisable;

    routes[{"GET", "/api/v1/events"}] = HandleEvents;
    routes[{"GET", "/api/v1/video/screenshot"}] = HandleVideoScreenshot;
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

            // Detect error responses from handlers (via internal _http_status field)
            if (response_json.contains("_http_status")) {
                res.status_code = response_json["_http_status"].get<int>();
                response_json.erase("_http_status");
            } else {
                res.status_code = 200;
            }
            res.body = response_json.dump();
        } catch (const nlohmann::json::parse_error& e) {
            res.status_code = 400;
            res.body = MakeErrorResponse(400, "Invalid JSON", "invalid_json").dump();
        } catch (const std::invalid_argument& e) {
            res.status_code = 400;
            res.body = MakeErrorResponse(400, e.what(), "invalid_argument").dump();
        } catch (const std::exception& e) {
            LOG_ERROR(Remote, "Handler exception: {}", e.what());
            res.status_code = 500;
            res.body = MakeErrorResponse(500, "Internal server error", "internal_error").dump();
        }
    } else {
        res.status_code = 404;
        res.body = MakeErrorResponse(404, "Not found", "not_found").dump();
    }
}

} // namespace Remote
