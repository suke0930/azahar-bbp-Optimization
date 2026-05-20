// Copyright Citra Emulator Project / Azahar Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <array>
#include <cstring>
#include <span>

#include <catch2/catch_test_macros.hpp>

#include "audio_core/hle/aac_decoder.h"
#include "audio_core/hle/decoder.h"
#include "audio_fixures.h"
#include "core/core.h"
#include "core/memory.h"

namespace {

AudioCore::HLE::BinaryMessage InitAacDecoder(AudioCore::HLE::AACDecoder& decoder) {
    AudioCore::HLE::BinaryMessage init_request{};
    init_request.header.codec = AudioCore::HLE::DecoderCodec::DecodeAAC;
    init_request.header.cmd = AudioCore::HLE::DecoderCommand::Init;
    return decoder.ProcessRequest(init_request);
}

AudioCore::HLE::BinaryMessage DecodeAac(AudioCore::HLE::AACDecoder& decoder,
                                        Memory::MemorySystem& memory, std::span<const u8> packet,
                                        std::size_t src_offset, std::size_t dst0_offset,
                                        std::size_t dst1_offset, std::size_t dst_size,
                                        u32 response_mirror_word = 0) {
    if (!packet.empty()) {
        std::memcpy(memory.GetFCRAMPointer(src_offset), packet.data(), packet.size());
    }
    std::memset(memory.GetFCRAMPointer(dst0_offset), 0x7F, dst_size);
    std::memset(memory.GetFCRAMPointer(dst1_offset), 0x7F, dst_size);

    AudioCore::HLE::BinaryMessage decode_request{};
    decode_request.header.codec = AudioCore::HLE::DecoderCodec::DecodeAAC;
    decode_request.header.cmd = AudioCore::HLE::DecoderCommand::EncodeDecode;
    decode_request.decode_aac_request.src_addr = Memory::FCRAM_PADDR + src_offset;
    decode_request.decode_aac_request.size = static_cast<u32>(packet.size());
    decode_request.decode_aac_request.dst_addr_ch0 = Memory::FCRAM_PADDR + dst0_offset;
    decode_request.decode_aac_request.dst_addr_ch1 = Memory::FCRAM_PADDR + dst1_offset;
    decode_request.decode_aac_request.unknown2 = response_mirror_word;
    return decoder.ProcessRequest(decode_request);
}

} // namespace

TEST_CASE("AAC decoder classifies BBP radio packets as raw AAC", "[audio_core][aac]") {
    constexpr std::array<u8, 16> radio_packet_preview = {
        0x21, 0x1C, 0x93, 0xF8, 0xC8, 0x3E, 0x10, 0x74,
        0x00, 0x68, 0x33, 0x7C, 0x87, 0xF4, 0xCD, 0x9E,
    };
    constexpr std::array<u8, 7> adts_packet_preview = {
        0xFF, 0xF1, 0x4C, 0x80, 0x00, 0x1F, 0xFC,
    };
    constexpr std::array<u8, 12> m4a_packet_preview = {
        0x00, 0x00, 0x00, 0x18, 'f',  't',  'y',  'p',  'M',  '4',  'A',  ' ',
    };

    CHECK(AudioCore::HLE::DetectAACInputFormat(radio_packet_preview) ==
          AudioCore::HLE::AACInputFormat::Raw);
    CHECK(AudioCore::HLE::DetectAACInputFormat(adts_packet_preview) ==
          AudioCore::HLE::AACInputFormat::Adts);
    CHECK(AudioCore::HLE::DetectAACInputFormat(m4a_packet_preview) ==
          AudioCore::HLE::AACInputFormat::IsoBmff);
}

TEST_CASE("AAC decoder can decode ADTS AAC payloads", "[audio_core][aac]") {
    Core::System system;
    Memory::MemorySystem memory{system};
    AudioCore::HLE::AACDecoder decoder{memory};

    constexpr std::size_t src_offset = 0x1000;
    constexpr std::size_t dst0_offset = 0x2000;
    constexpr std::size_t dst1_offset = 0x4000;
    constexpr std::size_t dst_size = 0x1000;

    const auto init_response = InitAacDecoder(decoder);
    REQUIRE(init_response.header.result == AudioCore::HLE::ResultStatus::Success);

    const std::span<const u8> packet{fixure_buffer[0]};
    constexpr u32 response_mirror_word = 0x082C7510;
    const auto decode_response = DecodeAac(decoder, memory, packet, src_offset, dst0_offset,
                                           dst1_offset, dst_size, response_mirror_word);

    CHECK(decode_response.header.result == AudioCore::HLE::ResultStatus::Success);
    CHECK(decode_response.decode_aac_response.num_samples == response_mirror_word);
}

TEST_CASE("AAC decoder can decode raw BBP radio AAC access units", "[audio_core][aac]") {
    constexpr std::array<u8, 53> radio_packet = {
        0x21, 0x1C, 0x4F, 0xF7, 0xFF, 0xFF, 0xFF, 0xFF, 0xFD, 0xFD, 0x86, 0x2B, 0x0F, 0x90,
        0x34, 0x2C, 0x03, 0x31, 0x8B, 0xA1, 0x14, 0xFB, 0x9D, 0x7E, 0x26, 0x9D, 0xBF, 0xD1,
        0xA0, 0xEE, 0x07, 0x45, 0x87, 0xC0, 0xD8, 0x00, 0x7F, 0x20, 0x7C, 0x40, 0x00, 0xF6,
        0xDD, 0xCC, 0x46, 0x1E, 0xE2, 0x18, 0x8D, 0x78, 0xBC, 0x0A, 0xE0,
    };

    Core::System system;
    Memory::MemorySystem memory{system};
    AudioCore::HLE::AACDecoder decoder{memory};

    constexpr std::size_t src_offset = 0x1000;
    constexpr std::size_t dst0_offset = 0x2000;
    constexpr std::size_t dst1_offset = 0x4000;
    constexpr std::size_t dst_size = 0x1000;

    const auto init_response = InitAacDecoder(decoder);
    REQUIRE(init_response.header.result == AudioCore::HLE::ResultStatus::Success);

    constexpr u32 response_mirror_word = 0x082C7510;
    const auto decode_response = DecodeAac(decoder, memory, radio_packet, src_offset, dst0_offset,
                                           dst1_offset, dst_size, response_mirror_word);

    CHECK(decode_response.header.result == AudioCore::HLE::ResultStatus::Success);
    CHECK(decode_response.decode_aac_response.num_samples == response_mirror_word);
    CHECK(decode_response.decode_aac_response.sample_rate ==
          AudioCore::HLE::DecoderSampleRate::Rate32000);
    CHECK(decode_response.decode_aac_response.num_channels == 2);
}

TEST_CASE("AAC decoder rejects empty decode requests", "[audio_core][aac]") {
    Core::System system;
    Memory::MemorySystem memory{system};
    AudioCore::HLE::AACDecoder decoder{memory};

    constexpr std::size_t src_offset = 0x1000;
    constexpr std::size_t dst0_offset = 0x2000;
    constexpr std::size_t dst1_offset = 0x4000;
    constexpr std::size_t dst_size = 0x1000;

    const auto init_response = InitAacDecoder(decoder);
    REQUIRE(init_response.header.result == AudioCore::HLE::ResultStatus::Success);

    const std::span<const u8> empty_packet{};
    const auto decode_response =
        DecodeAac(decoder, memory, empty_packet, src_offset, dst0_offset, dst1_offset, dst_size);

    CHECK(decode_response.header.result == AudioCore::HLE::ResultStatus::Error);
}
