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

/*!
 * \brief [AUTO] Awake
 */
void AudioSource::Awake() {
    if (playOnAwake && clip && clip->IsLoaded()) {
        Play();
    }
}

/*!
 * \brief [AUTO] Play
 */
void AudioSource::Play() {
    if (!clip || !clip->IsLoaded()) return;
    
    int loops = loop ? -1 : 0;
    AEAudioPlay(clip->GetHandle(), group, volume, pitch, loops);
}

/*!
 * \brief [AUTO] PlayOneShot
 * \param oneShot [in]
 * \param volumeScale [in]
 */
/*!
 * \brief [AUTO] PlayOneShot
 * \param oneShot [in]
 * \param volumeScale [in]
 */
/*!
 * \brief [AUTO] PlayOneShot
 * \param oneShot [in]
 * \param volumeScale [in]
 */
/*!
 * \brief [AUTO] PlayOneShot
 * \param oneShot [in]
 * \param volumeScale [in]
 */
void AudioSource::PlayOneShot(AudioClip* oneShot, f32 volumeScale) {
    if (!oneShot || !oneShot->IsLoaded()) return;
    
    AEAudioPlay(oneShot->GetHandle(), group, volume * volumeScale, pitch, 0);
}

/*!
 * \brief [AUTO] Stop
 */
void AudioSource::Stop() {
    AEAudioStopGroup(group);
}

/*!
 * \brief [AUTO] Pause
 */
void AudioSource::Pause() {
    AEAudioPauseGroup(group);
}

/*!
 * \brief [AUTO] Resume
 */
void AudioSource::Resume() {
    AEAudioResumeGroup(group);
}

/*!
 * \brief [AUTO] SetVolume
 * \param newVolume [in]
 */
/*!
 * \brief [AUTO] SetVolume
 * \param newVolume [in]
 */
/*!
 * \brief [AUTO] SetVolume
 * \param newVolume [in]
 */
/*!
 * \brief [AUTO] SetVolume
 * \param newVolume [in]
 */
void AudioSource::SetVolume(f32 newVolume) {
    volume = newVolume;
    AEAudioSetGroupVolume(group, volume);
}

/*!
 * \brief [AUTO] SetPitch
 * \param newPitch [in]
 */
/*!
 * \brief [AUTO] SetPitch
 * \param newPitch [in]
 */
/*!
 * \brief [AUTO] SetPitch
 * \param newPitch [in]
 */
/*!
 * \brief [AUTO] SetPitch
 * \param newPitch [in]
 */
void AudioSource::SetPitch(f32 newPitch) {
    pitch = newPitch;
    AEAudioSetGroupPitch(group, pitch);
}

} // namespace Engine
