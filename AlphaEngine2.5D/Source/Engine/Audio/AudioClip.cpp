/**
 * @file AudioClip.cpp
 * @brief Implementation of AudioClip class
 */
#include "AudioClip.hpp"

namespace Engine {

AudioClip::AudioClip() : audio({nullptr}) {}

AudioClip::~AudioClip() {
    Unload();
}

AudioClip::AudioClip(AudioClip&& other) noexcept {
    audio = other.audio;
    other.audio.fmod_sound = nullptr;
}

AudioClip& AudioClip::operator=(AudioClip&& other) noexcept {
    if (this != &other) {
        Unload();
        audio = other.audio;
        other.audio.fmod_sound = nullptr;
    }
    return *this;
}

/*!
 * \brief [AUTO] LoadSound
 * \param filepath [in]
 * \return 
 */
/*!
 * \brief [AUTO] LoadSound
 * \param filepath [in]
 * \return 
 */
/*!
 * \brief [AUTO] LoadSound
 * \param filepath [in]
 * \return 
 */
/*!
 * \brief [AUTO] LoadSound
 * \param filepath [in]
 * \return 
 */
/*!
 * \brief [AUTO] LoadSound
 * \param filepath [in]
 * \return 
 */
bool AudioClip::LoadSound(const std::string& filepath) {
    Unload();
    audio = AEAudioLoadSound(filepath.c_str());
    return IsLoaded();
}

/*!
 * \brief [AUTO] LoadMusic
 * \param filepath [in]
 * \return 
 */
/*!
 * \brief [AUTO] LoadMusic
 * \param filepath [in]
 * \return 
 */
/*!
 * \brief [AUTO] LoadMusic
 * \param filepath [in]
 * \return 
 */
/*!
 * \brief [AUTO] LoadMusic
 * \param filepath [in]
 * \return 
 */
/*!
 * \brief [AUTO] LoadMusic
 * \param filepath [in]
 * \return 
 */
bool AudioClip::LoadMusic(const std::string& filepath) {
    Unload();
    audio = AEAudioLoadMusic(filepath.c_str());
    return IsLoaded();
}

/*!
 * \brief [AUTO] Unload
 */
void AudioClip::Unload() {
    if (IsLoaded()) {
        AEAudioUnloadAudio(audio);
        audio.fmod_sound = nullptr;
    }
}

/*!
 * \brief [AUTO] IsLoaded
 * \return 
 */
/*!
 * \brief [AUTO] IsLoaded
 * \return 
 */
/*!
 * \brief [AUTO] IsLoaded
 * \return 
 */
/*!
 * \brief [AUTO] IsLoaded
 * \return 
 */
/*!
 * \brief [AUTO] IsLoaded
 * \return 
 */
bool AudioClip::IsLoaded() const {
    return AEAudioIsValidAudio(audio) != 0;
}

/*!
 * \brief [AUTO] GetHandle
 * \return 
 */
/*!
 * \brief [AUTO] GetHandle
 * \return 
 */
/*!
 * \brief [AUTO] GetHandle
 * \return 
 */
/*!
 * \brief [AUTO] GetHandle
 * \return 
 */
/*!
 * \brief [AUTO] GetHandle
 * \return 
 */
AEAudio AudioClip::GetHandle() const {
    return audio;
}

} // namespace Engine
