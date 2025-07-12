/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_SRC_AUDIO_AUDIOMANAGER_H
#define WIREDENGINE_WIREDENGINE_SRC_AUDIO_AUDIOMANAGER_H

#include <Wired/Engine/ResourceIdentifier.h>
#include <Wired/Engine/Audio/AudioCommon.h>
#include <Wired/Engine/Audio/AudioListener.h>
#include <Wired/Engine/Audio/AudioSourceProperties.h>

#include <NEON//Common/AudioData.h>

#include <alext.h>

#include <glm/glm.hpp>

#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <expected>
#include <vector>
#include <deque>
#include <optional>
#include <string>

namespace NCommon
{
    class ILogger;
    class IMetrics;
}

namespace Wired::Engine
{
    enum class PlayState
    {
        Initial,
        Playing,
        Paused,
        Stopped
    };

    enum class SourceDataType
    {
        Static,     // Uses a single buffer which contains all the source audio data
        Streamed    // Has audio data streamed in via enqueued/dequeued buffers
    };

    struct AudioSourceState
    {
        PlayState playState{PlayState::Initial};
        std::optional<double> playTime;
    };

    class AudioManager
    {
        public:

            AudioManager(const NCommon::ILogger* pLogger, NCommon::IMetrics* pMetrics);

            bool Startup();
            void Shutdown();

            void DestroyAll();

            //
            // Load/Destroy asset audio resources
            //
            [[nodiscard]] bool LoadResourceAudio(const ResourceIdentifier& resourceIdentifier, const NCommon::AudioData* pAudioData);
            [[nodiscard]] bool IsResourceAudioLoaded(const ResourceIdentifier& resourceIdentifier);
            void DestroyResourceAudio(const ResourceIdentifier& resourceIdentifier);

            //
            // Create/Destroy/Manipulate audio sources
            //
            [[nodiscard]] std::expected<AudioSourceId, bool> CreateGlobalResourceSource(
                const ResourceIdentifier& resourceIdentifier,
                const AudioSourceProperties& properties,
                bool isTransient
            );
            [[nodiscard]] std::expected<AudioSourceId, bool> CreateLocalResourceSource(
                const ResourceIdentifier& resourceIdentifier,
                const AudioSourceProperties& properties,
                const glm::vec3& position,
                bool isTransient
            );
            [[nodiscard]] std::expected<AudioSourceId, bool> CreateGlobalStreamedSource(
                const AudioSourceProperties& properties
            );
            [[nodiscard]] std::expected<AudioSourceId, bool> CreateLocalStreamedSource(
                const AudioSourceProperties& properties,
                const glm::vec3& position
            );
            [[nodiscard]] bool PlaySource(const AudioSourceId& sourceId) const;
            [[nodiscard]] bool PauseSource(const AudioSourceId& sourceId) const;
            [[nodiscard]] bool StopSource(const AudioSourceId& sourceId) const;
            [[nodiscard]] std::optional<AudioSourceState> GetSourceState(const AudioSourceId& sourceId) const;
            [[nodiscard]] std::optional<SourceDataType> GetSourceDataType(const AudioSourceId& sourceId) const;
            [[nodiscard]] bool EnqueueStreamedData(const AudioSourceId& sourceId,
                                                   std::vector<std::unique_ptr<NCommon::AudioData>> audioDatas,
                                                   double streamStartTime,
                                                   bool autoPlayIfStopped);
            void FlushEnqueuedData(const AudioSourceId& sourceId);
            void DestroySource(const AudioSourceId& sourceId);

            //
            // System-driven
            //
            void UpdateAudioListener(const AudioListener& listener) const;
            [[nodiscard]] bool UpdateLocalSourcePosition(const AudioSourceId& sourceId, const glm::vec3& worldPosition) const;
            void DestroyFinishedTransientSources();
            void DestroyFinishedStreamedData();

        private:

