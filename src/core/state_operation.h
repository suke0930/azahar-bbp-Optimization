// Copyright Citra Emulator Project / Azahar Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/common_types.h"
#include "core/core.h"

namespace Core::StateOperation {

[[nodiscard]] constexpr bool IsRemoteOperation(u64 operation_id) {
    return operation_id != 0;
}

[[nodiscard]] constexpr System::ResultStatus GetRunLoopResult(
    u64 operation_id, System::ResultStatus operation_result) {
    if (IsRemoteOperation(operation_id) &&
        operation_result == System::ResultStatus::ErrorSavestate) {
        return System::ResultStatus::Success;
    }
    return operation_result;
}

} // namespace Core::StateOperation
