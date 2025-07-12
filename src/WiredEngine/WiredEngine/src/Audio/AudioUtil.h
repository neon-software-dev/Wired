/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_SRC_AUDIO_AUDIOUTIL_H
#define WIREDENGINE_WIREDENGINE_SRC_AUDIO_AUDIOUTIL_H

#include <AudioFile/AudioFile.h>

#include <NEON/Common/AudioData.h>

#include <vector>
#include <cstddef>
#include <array>
#include <cstdint>
#include <numeric>
#include <expected>

namespace Wired::Engine
{
    struct AudioUtil
    {
        static std::expected<std::unique_ptr<NCommon::AudioData>, bool> AudioDataFromBytes(const std::vector<std::byte>& bytes);

        /**
         * Appends a sample value (range of [-1,1]) to a byte buffer. Converts the sample to bytes as determined by
         * bitDepth parameter. A bitDepth of 8 results in a single byte sample value being appended, while any other
         * bitDepth results in a 16 bit sample value being appended.
         */
        static void AppendSample(std::vector<std::byte>& byteBuffer, const unsigned int& bitDepth, const double& sample);

        /**
         * Converts an AudioFile to a vector of bytes which represent the audio file
         */
        static std::vector<std::byte> AudioFileToByteBuffer(const AudioFile<double>& audioFile);

        /**
         * Combines a collection of audio datas with the same properties (format, sample rate) into one audio data
         */
        static std::expected<std::unique_ptr<NCommon::AudioData>, int> CombineAudioDatas(const std::vector<const NCommon::AudioData*>& audioDatas);
    };
}

#endif //WIREDENGINE_WIREDENGINE_SRC_AUDIO_AUDIOUTIL_H
