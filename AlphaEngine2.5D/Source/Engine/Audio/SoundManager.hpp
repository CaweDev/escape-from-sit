#pragma once

#include "AudioSource.hpp"
#include "Engine/Core/ConfigManager.hpp"
#include <string>
#include <unordered_map>

namespace Engine {

class SoundManager {
public:
    static void Initialize(const std::string& configPath = "Assets/Config/audio_config.json");
    static void Shutdown();

    static void Update(f32 deltaTime);

    static void PlayBGM(const std::string& key, bool loop = true);
    static void StopBGM();
    static void PlaySFX(const std::string& key, f32 volumeScale = 1.0f);

    static void SetBGMVolume(f32 volume);
    static void SetSFXVolume(f32 volume);

    static void UpdateDistanceLoop(const std::string& key, f32 distance, f32 nearDistance, f32 farDistance, f32 maxVolume = 1.0f);
    static void UpdateGuardProximityLoop(f32 nearestDistance);
    static void UpdateCCTVHumLoop(f32 nearestDistance);
    static void UpdateTerminalBuzzLoop(f32 nearestDistance);

private:
    enum class BGMFadeState {
        None,
        FadingOut,
        FadingIn
    };

    struct DistanceLoopConfig {
        f32 nearDistance = 2.5f;
        f32 farDistance = 10.0f;
        f32 maxVolume = 0.8f;
    };

    struct DistanceLoopChannel {
        AudioSource* source = nullptr;
        bool playing = false;
    };

    struct AudioConfig : public IConfig {
        std::unordered_map<std::string, std::string> bgm;
        std::unordered_map<std::string, std::string> sfx;
        f32 bgmVolume = 0.7f;
        f32 sfxVolume = 1.0f;
        f32 bgmFadeDuration = 0.8f;

        DistanceLoopConfig guardLoop;
        DistanceLoopConfig cctvLoop;
        DistanceLoopConfig terminalLoop;

        void FromJson(const Json::Value& json) override;
    };

    static void EnsureInitialized();
    static AudioClip* LoadMusicClip(const std::string& path);
    static AudioClip* LoadSoundClip(const std::string& path);

    static bool s_initialized;
    static AudioSource* s_bgmSource;
    static AudioSource* s_sfxSource;
    static std::unordered_map<std::string, AudioClip*> s_bgmClips;
    static std::unordered_map<std::string, AudioClip*> s_sfxClips;
    static std::string s_currentBGMKey;

    static f32 s_bgmBaseVolume;
    static f32 s_bgmFadeDuration;
    static f32 s_bgmFadeTimer;
    static f32 s_bgmFadeStartVolume;
    static BGMFadeState s_bgmFadeState;
    static std::string s_pendingBGMKey;
    static bool s_pendingBGMLoop;
    static bool s_stopAfterFadeOut;

    static std::unordered_map<std::string, DistanceLoopChannel> s_distanceLoops;
    static DistanceLoopConfig s_guardLoopConfig;
    static DistanceLoopConfig s_cctvLoopConfig;
    static DistanceLoopConfig s_terminalLoopConfig;

    static constexpr const char* kGuardLoopKey = "guard_proximity_loop";
    static constexpr const char* kCCTVHumLoopKey = "cctv_hum_loop";
    static constexpr const char* kTerminalBuzzLoopKey = "terminal_buzz_loop";
};

} // namespace Engine
