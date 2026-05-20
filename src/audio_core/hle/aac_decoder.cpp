// Copyright Citra Emulator Project / Azahar Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <array>
#include <string>

#include <neaacdec.h>
#include "audio_core/hle/aac_decoder.h"

namespace AudioCore::HLE {

namespace {

constexpr std::size_t preview_bytes = 16;

struct RawAacCandidateConfig {
    std::array<u8, 2> asc;
    u32 sample_rate;
    u8 channels;
};

constexpr std::array<u8, 2> BuildAacLcAsc(u8 sampling_frequency_index, u8 channels) {
    constexpr u8 object_type = LC;
    return {
        static_cast<u8>((object_type << 3) | (sampling_frequency_index >> 1)),
        static_cast<u8>(((sampling_frequency_index & 1) << 7) | (channels << 3)),
    };
}

constexpr std::array<RawAacCandidateConfig, 12> raw_aac_candidate_configs = {{
    {BuildAacLcAsc(5, 2), 32000, 2},
    {BuildAacLcAsc(4, 2), 44100, 2},
    {BuildAacLcAsc(3, 2), 48000, 2},
    {BuildAacLcAsc(6, 2), 24000, 2},
    {BuildAacLcAsc(7, 2), 22050, 2},
    {BuildAacLcAsc(8, 2), 16000, 2},
    {BuildAacLcAsc(5, 1), 32000, 1},
    {BuildAacLcAsc(4, 1), 44100, 1},
    {BuildAacLcAsc(3, 1), 48000, 1},
    {BuildAacLcAsc(6, 1), 24000, 1},
    {BuildAacLcAsc(7, 1), 22050, 1},
    {BuildAacLcAsc(8, 1), 16000, 1},
}};

[[nodiscard]] bool HasTagAt(std::span<const u8> data, std::size_t offset,
                            std::array<char, 4> tag) {
    if (data.size() < offset + tag.size()) {
        return false;
    }
    for (std::size_t i = 0; i < tag.size(); ++i) {
        if (data[offset + i] != static_cast<u8>(tag[i])) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] std::string PreviewBytes(std::span<const u8> data) {
    static constexpr char hex[] = "0123456789ABCDEF";

    const std::size_t count = std::min(preview_bytes, data.size());
    std::string out;
    out.reserve(count * 3);
    for (std::size_t i = 0; i < count; ++i) {
        if (i != 0) {
            out.push_back(' ');
        }
        out.push_back(hex[(data[i] >> 4) & 0xF]);
        out.push_back(hex[data[i] & 0xF]);
    }
    return out;
}

[[nodiscard]] const char* ToString(AACInputFormat format) {
    switch (format) {
    case AACInputFormat::Raw:
        return "raw";
    case AACInputFormat::Adts:
        return "adts";
    case AACInputFormat::IsoBmff:
        return "iso_bmff";
    }
    return "raw";
}

} // namespace

AACInputFormat DetectAACInputFormat(std::span<const u8> data) {
    if (data.size() >= 2 && data[0] == 0xFF && (data[1] & 0xF6) == 0xF0) {
        return AACInputFormat::Adts;
    }
    if (HasTagAt(data, 4, {'f', 't', 'y', 'p'})) {
        return AACInputFormat::IsoBmff;
    }
    return AACInputFormat::Raw;
}

AACDecoder::AACDecoder(Memory::MemorySystem& memory) : memory(memory) {
    Reset();
}

AACDecoder::~AACDecoder() {
    if (decoder) {
        NeAACDecClose(decoder);
        decoder = nullptr;
    }
}

BinaryMessage AACDecoder::ProcessRequest(const BinaryMessage& request) {
    if (request.header.codec != DecoderCodec::DecodeAAC) {
        LOG_ERROR(Audio_DSP, "AAC decoder received unsupported codec: {}",
                  static_cast<u16>(request.header.codec));
        return {
            .header =
                {
                    .result = ResultStatus::Error,
                },
        };
    }

    switch (request.header.cmd) {
    case DecoderCommand::Init: {
        BinaryMessage response = request;
        response.header.result = ResultStatus::Success;
        if (!OpenNewDecoder()) {
            response.header.result = ResultStatus::Error;
        }
        return response;
    }
    case DecoderCommand::EncodeDecode: {
        return Decode(request);
    }
    case DecoderCommand::Shutdown:
    case DecoderCommand::SaveState:
    case DecoderCommand::LoadState: {
        LOG_WARNING(Audio_DSP, "Got unimplemented AAC binary request: {}",
                    static_cast<u16>(request.header.cmd));
        BinaryMessage response = request;
        response.header.result = ResultStatus::Success;
        return response;
    }
    default:
        LOG_ERROR(Audio_DSP, "Got unknown AAC binary request: {}",
                  static_cast<u16>(request.header.cmd));
        return {
            .header =
                {
                    .result = ResultStatus::Error,
                },
        };
    }
}

void AACDecoder::Reset() {
    last_sample_rate = DecoderSampleRate::Rate48000;
    last_num_channels = 2;
    OpenNewDecoder();
}

BinaryMessage AACDecoder::Decode(const BinaryMessage& request) {
    BinaryMessage response{};
    response.header.codec = request.header.codec;
    response.header.cmd = request.header.cmd;
    response.header.result = ResultStatus::Success;
    response.decode_aac_response.size = request.decode_aac_request.size;
    response.decode_aac_response.sample_rate = last_sample_rate;
    response.decode_aac_response.num_channels = last_num_channels;
    // The real DSP mirrors this request word in the last decode response word.
    response.decode_aac_response.num_samples = request.decode_aac_request.unknown2;

    if (decoder == nullptr) {
        LOG_ERROR(Audio_DSP, "Failed to handle decode request: FAAD2 AAC decoder not open.");
        response.header.result = ResultStatus::Error;
        return response;
    }

    if (request.decode_aac_request.src_addr < Memory::FCRAM_PADDR ||
        request.decode_aac_request.src_addr + request.decode_aac_request.size >
            Memory::FCRAM_PADDR + Memory::FCRAM_SIZE) {
        LOG_ERROR(Audio_DSP, "Got out of bounds src_addr {:08x}",
                  request.decode_aac_request.src_addr);
        response.header.result = ResultStatus::Error;
        return response;
    }
    u8* data = memory.GetFCRAMPointer(request.decode_aac_request.src_addr - Memory::FCRAM_PADDR);
    u32 data_len = request.decode_aac_request.size;
    if (data_len == 0) {
        LOG_ERROR(Audio_DSP, "Got empty AAC decode request.");
        response.header.result = ResultStatus::Error;
        return response;
    }

    const std::span<const u8> input{data, data_len};
    const AACInputFormat input_format = DetectAACInputFormat(input);

    std::array<std::vector<s16>, 2> out_streams;
    bool decoded_during_initialization = false;

    if (!decoder_initialized) {
        if (input_format == AACInputFormat::Raw) {
            if (!InitializeRawDecoderAndDecode(input, response, out_streams)) {
                response.header.result = ResultStatus::Error;
                return response;
            }
            decoded_during_initialization = true;
        } else {
            unsigned long sample_rate{};
            u8 num_channels{};
            const auto init_result =
                NeAACDecInit(decoder, data, data_len, &sample_rate, &num_channels);
            if (init_result < 0) {
                LOG_ERROR(Audio_DSP, "Could not initialize FAAD2 AAC decoder for request: {} "
                                     "preview={}",
                          init_result, PreviewBytes(input));
                response.header.result = ResultStatus::Error;
                return response;
            }

            decoder_initialized = true;
            last_sample_rate = GetSampleRateEnum(sample_rate);
            last_num_channels = num_channels;
            response.decode_aac_response.sample_rate = last_sample_rate;
            response.decode_aac_response.num_channels = last_num_channels;

            data += init_result;
            data_len -= init_result;
        }
    }

    if (!decoded_during_initialization) {
        if (!DecodeFrames({data, data_len}, input_format, response, out_streams, "stream")) {
            response.header.result = ResultStatus::Error;
            return response;
        }
    }

    // Transfer the decoded buffer from vector to the FCRAM.
    for (std::size_t ch = 0; ch < out_streams.size(); ch++) {
        if (out_streams[ch].empty()) {
            continue;
        }
        auto byte_size = out_streams[ch].size() * sizeof(s16);
        auto dst = ch == 0 ? request.decode_aac_request.dst_addr_ch0
                           : request.decode_aac_request.dst_addr_ch1;
        if (dst < Memory::FCRAM_PADDR ||
            dst + byte_size > Memory::FCRAM_PADDR + Memory::FCRAM_SIZE) {
            LOG_ERROR(Audio_DSP, "Got out of bounds dst_addr_ch{} {:08x}", ch, dst);
            response.header.result = ResultStatus::Error;
            return response;
        }
        std::memcpy(memory.GetFCRAMPointer(dst - Memory::FCRAM_PADDR), out_streams[ch].data(),
                    byte_size);
    }

    return response;
}

bool AACDecoder::OpenNewDecoder() {
    if (decoder) {
        NeAACDecClose(decoder);
    }
    decoder_initialized = false;

    decoder = NeAACDecOpen();
    if (decoder == nullptr) {
        LOG_CRITICAL(Audio_DSP, "Could not open FAAD2 decoder.");
        return false;
    }

    auto config = NeAACDecGetCurrentConfiguration(decoder);
    config->defObjectType = LC;
    config->outputFormat = FAAD_FMT_16BIT;
    if (!NeAACDecSetConfiguration(decoder, config)) {
        LOG_CRITICAL(Audio_DSP, "Could not configure FAAD2 decoder.");
        NeAACDecClose(decoder);
        decoder = nullptr;
        return false;
    }

    return true;
}

bool AACDecoder::DecodeFrames(std::span<const u8> data, AACInputFormat input_format,
                              BinaryMessage& response,
                              std::array<std::vector<s16>, 2>& out_streams, const char* mode) {
    const u8* ptr = data.data();
    u32 remaining = static_cast<u32>(data.size());

    while (remaining > 0) {
        NeAACDecFrameInfo frame_info{};
        auto curr_sample_buffer =
            static_cast<s16*>(NeAACDecDecode(decoder, &frame_info, const_cast<u8*>(ptr), remaining));
        if (curr_sample_buffer == nullptr || frame_info.error != 0) {
            LOG_ERROR(Audio_DSP,
                      "Failed to decode AAC buffer using FAAD2: {} mode={} format={} remaining={} "
                      "preview={} bytes_consumed={}",
                      frame_info.error, mode, ToString(input_format), remaining,
                      PreviewBytes(std::span<const u8>{ptr, remaining}), frame_info.bytesconsumed);
            return false;
        }

        if (frame_info.channels == 0 || frame_info.channels > out_streams.size() ||
            frame_info.bytesconsumed == 0 || frame_info.bytesconsumed > remaining ||
            frame_info.samples % frame_info.channels != 0) {
            LOG_ERROR(Audio_DSP,
                      "FAAD2 produced an invalid AAC frame: channels={} bytesconsumed={} "
                      "remaining={} samples={}",
                      frame_info.channels, frame_info.bytesconsumed, remaining, frame_info.samples);
            return false;
        }

        last_sample_rate = GetSampleRateEnum(frame_info.samplerate);
        last_num_channels = frame_info.channels;
        response.decode_aac_response.sample_rate = last_sample_rate;
        response.decode_aac_response.num_channels = last_num_channels;

        const u32 num_samples = frame_info.samples / frame_info.channels;
        for (u32 sample = 0; sample < num_samples; sample++) {
            for (u32 ch = 0; ch < frame_info.channels; ch++) {
                out_streams[ch].push_back(curr_sample_buffer[(sample * frame_info.channels) + ch]);
            }
        }

        ptr += frame_info.bytesconsumed;
        remaining -= frame_info.bytesconsumed;
    }

    return true;
}

bool AACDecoder::InitializeRawDecoderAndDecode(std::span<const u8> data, BinaryMessage& response,
                                               std::array<std::vector<s16>, 2>& out_streams) {
    const DecoderSampleRate previous_sample_rate = last_sample_rate;
    const u32 previous_num_channels = last_num_channels;
    const DecoderSampleRate previous_response_sample_rate = response.decode_aac_response.sample_rate;
    const u32 previous_response_num_channels = response.decode_aac_response.num_channels;

    for (const auto& candidate : raw_aac_candidate_configs) {
        if (!OpenNewDecoder()) {
            last_sample_rate = previous_sample_rate;
            last_num_channels = previous_num_channels;
            response.decode_aac_response.sample_rate = previous_response_sample_rate;
            response.decode_aac_response.num_channels = previous_response_num_channels;
            return false;
        }

        unsigned long sample_rate{};
        u8 num_channels{};
        const auto init_result =
            NeAACDecInit2(decoder, const_cast<u8*>(candidate.asc.data()),
                          static_cast<unsigned long>(candidate.asc.size()), &sample_rate,
                          &num_channels);
        if (init_result < 0) {
            continue;
        }

        decoder_initialized = true;
        last_sample_rate = GetSampleRateEnum(sample_rate);
        last_num_channels = num_channels;
        response.decode_aac_response.sample_rate = last_sample_rate;
        response.decode_aac_response.num_channels = last_num_channels;

        std::array<std::vector<s16>, 2> trial_out_streams;
        if (!DecodeFrames(data, AACInputFormat::Raw, response, trial_out_streams, "raw")) {
            decoder_initialized = false;
            last_sample_rate = previous_sample_rate;
            last_num_channels = previous_num_channels;
            response.decode_aac_response.sample_rate = previous_response_sample_rate;
            response.decode_aac_response.num_channels = previous_response_num_channels;
            continue;
        }

        // FAAD2 can accept a raw AAC access unit before producing PCM on this path.
        out_streams = std::move(trial_out_streams);
        return true;
    }

    LOG_ERROR(Audio_DSP,
              "Could not initialize FAAD2 raw AAC decoder using fallback AudioSpecificConfig "
              "guesses: preview={}",
              PreviewBytes(data));
    return false;
}

} // namespace AudioCore::HLE