            struct Buffer
            {
                explicit Buffer(ALuint _bufferId,
                                ALenum _bufferFormat,
                                std::optional<ResourceIdentifier> _resourceIdentifier,
                                std::chrono::duration<double> _length,
                                double _streamStartTime = 0.0)
                    : bufferId(_bufferId)
                    , bufferFormat(_bufferFormat)
                    , resourceIdentifier(std::move(_resourceIdentifier))
                    , length(_length)
                    , streamStartTime(_streamStartTime)
                { }

                ALuint bufferId;
                ALenum bufferFormat;
                std::optional<ResourceIdentifier> resourceIdentifier;
                std::chrono::duration<double> length;
                double streamStartTime; // Start time (sec) of this buffer within the full audio stream it belongs to

                std::unordered_set<ALuint> sourceUsage;
            };

            enum class SourcePlayType
            {
                Local,
                Global
            };

            struct Source
            {
                Source(SourcePlayType _playType,
                       SourceDataType _dataType,
                       ALuint _sourceId,
                       const AudioSourceProperties& _audioSourceProperties,
                       bool _isTransient,
                       std::vector<ALuint> _initialBuffers = {})
                    : playType(_playType)
                    , dataType(_dataType)
                    , sourceId(_sourceId)
                    , audioSourceProperties(_audioSourceProperties)
                    , isTransient(_isTransient)
                {
                    std::ranges::copy(_initialBuffers, std::back_inserter(attachedBuffers));
                }

                SourcePlayType playType;
                SourceDataType dataType;
                ALuint sourceId;
                AudioSourceProperties audioSourceProperties;
                bool isTransient;

                std::deque<ALuint> attachedBuffers;
            };

        private:

            [[nodiscard]] std::expected<ALuint, bool> LoadStreamedAudio(const NCommon::AudioData* pAudioData,
                                                                        double streamStartTime = 0.0);

            [[nodiscard]] std::expected<AudioSourceId, bool> CreateResourceSource(
                SourcePlayType sourcePlayType,
                const ResourceIdentifier& resourceIdentifier,
                const AudioSourceProperties& properties,
                const std::optional<glm::vec3>& initialPosition,
                bool isTransient
            );

            [[nodiscard]] std::expected<AudioSourceId, bool> CreateStreamedSource(
                SourcePlayType sourcePlayType,
                const AudioSourceProperties& properties,
                const std::optional<glm::vec3>& initialPosition
            );

            void DestroyBuffer(ALuint bufferId);

            [[nodiscard]] std::expected<ALuint, bool> ALCreateBuffer(const std::vector<const NCommon::AudioData*>& audioDatas);
            void ALDestroyBuffer(ALuint bufferId);

            [[nodiscard]] std::expected<ALuint, bool> ALCreateSource(const SourceDataType& dataType,
                                                                     const AudioSourceProperties& audioSourceProperties,
                                                                     const std::vector<ALuint>& initialBufferIds,
                                                                     const std::optional<glm::vec3>& initialPosition);
            void ALDestroySource(ALuint sourceId);

            [[nodiscard]] std::optional<PlayState> GetPlayState(const AudioSourceId& sourceId) const;
            [[nodiscard]] std::optional<double> GetPlayTime(const AudioSourceId& sourceId) const;

            void SyncMetrics();

        private:

            const NCommon::ILogger* m_pLogger;
            NCommon::IMetrics* m_pMetrics;

            ALCdevice* m_pDevice{nullptr};
            ALCcontext* m_pContext{nullptr};

            mutable std::recursive_mutex m_buffersMutex;
            std::unordered_map<ALuint, Buffer> m_buffers;
            std::unordered_map<ResourceIdentifier, ALuint> m_resourceToBuffer;

            std::unordered_map<ALuint, Source> m_sources;
    };
}

#endif //WIREDENGINE_WIREDENGINE_SRC_AUDIO_AUDIOMANAGER_H
