// Copyright Citra Emulator Project / Azahar Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/common_types.h"
#include "common/math_util.h"

namespace VideoCore::BbpCompat {

[[nodiscard]] constexpr bool IsBandBrothersPProgramId(u64 program_id) {
    return program_id == 0x00040000000A0B00 || program_id == 0x0004000E000A0B00;
}

[[nodiscard]] bool IsCurrentBandBrothersP();

[[nodiscard]] constexpr bool RangeOverlaps(PAddr addr, u32 size, PAddr target_addr,
                                           u32 target_size) {
    if (size == 0 || target_size == 0) {
        return false;
    }
    const PAddr end = addr + size;
    const PAddr target_end = target_addr + target_size;
    return addr < target_end && target_addr < end;
}

[[nodiscard]] constexpr bool IsKnownNoteFramebufferRange(PAddr addr, u32 size) {
    return RangeOverlaps(addr, size, 0x18500680, 1024 * 64 * 2) ||
           RangeOverlaps(addr, size, 0x18530780, 1024 * 64 * 2) ||
           RangeOverlaps(addr, size, 0x18520700, 512 * 64 * 2) ||
           RangeOverlaps(addr, size, 0x18550800, 512 * 64 * 2) ||
           RangeOverlaps(addr, size, 0x18560880, 1024 * 8 * 2) ||
           RangeOverlaps(addr, size, 0x18567200, 1024 * 8 * 2) ||
           RangeOverlaps(addr, size, 0x18564900, 512 * 8 * 2) ||
           RangeOverlaps(addr, size, 0x1856b280, 512 * 8 * 2);
}

[[nodiscard]] constexpr bool IsKnownNoteFramebufferUpload(PAddr surface_addr, u32 surface_size,
                                                          bool surface_is_rgba4,
                                                          PAddr upload_addr, u32 upload_size) {
    return surface_is_rgba4 && IsKnownNoteFramebufferRange(surface_addr, surface_size) &&
           IsKnownNoteFramebufferRange(upload_addr, upload_size);
}

[[nodiscard]] constexpr bool ShouldSkipGuardedNoteFramebufferUpload(
    PAddr surface_addr, u32 surface_size, bool surface_is_rgba4, PAddr upload_addr,
    u32 upload_size) {
    if (!IsKnownNoteFramebufferUpload(surface_addr, surface_size, surface_is_rgba4, upload_addr,
                                      upload_size)) {
        return false;
    }

    const PAddr upload_end = upload_addr + upload_size;
    return surface_addr == 0x18500680 &&
           (upload_addr >= 0x18504680 || (upload_addr == 0x18500680 && upload_end == 0x18520680));
}

[[nodiscard]] constexpr bool IsWrappedNegativeXSurface(PAddr surface_addr, u32 width, u32 height) {
    return surface_addr >= 0x18500000 && surface_addr < 0x18600000 && width == 1024 &&
           (height == 8 || height == 64);
}

[[nodiscard]] constexpr bool AdjustWrappedNegativeXViewport(PAddr surface_addr, u32 width,
                                                            u32 height,
                                                            Common::Rectangle<s32>& viewport) {
    if (!IsWrappedNegativeXSurface(surface_addr, width, height) || viewport.right > 0 ||
        viewport.left <= -static_cast<s32>(width)) {
        return false;
    }

    viewport.left += static_cast<s32>(width);
    viewport.right += static_cast<s32>(width);
    return true;
}

} // namespace VideoCore::BbpCompat
