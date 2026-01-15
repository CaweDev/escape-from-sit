/**
 * @file AudioSource.cpp
 * @brief Implementation of AudioSource class
 */
#include "AudioSource.hpp"

namespace Engine {

AudioSource::AudioSource() {
    // Create audio group for this source
    group = AEAudioCreateGroup();
}

AudioSource::~AudioSource() {
    if (AEAudioIsValidGroup(group)) {
        AEAudioUnloadAudioGroup(group);
    }
}

void AudioSource::Awake() {
    if (playOnAwake && clip && clip->IsLoaded()) {
        Play();
    }
}

void AudioSource::Play() {
    if (!clip || !clip->IsLoaded()) return;
    
    int loops = loop ? -1 : 0;
    AEAudioPlay(clip->GetHandle(), group, volume, pitch, loops);
}

void AudioSource::PlayOneShot(std::shared_ptr<AudioClip> oneShot, f32 volumeScale) {
    if (!oneShot || !oneShot->IsLoaded()) return;
    
    AEAudioPlay(oneShot->GetHandle(), group, volume * volumeScale, pitch, 0);
}

void AudioSource::Stop() {
    AEAudioStopGroup(group);
}

void AudioSource::Pause() {
    AEAudioPauseGroup(group);
}

void AudioSource::Resume() {
    AEAudioResumeGroup(group);
}

void AudioSource::SetVolume(f32 newVolume) {
    volume = newVolume;
    AEAudioSetGroupVolume(group, volume);
}

void AudioSource::SetPitch(f32 newPitch) {
    pitch = newPitch;
    AEAudioSetGroupPitch(group, pitch);
}

} // namespace Engine
