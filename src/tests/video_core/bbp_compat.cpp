// Copyright Citra Emulator Project / Azahar Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <catch2/catch_test_macros.hpp>
#include "video_core/bbp_compat.h"

TEST_CASE("BBP compatibility only matches Band Brothers P program IDs", "[video_core][bbp]") {
    REQUIRE(VideoCore::BbpCompat::IsBandBrothersPProgramId(0x00040000000A0B00));
    REQUIRE(VideoCore::BbpCompat::IsBandBrothersPProgramId(0x0004000E000A0B00));
    REQUIRE_FALSE(VideoCore::BbpCompat::IsBandBrothersPProgramId(0));
    REQUIRE_FALSE(VideoCore::BbpCompat::IsBandBrothersPProgramId(0x00040000000A0B01));
}

TEST_CASE("BBP compatibility wraps only narrow negative 1024-wide strip viewports",
          "[video_core][bbp]") {
    Common::Rectangle<s32> viewport{-112, 0, -38, 8};
    REQUIRE(VideoCore::BbpCompat::AdjustWrappedNegativeXViewport(0x18560880, 1024, 8, viewport));
    REQUIRE(viewport.left == 912);
    REQUIRE(viewport.right == 986);

    Common::Rectangle<s32> positive{304, 0, 378, 8};
    REQUIRE_FALSE(
        VideoCore::BbpCompat::AdjustWrappedNegativeXViewport(0x18560880, 1024, 8, positive));

    Common::Rectangle<s32> partial{-12, 0, 12, 8};
    REQUIRE_FALSE(
        VideoCore::BbpCompat::AdjustWrappedNegativeXViewport(0x18560880, 1024, 8, partial));

    Common::Rectangle<s32> too_far{-1100, 0, -1025, 8};
    REQUIRE_FALSE(
        VideoCore::BbpCompat::AdjustWrappedNegativeXViewport(0x18560880, 1024, 8, too_far));

    Common::Rectangle<s32> wrong_width{-112, 0, -38, 8};
    REQUIRE_FALSE(
        VideoCore::BbpCompat::AdjustWrappedNegativeXViewport(0x18560880, 512, 8, wrong_width));
}

TEST_CASE("BBP compatibility skips only known stale note framebuffer uploads",
          "[video_core][bbp]") {
    REQUIRE(VideoCore::BbpCompat::IsKnownNoteFramebufferRange(0x18500680, 1024 * 64 * 2));
    REQUIRE(VideoCore::BbpCompat::IsKnownNoteFramebufferRange(0x18504680, 1024));
    REQUIRE_FALSE(VideoCore::BbpCompat::IsKnownNoteFramebufferRange(0x18600680, 1024));

    REQUIRE(VideoCore::BbpCompat::ShouldSkipGuardedNoteFramebufferUpload(
        0x18500680, 1024 * 64 * 2, true, 0x18504680, 1024 * 48 * 2));
    REQUIRE(VideoCore::BbpCompat::ShouldSkipGuardedNoteFramebufferUpload(
        0x18500680, 1024 * 64 * 2, true, 0x18500680, 1024 * 64 * 2));
    REQUIRE_FALSE(VideoCore::BbpCompat::ShouldSkipGuardedNoteFramebufferUpload(
        0x18530780, 1024 * 64 * 2, true, 0x18534780, 1024 * 48 * 2));
    REQUIRE_FALSE(VideoCore::BbpCompat::ShouldSkipGuardedNoteFramebufferUpload(
        0x18500680, 1024 * 64 * 2, false, 0x18504680, 1024 * 48 * 2));
}
