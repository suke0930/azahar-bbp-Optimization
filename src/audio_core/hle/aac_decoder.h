// Copyright Citra Emulator Project / Azahar Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <span>
#include <vector>

#include "audio_core/hle/decoder.h"

namespace AudioCore::HLE {

using NeAACDecHandle = void*;

class AACDecoder final : public DecoderBase {
public:
    explicit AACDecoder(Memory::MemorySystem& memory);
    ~AACDecoder() override;
    BinaryMessage ProcessRequest(const BinaryMessage& request) override;

    void Reset() override;

private:
    BinaryMessage Decode(const BinaryMessage& request);
    bool OpenNewDecoder();
    bool DecodeFrames(std::span<const u8> data, BinaryMessage& response,
                      std::array<std::vector<s16>, 2>& out_streams, const char* mode);
    bool InitializeRawDecoderAndDecode(std::span<const u8> data, BinaryMessage& response,
                                       std::array<std::vector<s16>, 2>& out_streams);

    Memory::MemorySystem& memory;
    NeAACDecHandle decoder = nullptr;
    bool decoder_initialized = false;
    DecoderSampleRate last_sample_rate = DecoderSampleRate::Rate48000;
    u32 last_num_channels = 2;
};

} // namespace AudioCore::HLE
