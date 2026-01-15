/**
 * @file AudioSource.hpp
 * @brief Component for playing audio
 */
#pragma once

#include "../Core/Component.hpp"
#include "AudioClip.hpp"
#include <memory>

namespace Engine {

/**
 * @class AudioSource
 * @brief Component that plays audio clips
 */
class AudioSource : public Component {
public:
    /** Audio clip to play */
    std::shared_ptr<AudioClip> clip;
    
    /** Volume (0 to 1) */
    f32 volume = 1.0f;
    
    /** Pitch (0 to 1, where 1 is normal) */
    f32 pitch = 1.0f;
    
    /** Whether to loop playback */
    bool loop = false;
    
    /** Play on awake */
    bool playOnAwake = false;

    AudioSource();
    ~AudioSource();

    void Awake() override;

    /** Play the audio clip */
    void Play();
    
    /** Play a specific clip (one-shot) */
    void PlayOneShot(std::shared_ptr<AudioClip> oneShot, f32 volumeScale = 1.0f);
    
    /** Stop playback */
    void Stop();
    
    /** Pause playback */
    void Pause();
    
    /** Resume playback */
    void Resume();
    
    /** Set volume (applies to group) */
    void SetVolume(f32 newVolume);
    
    /** Set pitch (applies to group) */
    void SetPitch(f32 newPitch);

private:
    AEAudioGroup group = { nullptr };
};

} // namespace Engine
