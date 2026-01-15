/**
 * @file AudioClip.hpp
 * @brief Audio clip wrapper for Alpha Engine audio
 */
#pragma once

#include "AEEngine.h"
#include <string>

namespace Engine {

/**
 * @class AudioClip
 * @brief Represents an audio file that can be played
 */
class AudioClip {
public:
    AudioClip();
    
    ~AudioClip();

    // Prevent copying (owns resources)
    AudioClip(const AudioClip&) = delete;
    AudioClip& operator=(const AudioClip&) = delete;
    
    // Allow moving
    AudioClip(AudioClip&& other) noexcept;
    
    AudioClip& operator=(AudioClip&& other) noexcept;

    /** Load audio as sound (for short effects) */
    bool LoadSound(const std::string& filepath);
    
    /** Load audio as music (for long background tracks) */
    bool LoadMusic(const std::string& filepath);
    
    /** Unload audio resource */
    void Unload();
    
    /** Check if audio is loaded */
    bool IsLoaded() const;
    
    /** Get internal AEAudio handle */
    AEAudio GetHandle() const;

private:
    AEAudio audio = { nullptr };
};

} // namespace Engine
