// Copyright Citra Emulator Project / Azahar Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "video_core/bbp_compat.h"

#include <atomic>

namespace VideoCore::BbpCompat {

namespace {
std::atomic_bool current_is_band_brothers_p{};
}

void SetCurrentProgramId(u64 program_id) {
    current_is_band_brothers_p.store(IsBandBrothersPProgramId(program_id), std::memory_order_relaxed);
}

bool IsCurrentBandBrothersP() {
    return current_is_band_brothers_p.load(std::memory_order_relaxed);
}

bool AdjustCurrentWrappedNegativeXViewport(PAddr surface_addr, u32 width, u32 height,
                                           Common::Rectangle<s32>& viewport) {
    if (!CanAdjustWrappedNegativeXViewport(surface_addr, width, height, viewport) ||
        !IsCurrentBandBrothersP()) {
        return false;
    }

    viewport.left += static_cast<s32>(width);
    viewport.right += static_cast<s32>(width);
    return true;
}

} // namespace VideoCore::BbpCompat
