// Copyright Citra Emulator Project / Azahar Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <chrono>
#include <future>
#include <memory>
#include <vector>

#include <json.hpp>
#include <lodepng.h>
#include "core/3ds.h"
#include "core/core.h"
#include "core/frontend/framebuffer_layout.h"
#include "core/remote/remote_handler.h"
#include "video_core/gpu.h"
#include "video_core/renderer_base.h"

namespace Remote {
namespace {

constexpr auto ScreenshotTimeout = std::chrono::seconds{5};

struct ScreenshotCapture {
    explicit ScreenshotCapture(std::size_t pixel_count) : bgra(pixel_count * 4) {}

    std::vector<u8> bgra;
    std::promise<bool> completion;
    bool invert_y = false;
    VideoCore::ScreenshotPixelFormat format = VideoCore::ScreenshotPixelFormat::BGRA8;
};

void SetJson(RemoteResponse& res, int status, const nlohmann::json& body) {
    res.status_code = status;
    res.content_type = "application/json";
    res.body = body.dump();
}

std::vector<u8> ConvertToRgba(const std::vector<u8>& pixels, u32 width, u32 height, bool invert_y,
                             VideoCore::ScreenshotPixelFormat format) {
    std::vector<u8> rgba(pixels.size());
    for (u32 y = 0; y < height; y++) {
        const u32 src_y = invert_y ? (height - 1 - y) : y;
        for (u32 x = 0; x < width; x++) {
            const std::size_t src = (static_cast<std::size_t>(src_y) * width + x) * 4;
            const std::size_t dst = (static_cast<std::size_t>(y) * width + x) * 4;
            if (format == VideoCore::ScreenshotPixelFormat::BGRA8) {
                rgba[dst] = pixels[src + 2];
                rgba[dst + 1] = pixels[src + 1];
                rgba[dst + 2] = pixels[src];
            } else {
                rgba[dst] = pixels[src];
                rgba[dst + 1] = pixels[src + 1];
                rgba[dst + 2] = pixels[src + 2];
            }
            rgba[dst + 3] = pixels[src + 3];
        }
    }
    return rgba;
}

} // namespace

void HandleVideoScreenshot(Core::System& system, const nlohmann::json& /*body*/, RemoteResponse& res) {
    if (!system.IsPoweredOn()) {
        SetJson(res, 400, {{"error", "Emulator is not running"}, {"code", "not_powered_on"}});
        return;
    }

    auto& renderer = system.GPU().Renderer();
    if (renderer.IsScreenshotPending()) {
        SetJson(res, 409,
                {{"error", "Screenshot already pending"}, {"code", "screenshot_pending"}});
        return;
    }

    const Layout::FramebufferLayout layout = Layout::DefaultFrameLayout(
        Core::kScreenTopWidth, Core::kScreenTopHeight + Core::kScreenBottomHeight, false, false);
    auto capture =
        std::make_shared<ScreenshotCapture>(static_cast<std::size_t>(layout.width) * layout.height);
    auto future = capture->completion.get_future();

    if (!renderer.RequestScreenshot(
            capture->bgra.data(),
            [capture](bool invert_y, VideoCore::ScreenshotPixelFormat format) {
                capture->invert_y = invert_y;
                capture->format = format;
                capture->completion.set_value(true);
            },
            layout)) {
        SetJson(res, 409,
                {{"error", "Screenshot already pending"}, {"code", "screenshot_pending"}});
        return;
    }

    if (system.frame_limiter.IsFrameAdvancing()) {
        system.frame_limiter.AdvanceFrame();
    }

    if (future.wait_for(ScreenshotTimeout) != std::future_status::ready) {
        SetJson(res, 504,
                {{"error", "Timed out waiting for screenshot"}, {"code", "screenshot_timeout"}});
        return;
    }

    const auto rgba = ConvertToRgba(capture->bgra, layout.width, layout.height, capture->invert_y,
                                   capture->format);
    std::vector<u8> png;
    const u32 encode_result = lodepng::encode(png, rgba.data(), layout.width, layout.height);
    if (encode_result != 0) {
        SetJson(res, 500,
                {{"error", lodepng_error_text(encode_result)}, {"code", "screenshot_encode_failed"}});
        return;
    }

    res.status_code = 200;
    res.content_type = "image/png";
    res.body.assign(reinterpret_cast<const char*>(png.data()), png.size());
}

} // namespace Remote
