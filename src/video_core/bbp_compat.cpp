// Copyright Citra Emulator Project / Azahar Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "video_core/bbp_compat.h"

#include "core/core.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/process.h"

namespace VideoCore::BbpCompat {

bool IsCurrentBandBrothersP() {
    const auto process = Core::System::GetInstance().Kernel().GetCurrentProcess();
    if (!process || !process->codeset) {
        return false;
    }
    return IsBandBrothersPProgramId(process->codeset->program_id);
}

} // namespace VideoCore::BbpCompat
