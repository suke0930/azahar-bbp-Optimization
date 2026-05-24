// Copyright Citra Emulator Project / Azahar Emulator Project
// Licensed under GPLv2 or any later version

#pragma once

#include <string>
#include "core/hle/service/http/http_c.h"

namespace Service::HTTP::BbpNasc {

[[nodiscard]] constexpr bool IsBandBrothersPProgramId(u64 program_id) {
    return program_id == 0x00040000000A0B00 || program_id == 0x0004000E000A0B00;
}

[[nodiscard]] constexpr bool IsFrdProgramId(u64 program_id) {
    return program_id == 0x0004013000003202 || program_id == 0x0004013000003203 ||
           program_id == 0x0004013020003203;
}

[[nodiscard]] inline bool IsNascAcRequest(const URLInfo& url_info) {
    return url_info.is_https && url_info.host == "nasc.nintendowifi.net" &&
           url_info.path == "/ac";
}

[[nodiscard]] inline bool ShouldSynthesizeShutdown(u64 current_title_id, u64 caller_program_id,
                                                   RequestMethod method,
                                                   const URLInfo& url_info) {
    const bool is_post = method == RequestMethod::Post || method == RequestMethod::PostEmpty;
    return IsBandBrothersPProgramId(current_title_id) && IsFrdProgramId(caller_program_id) &&
           is_post && IsNascAcRequest(url_info);
}

[[nodiscard]] inline std::string MakeShutdownResponse() {
    return "retry=MA**&returncd=MTEw&datetime=MjAyNjA1MjQwNjA4NTk*\r\n";
}

} // namespace Service::HTTP::BbpNasc
