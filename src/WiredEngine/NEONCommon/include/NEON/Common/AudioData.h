/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef NEONCOMMON_INCLUDE_NEON_COMMON_AUDIODATA_H
#define NEONCOMMON_INCLUDE_NEON_COMMON_AUDIODATA_H

#include <NEON/Common/SharedLib.h>

#include <vector>
#include <cstdint>
#include <chrono>
#include <cassert>

namespace NCommon
{
    enum class AudioDataFormat
    {
        Mono8,      // 1 channel, 1 byte per sample
        Mono16,     // 1 channel, 2 bytes per sample
        Stereo8,    // 2 channels, 2 byte per sample
        Stereo16    // 2 channels, 4 bytes per sample
    };

    [[nodiscard]] static unsigned int GetAudioFormatNumChannels(AudioDataFormat format)
    {
        switch (format)
        {
            case AudioDataFormat::Mono8:
            case AudioDataFormat::Mono16:
                return 1;
            case AudioDataFormat::Stereo8:
            case AudioDataFormat::Stereo16:
                return 2;
        }

        assert(false);
        return 0;
    }

    [[nodiscard]] static uint8_t GetAudioFormatBytesPerSample(AudioDataFormat format)
    {
        switch (format)
        {
            case AudioDataFormat::Mono8: return 1;
            case AudioDataFormat::Mono16: return 2;
            case AudioDataFormat::Stereo8: return 2;
            case AudioDataFormat::Stereo16: return 4;
        }

        assert(false);
        return 0;
    }

    struct NEON_PUBLIC AudioData
    {
        AudioData(const AudioDataFormat& _format, const uint32_t& _sampleRate, std::vector<std::byte> _data)
            : format(_format)
            , sampleRate(_sampleRate)
            , data(std::move(_data))
        {
            assert(sampleRate != 0);
        }

        [[nodiscard]] uint8_t BytesPerSample() const
        {
            return GetAudioFormatBytesPerSample(format);
        }

        [[nodiscard]] std::size_t NumSamples() const
        {
            return data.size() / BytesPerSample();
        }

        [[nodiscard]] std::chrono::duration<double> Duration() const
        {
            return std::chrono::duration<double>((double)NumSamples() / (double)sampleRate);
        }

        [[nodiscard]] bool IsMonoFormat() const
        {
            return GetAudioFormatNumChannels(format) == 1;
        }

        AudioDataFormat format;
        uint32_t sampleRate;
        std::vector<std::byte> data;
    };
}

#endif //NEONCOMMON_INCLUDE_NEON_COMMON_AUDIODATA_H
