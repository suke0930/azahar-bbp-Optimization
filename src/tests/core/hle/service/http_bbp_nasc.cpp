// Copyright Citra Emulator Project / Azahar Emulator Project
// Licensed under GPLv2 or any later version

#include <catch2/catch_test_macros.hpp>
#include <string>

#include "core/hle/service/http/bbp_nasc.h"

namespace {

constexpr u64 BBP_TITLE_ID = 0x00040000000A0B00;
constexpr u64 BBP_UPDATE_TITLE_ID = 0x0004000E000A0B00;
constexpr u64 OTHER_TITLE_ID = 0x00040000000A0B01;
constexpr u64 FRD_TITLE_ID = 0x0004013000003202;
constexpr u64 FRD_SAFE_MODE_TITLE_ID = 0x0004013000003203;
constexpr u64 NIM_TITLE_ID = 0x0004013000002C02;

Service::HTTP::URLInfo NascAcUrl() {
    return {
        .is_https = true,
        .host = "nasc.nintendowifi.net",
        .port = 443,
        .path = "/ac",
    };
}

} // namespace

TEST_CASE("BBP NASC shutdown synthesis matches only BBP FRD POST to NASC AC",
          "[core][http][bbp]") {
    const auto url = NascAcUrl();

    REQUIRE(Service::HTTP::BbpNasc::ShouldSynthesizeShutdown(
        BBP_TITLE_ID, FRD_TITLE_ID, Service::HTTP::RequestMethod::Post, url));
    REQUIRE(Service::HTTP::BbpNasc::ShouldSynthesizeShutdown(
        BBP_UPDATE_TITLE_ID, FRD_SAFE_MODE_TITLE_ID, Service::HTTP::RequestMethod::Post, url));
    REQUIRE(Service::HTTP::BbpNasc::ShouldSynthesizeShutdown(
        BBP_TITLE_ID, FRD_TITLE_ID, Service::HTTP::RequestMethod::PostEmpty, url));
}

TEST_CASE("BBP NASC shutdown synthesis rejects unrelated titles, callers, methods, and URLs",
          "[core][http][bbp]") {
    auto url = NascAcUrl();

    REQUIRE_FALSE(Service::HTTP::BbpNasc::ShouldSynthesizeShutdown(
        OTHER_TITLE_ID, FRD_TITLE_ID, Service::HTTP::RequestMethod::Post, url));
    REQUIRE_FALSE(Service::HTTP::BbpNasc::ShouldSynthesizeShutdown(
        BBP_TITLE_ID, NIM_TITLE_ID, Service::HTTP::RequestMethod::Post, url));
    REQUIRE_FALSE(Service::HTTP::BbpNasc::ShouldSynthesizeShutdown(
        BBP_TITLE_ID, FRD_TITLE_ID, Service::HTTP::RequestMethod::Get, url));

    url.host = "example.com";
    REQUIRE_FALSE(Service::HTTP::BbpNasc::ShouldSynthesizeShutdown(
        BBP_TITLE_ID, FRD_TITLE_ID, Service::HTTP::RequestMethod::Post, url));

    url = NascAcUrl();
    url.path = "/other";
    REQUIRE_FALSE(Service::HTTP::BbpNasc::ShouldSynthesizeShutdown(
        BBP_TITLE_ID, FRD_TITLE_ID, Service::HTTP::RequestMethod::Post, url));
}

TEST_CASE("BBP NASC shutdown response carries return code 110", "[core][http][bbp]") {
    REQUIRE(Service::HTTP::BbpNasc::MakeShutdownResponse().find("returncd=MTEw") !=
            std::string::npos);
}
