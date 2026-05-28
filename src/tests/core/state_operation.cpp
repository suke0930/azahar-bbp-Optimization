// Copyright Citra Emulator Project / Azahar Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <catch2/catch_test_macros.hpp>

#include "core/state_operation.h"

TEST_CASE("StateOperation[RunLoopResult]", "[core]") {
    constexpr u64 frontend_operation_id = 0;
    constexpr u64 remote_operation_id = 1;

    REQUIRE(Core::StateOperation::GetRunLoopResult(frontend_operation_id,
                                                   Core::System::ResultStatus::ErrorSavestate) ==
            Core::System::ResultStatus::ErrorSavestate);
    REQUIRE(Core::StateOperation::GetRunLoopResult(remote_operation_id,
                                                   Core::System::ResultStatus::ErrorSavestate) ==
            Core::System::ResultStatus::Success);
    REQUIRE(Core::StateOperation::GetRunLoopResult(remote_operation_id,
                                                   Core::System::ResultStatus::ShutdownRequested) ==
            Core::System::ResultStatus::ShutdownRequested);
}
