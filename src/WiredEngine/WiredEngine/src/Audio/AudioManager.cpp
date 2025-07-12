/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "AudioManager.h"
#include "AudioUtil.h"

#include <Wired/Engine/Metrics.h>

#include <NEON/Common/Log/ILogger.h>
#include <NEON/Common/Metrics/IMetrics.h>

#include <cassert>

namespace Wired::Engine
{

#define AssertStartedUp() \
    assert(m_pDevice != nullptr && m_pContext != nullptr) \

static std::vector<std::string> ParseALCStringList(const ALCchar* pStringList)
{
    if (pStringList == nullptr) { return {}; }

    std::vector<std::string> strs;

    const ALCchar *pChar = pStringList;

    while (*pChar != '\0')
    {
        const auto strLength = strlen(pChar);
        strs.emplace_back(pChar, strLength);
        pChar += strLength;
    }

    return strs;
}

static ALenum AudioDataFormatToAlFormat(const NCommon::AudioDataFormat& format)
{
    switch (format)
    {
        case NCommon::AudioDataFormat::Mono8: return AL_FORMAT_MONO8;
        case NCommon::AudioDataFormat::Mono16: return AL_FORMAT_MONO16;
        case NCommon::AudioDataFormat::Stereo8: return AL_FORMAT_STEREO8;
        case NCommon::AudioDataFormat::Stereo16: return AL_FORMAT_STEREO16;
    }

    assert(false);
    return AL_FORMAT_MONO8;
}

AudioManager::AudioManager(const NCommon::ILogger* pLogger, NCommon::IMetrics* pMetrics)
    : m_pLogger(pLogger)
    , m_pMetrics(pMetrics)
{

}

bool AudioManager::Startup()
{
    LogInfo("AudioManager: Starting Up");

    //
    // Determine the output device to use
    //
    const ALCchar* pAllDevices{nullptr};
    const ALCchar* pDefaultDeviceName{nullptr};

    if (alcIsExtensionPresent(nullptr, "ALC_enumerate_all_EXT"))
    {
        pAllDevices = alcGetString(nullptr, ALC_ALL_DEVICES_SPECIFIER);
        pDefaultDeviceName = alcGetString(nullptr, ALC_DEFAULT_ALL_DEVICES_SPECIFIER);
    }
    else
    {
        pAllDevices = alcGetString(nullptr, ALC_DEVICE_SPECIFIER);
        pDefaultDeviceName = alcGetString(nullptr, ALC_DEFAULT_DEVICE_SPECIFIER);
    }

    const std::string defaultDevice(pDefaultDeviceName);
    const std::vector<std::string> allDevices = ParseALCStringList(pAllDevices);

    std::vector<std::string> candidateDevices
    {
        defaultDevice
    };

    for (const auto& device : allDevices)
    {
        if (device != defaultDevice)
        {
            candidateDevices.push_back(device);
        }
    }

    LogInfo("AudioManager: Discovered {} audio device(s)", candidateDevices.size());

    //
    // Open an audio device for output
    //
    for (const auto& device : candidateDevices)
    {
        LogInfo("AudioManager: Attempting to open output device: {}", device);

        alGetError();
        m_pDevice = alcOpenDevice(device.c_str());
        if (m_pDevice == nullptr)
        {
            LogError("AudioManager: alcOpenDevice failed for device: {}, error code: {}", device, alGetError());
            continue;
        }

        LogInfo("AudioManager: Using output device: {}", device);
    }

    if (m_pDevice == nullptr)
    {
        LogError("AudioManager: Exhausted all available audio devices, aborting");
        return false;
    }

    //
    // Create an OpenAL audio context
    //
    alGetError();
    m_pContext = alcCreateContext(m_pDevice, nullptr);
    if (m_pContext == nullptr)
    {
        LogError("AudioManager: alcCreateContext failed, error code: {}", alGetError());
        Shutdown();
        return false;
    }

    //
    // Activate the audio context
    //
    alGetError();
    if (alcMakeContextCurrent(m_pContext) == ALC_FALSE)
    {
        LogError("AudioManager: alcMakeContextCurrent failed, error code: {}", alGetError());
        Shutdown();
        return false;
    }

    return true;
}

void AudioManager::Shutdown()
{
    LogInfo("AudioManager shutting down");

    // Unload any existing resources
    DestroyAll();

    // Shutdown and destroy the audio context + device
    alcMakeContextCurrent(nullptr);

    if (m_pContext != nullptr)
    {
        alcDestroyContext(m_pContext);
        m_pContext = nullptr;
    }

    if (m_pDevice != nullptr)
    {
        alcCloseDevice(m_pDevice);
        m_pDevice = nullptr;
    }
}

void AudioManager::DestroyAll()
{
    LogInfo("AudioManager: Destroying all");

    while (!m_sources.empty())
    {
        DestroySource(m_sources.cbegin()->first);
    }

    while (!m_buffers.empty())
    {
        DestroyBuffer(m_buffers.cbegin()->first);
    }
}

bool AudioManager::LoadResourceAudio(const ResourceIdentifier& resourceIdentifier, const NCommon::AudioData* pAudioData)
{
    AssertStartedUp();

    LogInfo("AudioManager: Loading resource audio: {}", resourceIdentifier.GetUniqueName());

    std::lock_guard buffersLock(m_buffersMutex);

    if (IsResourceAudioLoaded(resourceIdentifier))
    {
        LogWarning("AudioManager::LoadResourceAudio: Resource already has audio loaded, ignoring: {}", resourceIdentifier.GetUniqueName());
        return true;
    }

    const auto bufferId = ALCreateBuffer({pAudioData});
    if (!bufferId)
    {
        LogError("AudioManager::LoadResourceAudio: Failed to create buffer for resource audio: {}", resourceIdentifier.GetUniqueName());
        return false;
    }

    const auto bufferFormat = AudioDataFormatToAlFormat(pAudioData->format);

    const auto buffer = Buffer(*bufferId, bufferFormat, resourceIdentifier, pAudioData->Duration());

    m_buffers.insert({*bufferId, buffer});
    m_resourceToBuffer.insert({resourceIdentifier, *bufferId});

    LogInfo("AudioManager: Created buffer {} for resource audio: {}", *bufferId, resourceIdentifier.GetUniqueName());

    SyncMetrics();

    return true;
}

std::expected<ALuint, bool> AudioManager::LoadStreamedAudio(const NCommon::AudioData* pAudioData, double streamStartTime)
{
    AssertStartedUp();

    LogDebug("AudioManager: Loading streamed audio");

    std::lock_guard buffersLock(m_buffersMutex);

    const auto bufferId = ALCreateBuffer({pAudioData});
    if (!bufferId)
    {
        LogError("AudioManager::LoadStreamedAudio: Failed to create buffer for streamed audio");
        return std::unexpected(false);
    }

    const auto bufferFormat = AudioDataFormatToAlFormat(pAudioData->format);

    const auto buffer = Buffer(*bufferId, bufferFormat, std::nullopt, pAudioData->Duration(), streamStartTime);

    m_buffers.insert({*bufferId, buffer});

    LogDebug("AudioManager: Created buffer {} for streamed audio", *bufferId);

    SyncMetrics();

    return *bufferId;
}

bool AudioManager::IsResourceAudioLoaded(const ResourceIdentifier& resourceIdentifier)
{
    std::lock_guard buffersLock(m_buffersMutex);

    return m_resourceToBuffer.contains(resourceIdentifier);
}

void AudioManager::DestroyResourceAudio(const ResourceIdentifier& resourceIdentifier)
{
    AssertStartedUp();

    LogInfo("AudioManager: Destroying resource audio: {}", resourceIdentifier.GetUniqueName());

    std::lock_guard buffersLock(m_buffersMutex);

    const auto it = m_resourceToBuffer.find(resourceIdentifier);
    if (it == m_resourceToBuffer.cend())
    {
        LogWarning("AudioManager::DestroyResourceAudio: No such asset audio record exists: {}", resourceIdentifier.GetUniqueName());
        return;
    }

    DestroyBuffer(it->second);
}

[[nodiscard]] std::expected<AudioSourceId, bool> AudioManager::CreateGlobalResourceSource(
    const ResourceIdentifier& resourceIdentifier,
    const AudioSourceProperties& properties,
    bool isTransient
)
{
    return CreateResourceSource(SourcePlayType::Global, resourceIdentifier, properties, std::nullopt, isTransient);
}

[[nodiscard]] std::expected<AudioSourceId, bool> AudioManager::CreateLocalResourceSource(
    const ResourceIdentifier& resourceIdentifier,
    const AudioSourceProperties& properties,
    const glm::vec3& position,
    bool isTransient
)
{
    return CreateResourceSource(SourcePlayType::Local, resourceIdentifier, properties, position, isTransient);
}

std::expected<AudioSourceId, bool> AudioManager::CreateGlobalStreamedSource(
    const AudioSourceProperties& properties
)
{
    return CreateStreamedSource(SourcePlayType::Global, properties, std::nullopt);
}

std::expected<AudioSourceId, bool> AudioManager::CreateLocalStreamedSource(
    const AudioSourceProperties& properties,
    const glm::vec3& position
)
{
    return CreateStreamedSource(SourcePlayType::Local, properties, position);
}

std::expected<AudioSourceId, bool> AudioManager::CreateResourceSource(
    SourcePlayType sourcePlayType,
    const ResourceIdentifier& resourceIdentifier,
    const AudioSourceProperties& properties,
    const std::optional<glm::vec3>& initialPosition,
    bool isTransient
)
{
    AssertStartedUp();

    LogInfo("AudioManager: Creating source for resource audio: {}", resourceIdentifier.GetUniqueName());

    std::lock_guard lock(m_buffersMutex);

    const auto resourceIt = m_resourceToBuffer.find(resourceIdentifier);
    if (resourceIt == m_resourceToBuffer.cend())
    {
        LogError("AudioManager::CreateResourceSource: Unable to create source as resource has no audio loaded: {}", resourceIdentifier.GetUniqueName());
        return std::unexpected(false);
    }

    const auto bufferIt = m_buffers.find(resourceIt->second);
    if (bufferIt == m_buffers.cend())
    {
        LogError("AudioManager::CreateResourceSource: No such buffer exists: {}", resourceIt->second);
        return std::unexpected(false);
    }

    // If we're creating a local source, the audio must be in mono format, as OpenAL can't spatialize a
    // stereo audio source
    if (sourcePlayType == SourcePlayType::Local)
    {
        if (bufferIt->second.bufferFormat != AL_FORMAT_MONO8 && bufferIt->second.bufferFormat != AL_FORMAT_MONO16)
        {
            LogError("AudioManager::CreateResourceSource: Local audio sources require mono-format audio data");
            return std::unexpected(false);
        }
    }

    //
    // Create the source
    //
    std::vector<ALuint> initialBuffers = {bufferIt->first};

    const auto sourceId = ALCreateSource(SourceDataType::Static, properties, initialBuffers, initialPosition);
    if (!sourceId)
    {
        LogError("AudioManager::CreateResourceSource: Failed to create source for resource: {}", resourceIt->second);
        return std::unexpected(false);
    }

    //
    // Update the asset's buffer to know it's in-use by this source
    //
    bufferIt->second.sourceUsage.insert(*sourceId);

    //
    // Record the source and return
    //
    Source source(sourcePlayType, SourceDataType::Static, *sourceId, properties, isTransient, initialBuffers);

    m_sources.insert({*sourceId, std::move(source)});

    SyncMetrics();

    return *sourceId;
}

std::expected<AudioSourceId, bool> AudioManager::CreateStreamedSource(
    SourcePlayType sourcePlayType,
    const AudioSourceProperties& properties,
    const std::optional<glm::vec3>& initialPosition
)
{
    AssertStartedUp();

    LogInfo("AudioManager: Creating source for streamed audio");

    //
    // Create the source
    //
    const auto sourceId = ALCreateSource(SourceDataType::Streamed, properties, {}, initialPosition);
    if (!sourceId)
    {
        LogError("AudioManager::CreateStreamedSource: Failed to create source");
        return std::unexpected(false);
    }

    //
    // Record the source and return
    //
    Source source(sourcePlayType, SourceDataType::Streamed, *sourceId, properties, false, {});

    m_sources.insert({*sourceId, std::move(source)});

    SyncMetrics();

    return *sourceId;
}

bool AudioManager::PlaySource(const AudioSourceId& sourceId) const
{
    AssertStartedUp();

    LogDebug("AudioManager: Playing audio source: {}", sourceId);

    if (!m_sources.contains(sourceId)) { return false; }

    alSourcePlay(sourceId);

    return true;
}

bool AudioManager::PauseSource(const AudioSourceId& sourceId) const
{
    AssertStartedUp();

    LogInfo("AudioManager: Pausing audio source: {}", sourceId);

    if (!m_sources.contains(sourceId)) { return false; }

    alSourcePause(sourceId);

    return true;
}

bool AudioManager::StopSource(const AudioSourceId& sourceId) const
{
    AssertStartedUp();

    LogDebug("AudioManager: Stopping audio source: {}", sourceId);

    if (!m_sources.contains(sourceId)) { return false; }

    alSourceStop(sourceId);

    return true;
}

std::optional<PlayState> AudioManager::GetPlayState(const AudioSourceId& sourceId) const
{
    AssertStartedUp();

    const auto sourceIt = m_sources.find(sourceId);
    if (sourceIt == m_sources.cend())
    {
        LogError("AudioManager::GetPlayState: No such source exists: {}", sourceId);
        return std::nullopt;
    }

    alGetError();
    ALint sourceState{AL_INVALID};
    alGetSourcei(sourceId, AL_SOURCE_STATE, &sourceState);
    if (const auto error = alGetError(); error != AL_NO_ERROR)
    {
        return std::nullopt;
    }

    if (sourceState == AL_INITIAL)      { return PlayState::Initial; }
    else if (sourceState == AL_PLAYING) { return PlayState::Playing; }
    else if (sourceState == AL_PAUSED)  { return PlayState::Paused; }
    else if (sourceState == AL_STOPPED) { return PlayState::Stopped; }
    else
    {
        LogError("AudioManager::GetSourceState: Unhandled OpenAL source state: {}", sourceState);
        return std::nullopt;
    }
}

std::optional<double> AudioManager::GetPlayTime(const AudioSourceId& sourceId) const
{
    AssertStartedUp();

    const auto sourceIt = m_sources.find(sourceId);
    if (sourceIt == m_sources.cend())
    {
        LogError("AudioManager::GetPlayTime: No such source exists: {}", sourceId);
        return std::nullopt;
    }

    // If the source has no data associated with it, we can't determine play time
    if (sourceIt->second.attachedBuffers.empty())
    {
        return std::nullopt;
    }

    const auto frontBufferId = sourceIt->second.attachedBuffers.front();
    const auto frontBufferIt = m_buffers.find(frontBufferId);
    if (frontBufferIt == m_buffers.cend())
    {
        LogError("AudioManager::GetPlayTime: Front buffer {} for source {} doesn't exist", frontBufferId, sourceId);
        return std::nullopt;
    }

    const auto backBufferId = sourceIt->second.attachedBuffers.back();
    const auto backBufferIt = m_buffers.find(backBufferId);
    if (backBufferIt == m_buffers.cend())
    {
        LogError("AudioManager::GetPlayTime: Back buffer {} for source {} doesn't exist", backBufferId, sourceId);
        return std::nullopt;
    }

    const auto playState = GetPlayState(sourceId);
    if (!playState)
    {
        LogError("AudioManager::GetPlayTime: Failed to get play state");
        return std::nullopt;
    }

    switch (*playState)
    {
        // If the source hasn't started playing yet, its play time is the stream start time of its
        // first buffer (it's effectively stuck at its play point until it's played)
        case PlayState::Initial:
        {
            return frontBufferIt->second.streamStartTime;
        }
            // Otherwise, if the source is playing or paused, we can query OpenAL for the offset since
            // the start of its initial (attached) buffer
        case PlayState::Playing:
        case PlayState::Paused:
        {
            float sourceSecOffset{0.0f};

            alGetError();
            alGetSourcef(sourceId, AL_SEC_OFFSET, &sourceSecOffset);
            if (const auto error = alGetError(); error != AL_NO_ERROR)
            {
                LogError("AudioManager::GetPlayTime: Failed to query for source offset");
                return std::nullopt;
            }

            return frontBufferIt->second.streamStartTime + (double)sourceSecOffset;
        }
            // Otherwise, if the source is stopped, querying for sec offset would return 0, so return
            // that it's at the end of its last buffer (similar to Initial state, it's effectively
            // stuck at the end of its play duration)
        case PlayState::Stopped:
        {
            return backBufferIt->second.streamStartTime + backBufferIt->second.length.count();
        }
    }

    assert(false);
    return std::nullopt;
}

std::optional<AudioSourceState> AudioManager::GetSourceState(const AudioSourceId& sourceId) const
{
    AssertStartedUp();

    const auto playState = GetPlayState(sourceId);
    if (!playState)
    {
        LogError("AudioManager::GetSourceState: Failed to get play state for source: {}", sourceId);
        return std::nullopt;
    }

    return AudioSourceState{
        .playState = *playState,
        .playTime = GetPlayTime(sourceId)
    };
}

std::optional<SourceDataType> AudioManager::GetSourceDataType(const AudioSourceId& sourceId) const
{
    const auto it = m_sources.find(sourceId);
    if (it == m_sources.cend())
    {
        return std::nullopt;
    }

    return it->second.dataType;
}

bool AudioManager::EnqueueStreamedData(const AudioSourceId& sourceId,
                                       std::vector<std::unique_ptr<NCommon::AudioData>> audioDatas,
                                       double streamStartTime,
                                       bool autoPlayIfStopped)
{
    AssertStartedUp();

    std::lock_guard lock(m_buffersMutex);

    const auto sourceIt = m_sources.find(sourceId);
    if (sourceIt == m_sources.cend())
    {
        LogError("AudioManager::EnqueueStreamedData: No such source exists: {}", sourceId);
        return false;
    }

    std::vector<const NCommon::AudioData*> audioDataPtrs;
    std::optional<NCommon::AudioDataFormat> audioFormat;
    std::optional<uint32_t> audioSampleRate;
    bool audioDatasMatch = true;

    for (const auto& audioData : audioDatas)
    {
        audioDataPtrs.push_back(audioData.get());

        // If the source is a local source, the audio must be in mono format, as OpenAL can't spatialize a
        // stereo audio source
        if (sourceIt->second.playType == SourcePlayType::Local)
        {
            if (!audioData->IsMonoFormat())
            {
                LogError("AudioManager::EnqueueStreamedData: Local audio source require mono-formatted audio data");
                return false;
            }
        }

        if (!audioFormat || !audioSampleRate)
        {
            audioFormat = audioData->format;
            audioSampleRate = audioData->sampleRate;
        }
        else if (*audioFormat != audioData->format ||
                 *audioSampleRate != audioData->sampleRate)
        {
            audioDatasMatch = false;
            break;
        }
    }

    std::vector<std::unique_ptr<NCommon::AudioData>> audioDataToLoad;

    // If all the audio data's configuration (format, sample rate) matches, we can combine them
    // all into one audio data in one buffer, rather than creating separate buffers for each one
    if (audioDatasMatch)
    {
        auto combinedAudioData = AudioUtil::CombineAudioDatas(audioDataPtrs);
        if (combinedAudioData)
        {
            audioDataToLoad.push_back(std::move(*combinedAudioData));
        }
        else
        {
            LogError("AudioManager::EnqueueStreamedData: Failed to combine audio data");
            audioDataToLoad = std::move(audioDatas);
        }
    }
    // Otherwise, create a separate buffer for each provided audio data
    else
    {
        audioDataToLoad = std::move(audioDatas);
    }

    //
    // Load the audio data into buffers
    //
    std::vector<ALuint> audioDataBufferIds;

    const auto destroyCreatedBuffers = [&](){
        for (const auto& toDelete : audioDataBufferIds)
        {
            DestroyBuffer(toDelete);
        }
    };

    for (const auto& audioData : audioDataToLoad)
    {
        const auto bufferId = LoadStreamedAudio(audioData.get(), streamStartTime);
        if (!bufferId)
        {
            LogError("AudioManager::EnqueueStreamedData: Failed to load streamed audio");
            destroyCreatedBuffers();
            return false;
        }

        audioDataBufferIds.push_back(*bufferId);
    }

    //
    // Enqueue the buffers with the source
    //
    alGetError();
    alSourceQueueBuffers(sourceId, (ALsizei)audioDataBufferIds.size(), audioDataBufferIds.data());
    if (const auto error = alGetError(); error != AL_NO_ERROR)
    {
        LogError("AudioManager::EnqueueStreamedData: alSourceQueueBuffers failed, error code: {}", error);
        destroyCreatedBuffers();
        return false;
    }

    for (const auto& bufferId : audioDataBufferIds)
    {
        sourceIt->second.attachedBuffers.push_back(bufferId);
    }

    //
    // For each buffer, mark it as in use by the source
    //
    for (const auto& bufferId : audioDataBufferIds)
    {
        m_buffers.at(bufferId).sourceUsage.insert(sourceId);
    }

    //
    // Play the source if requested
    //
    if (autoPlayIfStopped)
    {
        const auto playState = GetPlayState(sourceId);
        if (playState == PlayState::Initial || playState == PlayState::Stopped)
        {
            return PlaySource(sourceId);
        }
    }

    return true;
}

void AudioManager::FlushEnqueuedData(const AudioSourceId& sourceId)
{
    const auto sourceIt = m_sources.find(sourceId);
    if (sourceIt == m_sources.cend())
    {
        LogError("AudioManager::FlushEnqueuedData: No such source exists: {}", sourceId);
        return;
    }

    if (sourceIt->second.dataType != SourceDataType::Streamed)
    {
        LogError("AudioManager::FlushEnqueuedData: Can't flush enqueued data for non-streamed audio source: {}", sourceId);
        return;
    }

    (void)StopSource(sourceId);

    //
    // Unqueue the buffers from the source
    //
    std::vector<ALuint> attachedBuffers;

    std::ranges::copy(sourceIt->second.attachedBuffers, std::back_inserter(attachedBuffers));
    sourceIt->second.attachedBuffers.clear();

    alGetError();
    alSourceUnqueueBuffers(sourceId, (ALsizei)attachedBuffers.size(), attachedBuffers.data());
    if (const auto error = alGetError(); error != AL_NO_ERROR)
    {
        LogError("AudioManager::FlushEnqueuedData: alSourceUnqueueBuffers failed, error code: {}", error);
    }

    //
    // Mark the buffers as no longer in use by the source
    //
    for (const auto& bufferId : attachedBuffers)
    {
        m_buffers.at(bufferId).sourceUsage.erase(sourceId);
    }

    //
    // Destroy the buffers
    //
    for (const auto& bufferId : attachedBuffers)
    {
        DestroyBuffer(bufferId);
    }
}

void AudioManager::DestroySource(const AudioSourceId& sourceId)
{
    AssertStartedUp();

    LogInfo("AudioManager: Destroying audio source: {}", sourceId);

    const auto sourceIt = m_sources.find(sourceId);
    if (sourceIt == m_sources.cend())
    {
        LogWarning("AudioManager::DestroySource: No such source exists: {}", sourceId);
        return;
    }

    // Make sure the source isn't playing
    alSourceStop(sourceId);

    // For each buffer attached to the source, record it that it's no longer in use by the source
    {
        std::lock_guard lock(m_buffersMutex);

        for (const auto& attachedBuffer : sourceIt->second.attachedBuffers)
        {
            const auto bufferIt = m_buffers.find(attachedBuffer);
            if (bufferIt != m_buffers.cend())
            {
                bufferIt->second.sourceUsage.erase(sourceId);
            }
        }
    }

    // Destroy and erase the source
    ALDestroySource(sourceId);
    m_sources.erase(sourceId);

    SyncMetrics();
}

void AudioManager::UpdateAudioListener(const AudioListener& listener) const
{
    AssertStartedUp();

    alListenerf(AL_GAIN, listener.gain);

    alListener3f(AL_POSITION, listener.worldPosition.x, listener.worldPosition.y, listener.worldPosition.z);

    float orientationVals[6];
    orientationVals[0] = listener.lookUnit.x;
    orientationVals[1] = listener.lookUnit.y;
    orientationVals[2] = listener.lookUnit.z;
    orientationVals[3] = listener.upUnit.x;
    orientationVals[4] = listener.upUnit.y;
    orientationVals[5] = listener.upUnit.z;
    alListenerfv(AL_ORIENTATION, orientationVals);
}

bool AudioManager::UpdateLocalSourcePosition(const AudioSourceId& sourceId, const glm::vec3& worldPosition) const
{
    AssertStartedUp();

    const auto sourceIt = m_sources.find(sourceId);
    if (sourceIt == m_sources.cend())
    {
        LogError("AudioManager::UpdateLocalSourcePosition: No such source exists: {}", sourceId);
        return false;
    }

    alSource3f(sourceId, AL_POSITION, worldPosition.x, worldPosition.y, worldPosition.z);

    return true;
}

void AudioManager::DestroyFinishedTransientSources()
{
    AssertStartedUp();

    std::unordered_set<ALuint> toDestroy;

    //
    // Find sources that are marked as transient and are in stopped state
    //
    for (const auto& sourceIt : m_sources)
    {
        if (!sourceIt.second.isTransient)
        {
            continue;
        }

        const auto playState = GetPlayState(sourceIt.first);
        if (!playState)
        {
            continue;
        }

        if (*playState == PlayState::Stopped)
        {
            toDestroy.insert(sourceIt.first);
        }
    }

    //
    // Destroy each such identified source
    //
    for (const auto& sourceId : toDestroy)
    {
        LogInfo("AudioManager: Found stopped transient source to be destroyed: {}", sourceId);
        DestroySource(sourceId);
    }
}

void AudioManager::DestroyFinishedStreamedData()
{
    AssertStartedUp();

    std::lock_guard buffersLock(m_buffersMutex);

    //
    // Find streamed sources with finished/processed/played buffers that can now be
    // unqueued from the source and destroyed
    //
    for (auto& sourceIt : m_sources)
    {
        const auto sourceId = sourceIt.first;

        // Skip non-streamed sources
        if (sourceIt.second.dataType != SourceDataType::Streamed)
        {
            continue;
        }

        //
        // Query for the source's number of finished/processed buffers
        //
        ALint numBuffersProcessed{0};
        alGetSourcei(sourceId, AL_BUFFERS_PROCESSED, &numBuffersProcessed);

        // Nothing to clean up
        if (numBuffersProcessed <= 0)
        {
            continue;
        }

        // Edge case: if more buffers are processed than we know about (shouldn't ever be the case)
        if ((unsigned int)numBuffersProcessed > sourceIt.second.attachedBuffers.size())
        {
            LogError("AudioManager::DestroyFinishedStreamedData: numBuffersProcessed is larger than the number of buffers we know about");
            numBuffersProcessed = std::min(numBuffersProcessed, (int)sourceIt.second.attachedBuffers.size());
        }

        // We want to unqueue all but the LAST processed buffer, so that even when a streamed source
        // has finished playing all its enqueued data, a call to GetSourcePlayTime() will be able to
        // return that the source's play position is at the end of its previously enqueued stream length
        if (numBuffersProcessed == (int)sourceIt.second.attachedBuffers.size())
        {
            // If there's one buffer and its finished, leave it alone
            if (numBuffersProcessed == 1)
            {
                return;
            }

            // Otherwise, decrement the num buffers processed so we unqueue all but the last
            numBuffersProcessed--;
        }

        //
        // Unqueue the buffers from the source
        //
        std::vector<ALuint> processedBuffers;

        for (int x = 0; x < numBuffersProcessed; ++x)
        {
            const auto bufferId = sourceIt.second.attachedBuffers.front();
            processedBuffers.push_back(bufferId);
            sourceIt.second.attachedBuffers.pop_front();
        }

        alGetError();
        alSourceUnqueueBuffers(sourceId, (ALsizei)processedBuffers.size(), processedBuffers.data());
        if (const auto error = alGetError(); error != AL_NO_ERROR)
        {
            LogError("AudioManager::DestroyFinishedStreamedData: alSourceUnqueueBuffers failed, error code: {}", error);
        }

        //
        // Mark the buffers as no longer in use by the source
        //
        for (const auto& bufferId : processedBuffers)
        {
            m_buffers.at(bufferId).sourceUsage.erase(sourceId);
        }

        //
        // Destroy the buffers
        //
        for (const auto& bufferId : processedBuffers)
        {
            DestroyBuffer(bufferId);
        }
    }
}

void AudioManager::DestroyBuffer(ALuint bufferId)
{
    AssertStartedUp();

    LogDebug("AudioManager: Destroying buffer: {}", bufferId);

    std::lock_guard buffersLock(m_buffersMutex);

    const auto bufferIt = m_buffers.find(bufferId);
    if (bufferIt == m_buffers.cend())
    {
        LogWarning("AudioManager::DestroyBuffer: No such buffer record exists: {}", bufferId);
        return;
    }

    //
    // Erase any mapping of asset to the buffer
    //
    if (bufferIt->second.resourceIdentifier)
    {
        m_resourceToBuffer.erase(*bufferIt->second.resourceIdentifier);
    }

    //
    // Destroy any sources that have the buffer actively attached
    //
    const auto sourceUsages = bufferIt->second.sourceUsage; // Copy, because DestroySource below modifies the buffer's sourceUsage

    for (const auto& sourceUsage : sourceUsages)
    {
        LogDebug("AudioManager::DestroyBuffer: Destroying buffer {} while source {} is actively using it", bufferId, sourceUsage);
        DestroySource(sourceUsage);
    }

    //
    // Destroy and erase the buffer
    //
    ALDestroyBuffer(bufferId);
    m_buffers.erase(bufferIt);

    SyncMetrics();
}

std::expected<ALuint, bool> AudioManager::ALCreateBuffer(const std::vector<const NCommon::AudioData*>& audioDatas)
{
    AssertStartedUp();

    const auto audioData = AudioUtil::CombineAudioDatas(audioDatas);
    if (!audioData)
    {
        LogError("AudioManager::ALCreateBuffer: Failed to combine audio datas, error code: {}", audioData.error());
        return std::unexpected(false);
    }

    //
    // Generate an audio buffer
    //
    ALuint bufferId{AL_NONE};

    alGetError();
    alGenBuffers(1, &bufferId);
    if (const auto error = alGetError(); error != AL_NO_ERROR)
    {
        LogError("AudioManager::ALCreateBuffer: alGenBuffers failed, error code: {}", error);
        return std::unexpected(false);
    }

    //
    // Populate the audio buffer
    //
    alGetError();
    alBufferData(bufferId,
                 AudioDataFormatToAlFormat((*audioData)->format),
                 (*audioData)->data.data(),
                 (ALsizei)(*audioData)->data.size(),
                 (ALsizei)(*audioData)->sampleRate);
    if (const auto error = alGetError(); error != AL_NO_ERROR)
    {
        LogError("AudioManager::ALCreateBuffer: alBufferData failed, error code: {}", error);
        alDeleteBuffers(1, &bufferId);
        return std::unexpected(false);
    }

    return bufferId;
}

void AudioManager::ALDestroyBuffer(ALuint bufferId)
{
    AssertStartedUp();

    alGetError();
    alDeleteBuffers(1, &bufferId);
    if (const auto error = alGetError(); error != AL_NO_ERROR)
    {
        LogError("AudioManager::ALDestroyBuffer: alDeleteBuffers failed, error code: {}", error);
    }
}

std::expected<ALuint, bool> AudioManager::ALCreateSource(const SourceDataType& dataType,
                                                         const AudioSourceProperties& audioSourceProperties,
                                                         const std::vector<ALuint>& initialBufferIds,
                                                         const std::optional<glm::vec3>& initialPosition)
{
    AssertStartedUp();

    //
    // Create the audio source
    //
    ALuint sourceId{AL_NONE};

    alGetError();
    alGenSources(1, &sourceId);
    if (const auto error = alGetError(); error != AL_NO_ERROR)
    {
        LogError("AudioManager::ALCreateSource: alGenSources failed, error code: {}", error);
        return std::unexpected(false);
    }

    //
    // Set source audio properties
    //
    alSourcef(sourceId, AL_REFERENCE_DISTANCE, audioSourceProperties.referenceDistance);
    alSourcef(sourceId, AL_GAIN, audioSourceProperties.gain);

    if (dataType == SourceDataType::Static)
    {
        alSourcei(sourceId, AL_LOOPING, audioSourceProperties.looping);
    }

    if (initialPosition)
    {
        alSource3f(sourceId, AL_POSITION, initialPosition->x, initialPosition->y, initialPosition->z);
    }

    //
    // Attach initial buffers to the source
    //

    // Validation
    if (dataType == SourceDataType::Static && initialBufferIds.size() != 1)
    {
        LogError("AudioManager::ALCreateSource: Static sources require exactly one initial data buffer to be provided");
        return std::unexpected(false);
    }

    if (!initialBufferIds.empty())
    {
        switch (dataType)
        {
            case SourceDataType::Static:
            {
                alSourcei(sourceId, AL_BUFFER, (ALint)initialBufferIds.front());
            }
                break;
            case SourceDataType::Streamed:
            {
                alSourceQueueBuffers(sourceId, (ALsizei)initialBufferIds.size(), initialBufferIds.data());
            }
                break;
        }
    }

    return sourceId;
}

void AudioManager::ALDestroySource(ALuint sourceId)
{
    AssertStartedUp();

    alDeleteSources(1, &sourceId);
}

void AudioManager::SyncMetrics()
{
    m_pMetrics->SetCounterValue(METRIC_AUDIO_NUM_BUFFERS, m_buffers.size());
    m_pMetrics->SetCounterValue(METRIC_AUDIO_NUM_SOURCES, m_sources.size());
}

}
