#include "SoundManager.hpp"
#include <cstdio>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <unordered_map>
#include <vector>
#include <cmath>
#include "Engine/Math/Mathf.hpp"

namespace Engine {

    bool SoundManager::s_initialized = false;
    AudioSource* SoundManager::s_bgmSource = nullptr;
    AudioSource* SoundManager::s_sfxSource = nullptr;
    std::unordered_map<std::string, AudioClip*> SoundManager::s_bgmClips;
    std::unordered_map<std::string, AudioClip*> SoundManager::s_sfxClips;
    std::string SoundManager::s_currentBGMKey;

    f32 SoundManager::s_bgmBaseVolume = 0.7f;
    f32 SoundManager::s_bgmFadeDuration = 0.8f;
    f32 SoundManager::s_bgmFadeTimer = 0.0f;
    f32 SoundManager::s_bgmFadeStartVolume = 0.7f;

    SoundManager::BGMFadeState SoundManager::s_bgmFadeState = SoundManager::BGMFadeState::None;
    std::string SoundManager::s_pendingBGMKey;
    bool SoundManager::s_pendingBGMLoop = true;
    bool SoundManager::s_stopAfterFadeOut = false;

    std::unordered_map<std::string, SoundManager::DistanceLoopChannel> SoundManager::s_distanceLoops;
    SoundManager::DistanceLoopConfig SoundManager::s_guardLoopConfig{};
    SoundManager::DistanceLoopConfig SoundManager::s_cctvLoopConfig{};
    SoundManager::DistanceLoopConfig SoundManager::s_terminalLoopConfig{};
    std::unordered_map<std::string, AudioSource*> SoundManager::s_loopChannels;
    std::unordered_map<std::string, int> SoundManager::s_loopRefCount;
    std::unordered_map<std::string, f32> SoundManager::s_loopLastVolume;
    std::unordered_map<std::string, f32> SoundManager::s_distanceLoopLastVolume;
    bool SoundManager::s_sfxEnabled = true;

    // Cache for loose filename resolution (requested -> actual path)
    static std::unordered_map<std::string, std::string> s_loosePathCache;

    // Normalize a filename for loose comparison: lowercase, remove non-alphanumeric
    static std::string NormalizeLoose(const std::string& s) {
        std::string out;
        out.reserve(s.size());
        for (unsigned char c : s) {
            if (std::isalnum(c)) out.push_back((char)std::tolower(c));
        }
        return out;
    }

    void SoundManager::SetSFXEnabled(bool enabled)
    {
        EnsureInitialized();
        if (s_sfxEnabled == enabled) return;
        s_sfxEnabled = enabled;
        if (!s_sfxEnabled) {
            StopAllSFX();
        }
    }

    bool SoundManager::IsSFXEnabled()
    {
        return s_sfxEnabled;
    }

    // Try to find a file under Assets/Audio whose filename (without extension) loosely
    // matches the requested name. Returns empty string if not found.
    static std::string FindLooseAudioFile(const std::string& requested)
    {
        namespace fs = std::filesystem;
        if (requested.empty()) return {};

        auto it = s_loosePathCache.find(requested);
        if (it != s_loosePathCache.end()) return it->second;

        std::string target = NormalizeLoose(requested);

        fs::path base("Assets/Audio");
        try {
            if (!fs::exists(base)) {
                s_loosePathCache[requested] = std::string();
                return {};
            }

            for (auto& pe : fs::recursive_directory_iterator(base)) {
                if (!pe.is_regular_file()) continue;
                std::string fname = pe.path().filename().string();
                std::string stem = pe.path().stem().string();
                std::string normStem = NormalizeLoose(stem);
                std::string normName = NormalizeLoose(fname);
                if (normStem == target || normName == target) {
                    std::string found = pe.path().generic_string();
                    s_loosePathCache[requested] = found;
                    return found;
                }
            }
        }
        catch (...) {
            // ignore filesystem errors
        }

        s_loosePathCache[requested] = std::string();
        return {};
    }

    void SoundManager::AudioConfig::FromJson(const Json::Value& json)
    {
        bgm.clear();
        sfx.clear();

        const Json::Value& bgmNode = json["bgm"];
        if (bgmNode.isObject()) {
            for (const auto& key : bgmNode.getMemberNames()) {
                bgm[key] = bgmNode[key].asString();
            }
        }

        const Json::Value& sfxNode = json["sfx"];
        if (sfxNode.isObject()) {
            for (const auto& key : sfxNode.getMemberNames()) {
                sfx[key] = sfxNode[key].asString();
            }
        }

        const Json::Value& settingsNode = json["settings"];
        const Json::Value& sourceNode = settingsNode.isObject() ? settingsNode : json;

        bgmVolume = sourceNode.get("bgmVolume", 0.7f).asFloat();
        sfxVolume = sourceNode.get("sfxVolume", 1.0f).asFloat();
        bgmFadeDuration = sourceNode.get("bgmFadeDuration", 0.8f).asFloat();

        const Json::Value& guardNode = sourceNode["guardLoop"];
        if (guardNode.isObject()) {
            guardLoop.nearDistance = guardNode.get("nearDistance", guardLoop.nearDistance).asFloat();
            guardLoop.farDistance = guardNode.get("farDistance", guardLoop.farDistance).asFloat();
            guardLoop.maxVolume = guardNode.get("maxVolume", guardLoop.maxVolume).asFloat();
        }

        const Json::Value& cctvNode = sourceNode["cctvLoop"];
        if (cctvNode.isObject()) {
            cctvLoop.nearDistance = cctvNode.get("nearDistance", cctvLoop.nearDistance).asFloat();
            cctvLoop.farDistance = cctvNode.get("farDistance", cctvLoop.farDistance).asFloat();
            cctvLoop.maxVolume = cctvNode.get("maxVolume", cctvLoop.maxVolume).asFloat();
        }

        const Json::Value& terminalNode = sourceNode["terminalLoop"];
        if (terminalNode.isObject()) {
            terminalLoop.nearDistance = terminalNode.get("nearDistance", terminalLoop.nearDistance).asFloat();
            terminalLoop.farDistance = terminalNode.get("farDistance", terminalLoop.farDistance).asFloat();
            terminalLoop.maxVolume = terminalNode.get("maxVolume", terminalLoop.maxVolume).asFloat();
        }
    }

    void SoundManager::Initialize(const std::string& configPath)
    {
        if (s_initialized) {
            return;
        }

        s_bgmSource = new AudioSource();
        s_sfxSource = new AudioSource();

        AudioConfig config;
        if (!ConfigManager::ReadConfig(configPath, config)) {
            std::printf("[Warning] SoundManager: Failed to read audio config: %s\n", configPath.c_str());
            std::printf("[Warning] SoundManager: Falling back to built-in default audio config\n");

            config.bgm.clear();
            config.sfx.clear();

            // BGM
            config.bgm["main_menu"] = "Assets/Audio/BGM/main_menu.mp3";
            config.bgm["fps_gameplay"] = "Assets/Audio/BGM/Game BGM.wav";
            config.bgm["credits"] = "Assets/Audio/BGM/credits.mp3";

            // SFX
            config.sfx["ui_click"] = "Assets/Audio/UI/UI click.wav";
            config.sfx["ui_hover"] = "Assets/Audio/UI/UI hover.wav";
            config.sfx["ui_toggle"] = "Assets/Audio/UI/UI click.wav";
            config.sfx["access_card"] = "Assets/Audio/AccessCardPickup/Access card.wav";
            config.sfx["card_collect"] = "Assets/Audio/AccessCardPickup/Access card.wav";
            config.sfx["book_nearby"] = "Assets/Audio/Puzzle/book_nearby.wav";
            config.sfx["terminal_hack"] = "Assets/Audio/Door/Access Denied.wav";
            config.sfx["cctv_disabled"] = "Assets/Audio/CCTV/CCTV shutdown.wav";
            config.sfx["door_open"] = "Assets/Audio/Door/Door open.wav";
            config.sfx["door_close"] = "Assets/Audio/Door/Door close.wav";
            config.sfx["door_locked"] = "Assets/Audio/Door/Door lock.wav";
            config.sfx["zone_enter"] = "Assets/Audio/UI/Notif.wav";
            config.sfx["cctv_alert"] = "Assets/Audio/CCTV/CCTV caught.wav";
            config.sfx["win"] = "Assets/Audio/UI/Game Win.wav";
            config.sfx["lose"] = "Assets/Audio/UI/Game Lose.wav";
            config.sfx["guard_proximity_loop"] = "Assets/Audio/Guard/guard nearby.wav";
            config.sfx["guard_chasing"] = "Assets/Audio/Guard/Guard Chasing.wav";
            config.sfx["cctv_hum_loop"] = "Assets/Audio/CCTV/CCTV ambience.wav";
            config.sfx["terminal_buzz_loop"] = "Assets/Audio/CCTV/Terminal buzz.mp3";
            config.sfx["annoying_classmate"] = "Assets/Audio/AnnoyingClassMate/Annoying Classmate.wav";
            config.sfx["full_escalation"] = "Assets/Audio/Guard/Full Escalation.wav";
            config.sfx["jump"] = "Assets/Audio/Player/jump.wav";
            config.sfx["player_run"] = "Assets/Audio/Player/player run.wav";
            config.sfx["player_walking"] = "Assets/Audio/Player/player walking.wav";
            config.sfx["push"] = "Assets/Audio/Bully/push.wav";
            config.sfx["npc_alert"] = "Assets/Audio/AnnoyingClassMate/Annoying Classmate.wav";
            config.sfx["all_books_collected"] = "Assets/Audio/Puzzle/Puzzle_Complete.wav";
        }

        if (config.bgmVolume < 0.0f) config.bgmVolume = 0.0f;
        if (config.bgmVolume > 1.0f) config.bgmVolume = 1.0f;
        if (config.sfxVolume < 0.0f) config.sfxVolume = 0.0f;
        if (config.sfxVolume > 1.0f) config.sfxVolume = 1.0f;
        if (config.bgmFadeDuration < 0.01f) config.bgmFadeDuration = 0.01f;

        auto normalizeLoopCfg = [](DistanceLoopConfig& cfg) {
            if (cfg.nearDistance < 0.0f) cfg.nearDistance = 0.0f;
            if (cfg.farDistance <= cfg.nearDistance) cfg.farDistance = cfg.nearDistance + 0.01f;
            if (cfg.maxVolume < 0.0f) cfg.maxVolume = 0.0f;
            if (cfg.maxVolume > 1.0f) cfg.maxVolume = 1.0f;
            };

        normalizeLoopCfg(config.guardLoop);
        normalizeLoopCfg(config.cctvLoop);
        normalizeLoopCfg(config.terminalLoop);

        s_guardLoopConfig = config.guardLoop;
        s_cctvLoopConfig = config.cctvLoop;
        s_terminalLoopConfig = config.terminalLoop;

        s_bgmBaseVolume = config.bgmVolume;
        s_bgmFadeDuration = config.bgmFadeDuration;

        for (const auto& [key, path] : config.bgm) {
            auto* clip = LoadMusicClip(path);
            if (clip && clip->IsLoaded()) {
                s_bgmClips[key] = clip;
            }
            else {
                std::printf("[Warning] SoundManager: Failed to load BGM '%s' from %s\n", key.c_str(), path.c_str());
                delete clip;
            }
        }

        for (const auto& [key, path] : config.sfx) {
            auto* clip = LoadSoundClip(path);
            if (!(clip && clip->IsLoaded())) {
                std::string resolved = FindLooseAudioFile(key);
                if (!resolved.empty()) {
                    std::printf("[Info] SoundManager: Attempting to load SFX '%s' via resolved path: %s\n", key.c_str(), resolved.c_str());
                    delete clip;
                    clip = LoadSoundClip(resolved);
                }
            }

            if (clip && clip->IsLoaded()) {
                s_sfxClips[key] = clip;
            }
            else {
                std::printf("[Warning] SoundManager: Failed to load SFX '%s' from %s\n", key.c_str(), path.c_str());
                delete clip;
            }
        }

        s_bgmSource->loop = true;
        s_bgmSource->volume = s_bgmBaseVolume;
        s_sfxSource->volume = config.sfxVolume;

        s_initialized = true;
    }

    void SoundManager::Shutdown()
    {
        if (s_bgmSource) s_bgmSource->Stop();
        if (s_sfxSource) s_sfxSource->Stop();

        for (auto& [_, channel] : s_distanceLoops) {
            if (channel.source) {
                channel.source->Stop();
                delete channel.source;
                channel.source = nullptr;
            }
        }
        s_distanceLoops.clear();

        for (auto& [k, src] : s_loopChannels) {
            if (src) {
                src->Stop();
                delete src;
            }
        }
        s_loopChannels.clear();
        s_loopRefCount.clear();

        for (auto& [_, clip] : s_bgmClips) delete clip;
        for (auto& [_, clip] : s_sfxClips) delete clip;

        s_bgmClips.clear();
        s_sfxClips.clear();
        s_currentBGMKey.clear();
        s_pendingBGMKey.clear();
        s_bgmFadeTimer = 0.0f;
        s_bgmFadeState = BGMFadeState::None;
        s_stopAfterFadeOut = false;

        try {
            s_loosePathCache.clear();
            s_loosePathCache.rehash(0);
        }
        catch (...) {}

        delete s_bgmSource;
        delete s_sfxSource;
        s_bgmSource = nullptr;
        s_sfxSource = nullptr;

        s_initialized = false;
    }

    void SoundManager::Update(f32 deltaTime)
    {
        EnsureInitialized();

        if (!s_bgmSource || deltaTime <= 0.0f) return;
        if (s_bgmFadeState == BGMFadeState::None) return;

        // Clamp deltaTime to avoid abrupt fades during large frame drops (e.g. scene loading)
        if (deltaTime > 0.1f) deltaTime = 0.1f;

        s_bgmFadeTimer += deltaTime;
        f32 t = s_bgmFadeTimer / s_bgmFadeDuration;
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;

        if (s_bgmFadeState == BGMFadeState::FadingOut) {
            s_bgmSource->SetVolume(s_bgmFadeStartVolume * (1.0f - t));

            if (t >= 1.0f) {
                s_bgmSource->Stop();
                s_currentBGMKey.clear();

                if (s_stopAfterFadeOut || s_pendingBGMKey.empty()) {
                    s_pendingBGMKey.clear();
                    s_stopAfterFadeOut = false;
                    s_bgmFadeState = BGMFadeState::None;
                    s_bgmSource->SetVolume(s_bgmBaseVolume);
                    return;
                }

                auto it = s_bgmClips.find(s_pendingBGMKey);
                if (it == s_bgmClips.end() || !it->second || !it->second->IsLoaded()) {
                    std::printf("[Warning] SoundManager: Pending BGM key not found/loaded: %s\n", s_pendingBGMKey.c_str());
                    s_pendingBGMKey.clear();
                    s_bgmFadeState = BGMFadeState::None;
                    s_bgmSource->SetVolume(s_bgmBaseVolume);
                    return;
                }

                s_bgmSource->loop = s_pendingBGMLoop;
                s_bgmSource->clip = it->second;
                s_bgmSource->volume = s_bgmBaseVolume;
                s_bgmSource->Play();
                s_bgmSource->SetVolume(0.0f);
                s_currentBGMKey = s_pendingBGMKey;
                s_pendingBGMKey.clear();

                s_bgmFadeState = BGMFadeState::FadingIn;
                s_bgmFadeTimer = 0.0f;
                return;
            }
        }

        if (s_bgmFadeState == BGMFadeState::FadingIn) {
            s_bgmSource->SetVolume(s_bgmBaseVolume * t);

            if (t >= 1.0f) {
                s_bgmSource->SetVolume(s_bgmBaseVolume);
                s_bgmFadeState = BGMFadeState::None;
            }
        }
    }

    void SoundManager::PlayBGM(const std::string& key, bool loop)
    {
        EnsureInitialized();

        auto it = s_bgmClips.find(key);
        if (it == s_bgmClips.end()) {
            std::printf("[Warning] SoundManager: BGM key not found: %s\n", key.c_str());
            return;
        }

        if (!s_bgmSource) return;

        if (s_currentBGMKey == key && s_bgmFadeState != BGMFadeState::FadingOut) return;

        if (s_currentBGMKey.empty()) {
            s_bgmSource->loop = loop;
            s_bgmSource->clip = it->second;
            s_bgmSource->volume = s_bgmBaseVolume;
            s_bgmSource->Play();
            s_bgmSource->SetVolume(0.0f);
            s_currentBGMKey = key;

            s_bgmFadeState = BGMFadeState::FadingIn;
            s_bgmFadeTimer = 0.0f;
            s_stopAfterFadeOut = false;
            s_pendingBGMKey.clear();
            return;
        }

        s_pendingBGMKey = key;
        s_pendingBGMLoop = loop;
        s_stopAfterFadeOut = false;
        s_bgmFadeStartVolume = s_bgmSource->volume;
        s_bgmFadeTimer = 0.0f;
        s_bgmFadeState = BGMFadeState::FadingOut;
    }

    void SoundManager::StopBGM()
    {
        if (!s_bgmSource || s_currentBGMKey.empty()) {
            s_currentBGMKey.clear();
            return;
        }

        s_pendingBGMKey.clear();
        s_stopAfterFadeOut = true;
        s_bgmFadeStartVolume = s_bgmSource->volume;
        s_bgmFadeTimer = 0.0f;
        s_bgmFadeState = BGMFadeState::FadingOut;
    }

    void SoundManager::PlaySFX(const std::string& key, f32 volumeScale)
    {
        EnsureInitialized();

        if (!s_sfxEnabled) return;

        auto it = s_sfxClips.find(key);
        if (it == s_sfxClips.end()) {
            std::printf("[Warning] SoundManager: SFX key not found: %s\n", key.c_str());
            return;
        }

        if (!s_sfxSource) return;
        s_sfxSource->PlayOneShot(it->second, volumeScale);
    }

    void SoundManager::StopAllSFX()
    {
        if (s_sfxSource) s_sfxSource->Stop();

        for (auto& [_, channel] : s_distanceLoops) {
            if (channel.source) {
                channel.source->Stop();
                delete channel.source;
                channel.source = nullptr;
            }
            channel.playing = false;
        }
        s_distanceLoops.clear();

        for (auto& [k, src] : s_loopChannels) {
            if (src) {
                src->Stop();
                delete src;
            }
        }
        s_loopChannels.clear();
        s_loopRefCount.clear();
        s_loopLastVolume.clear();
        s_distanceLoopLastVolume.clear();
    }


    void SoundManager::SetBGMVolume(f32 volume)
    {
        if (volume < 0.0f) volume = 0.0f;
        if (volume > 1.0f) volume = 1.0f;

        s_bgmBaseVolume = volume;

        if (s_bgmSource && s_bgmFadeState == BGMFadeState::None) {
            s_bgmSource->SetVolume(volume);
        }
    }

    void SoundManager::SetSFXVolume(f32 volume)
    {
        if (!s_sfxSource) return;

        if (volume < 0.0f) volume = 0.0f;
        if (volume > 1.0f) volume = 1.0f;

        s_sfxSource->SetVolume(volume);
    }

    void SoundManager::PlayLoop(const std::string& key, f32 volume)
    {
        EnsureInitialized();

        if (!s_sfxEnabled) return;

        auto it = s_sfxClips.find(key);
        if (it == s_sfxClips.end() || !it->second || !it->second->IsLoaded()) {
            AudioClip* dynamicClip = LoadSoundClip(key);
            if (!dynamicClip) {
                std::string resolved = FindLooseAudioFile(key);
                if (!resolved.empty()) dynamicClip = LoadSoundClip(resolved);
            }

            if (dynamicClip && dynamicClip->IsLoaded()) {
                s_sfxClips[key] = dynamicClip;
                it = s_sfxClips.find(key);
            }
            else {
                delete dynamicClip;
                return;
            }
        }

        auto lit = s_loopChannels.find(key);
        if (lit == s_loopChannels.end() || !lit->second) {
            AudioSource* src = new AudioSource();
            src->loop = true;
            src->clip = it->second;

            f32 masterSfx = s_sfxSource ? s_sfxSource->volume : 1.0f;
            f32 finalVolume = volume * masterSfx;
            f32 minFinal = Mathf::Max(volume * 0.5f, 0.25f);
            if (finalVolume < minFinal) finalVolume = minFinal;

            src->SetVolume(finalVolume);
            src->Play();
            s_loopChannels[key] = src;
            s_loopRefCount[key] = 1;
            s_loopLastVolume[key] = finalVolume;
        }
        else {
            // Increase reference count so multiple callers can request the same
            // looping SFX without it being stopped prematurely by another caller.
            s_loopRefCount[key]++;

            f32 masterSfx = s_sfxSource ? s_sfxSource->volume : 1.0f;
            f32 finalVolume = volume * masterSfx;
            f32 minFinal = Mathf::Max(volume * 0.5f, 0.25f);
            if (finalVolume < minFinal) finalVolume = minFinal;

            f32 prev = 0.0f;
            auto pvIt = s_loopLastVolume.find(key);
            if (pvIt != s_loopLastVolume.end()) prev = pvIt->second;

            const f32 EPS = 0.01f;
            if (std::fabs(finalVolume - prev) >= EPS) {
                lit->second->SetVolume(finalVolume);
                s_loopLastVolume[key] = finalVolume;
            }
        }
    }

    void SoundManager::StopLoop(const std::string& key)
    {
        auto lit = s_loopChannels.find(key);
        if (lit == s_loopChannels.end() || !lit->second) return;

        auto rit = s_loopRefCount.find(key);
        if (rit != s_loopRefCount.end()) {
            rit->second--;
            if (rit->second > 0) return;
        }

        lit->second->Stop();
        delete lit->second;
        s_loopChannels.erase(lit);
        s_loopRefCount.erase(key);
    }

    void SoundManager::UpdateDistanceLoop(const std::string& key, f32 distance, f32 nearDistance, f32 farDistance, f32 maxVolume)
    {
        EnsureInitialized();

        if (!s_sfxEnabled) {
            StopDistanceLoop(key);
            return;
        }

        if (nearDistance < 0.0f) nearDistance = 0.0f;
        if (farDistance <= nearDistance) farDistance = nearDistance + 0.01f;
        if (maxVolume < 0.0f) maxVolume = 0.0f;
        if (maxVolume > 1.0f) maxVolume = 1.0f;

        if (!std::isfinite(distance) || distance < 0.0f) {
            distance = farDistance + 1.0f;
        }

        auto clipIt = s_sfxClips.find(key);
        if (clipIt == s_sfxClips.end() || !clipIt->second || !clipIt->second->IsLoaded()) {
            AudioClip* dynamicClip = LoadSoundClip(key);
            if (!dynamicClip) {
                std::string resolved = FindLooseAudioFile(key);
                if (!resolved.empty()) dynamicClip = LoadSoundClip(resolved);
            }

            if (dynamicClip && dynamicClip->IsLoaded()) {
                s_sfxClips[key] = dynamicClip;
                clipIt = s_sfxClips.find(key);
            }
            else {
                delete dynamicClip;
                return;
            }
        }

        f32 t = 0.0f;
        if (distance <= nearDistance) {
            t = 1.0f;
        }
        else if (distance >= farDistance) {
            t = 0.0f;
        }
        else {
            t = (farDistance - distance) / (farDistance - nearDistance);
            if (t < 0.0f) t = 0.0f;
            if (t > 1.0f) t = 1.0f;
        }

        // Smoothstep attenuation for less abrupt perceived transitions.
        t = t * t * (3.0f - 2.0f * t);

        f32 masterSfx = s_sfxSource ? s_sfxSource->volume : 1.0f;
        f32 finalVol = maxVolume * t * masterSfx;

        const f32 STOP_THRESHOLD = 0.001f;
        if (finalVol <= STOP_THRESHOLD) {
            auto it = s_distanceLoops.find(key);
            if (it != s_distanceLoops.end()) {
                if (it->second.source) {
                    it->second.source->Stop();
                    delete it->second.source;
                    it->second.source = nullptr;
                }
                it->second.playing = false;
                s_distanceLoopLastVolume.erase(key);
                s_distanceLoops.erase(it);
            }
            auto lit = s_loopChannels.find(key);
            if (lit != s_loopChannels.end()) {
                if (lit->second) { lit->second->Stop(); delete lit->second; }
                s_loopChannels.erase(lit);
                s_loopRefCount.erase(key);
                s_loopLastVolume.erase(key);
            }
            return;
        }

        auto& channel = s_distanceLoops[key];
        if (!channel.source) {
            channel.source = new AudioSource();
            channel.source->loop = true;
            channel.source->clip = clipIt->second;
            channel.playing = false;
        }

        f32 prev = 0.0f;
        auto itv = s_distanceLoopLastVolume.find(key);
        if (itv != s_distanceLoopLastVolume.end()) prev = itv->second;

        // Robust restart: when re-entering audible range from (near) silence,
        // force a restart so the loop reliably resumes.
        const f32 RESTART_THRESHOLD = STOP_THRESHOLD * 4.0f;
        const bool reenteringAudibleRange = (prev <= STOP_THRESHOLD && finalVol > RESTART_THRESHOLD);
        if (!channel.playing || reenteringAudibleRange) {
            channel.source->clip = clipIt->second;
            channel.source->loop = true;
            if (reenteringAudibleRange) {
                channel.source->Stop();
            }
            channel.source->Play();
            channel.playing = true;
            channel.source->SetVolume(finalVol);
        }

        const f32 EPS = 0.001f;
        if (std::fabs(finalVol - prev) >= EPS) {
            channel.source->SetVolume(finalVol);
        }

        // Keep this updated every frame so restart detection remains accurate.
        s_distanceLoopLastVolume[key] = finalVol;
    }

    void SoundManager::UpdateGuardProximityLoop(f32 nearestDistance)
    {
        UpdateDistanceLoop(kGuardLoopKey, nearestDistance, s_guardLoopConfig.nearDistance, s_guardLoopConfig.farDistance, s_guardLoopConfig.maxVolume);
    }

    void SoundManager::UpdateCCTVHumLoop(f32 nearestDistance)
    {
        UpdateDistanceLoop(kCCTVHumLoopKey, nearestDistance, s_cctvLoopConfig.nearDistance, s_cctvLoopConfig.farDistance, s_cctvLoopConfig.maxVolume);
    }

    void SoundManager::UpdateTerminalBuzzLoop(f32 nearestDistance)
    {
        UpdateDistanceLoop(kTerminalBuzzLoopKey, nearestDistance, s_terminalLoopConfig.nearDistance, s_terminalLoopConfig.farDistance, s_terminalLoopConfig.maxVolume);
    }

    void SoundManager::UpdateNPCAnnoyingLoop(f32 nearestDistance)
    {
        const f32 nearDist = 1.0f;
        const f32 farDist = 6.0f;
        const f32 maxVol = 0.8f;

        if (!s_sfxEnabled) {
            StopDistanceLoop("annoying_classmate");
            return;
        }

        UpdateDistanceLoop("annoying_classmate", nearestDistance, nearDist, farDist, maxVol);

        f32 t = 0.0f;
        if (nearestDistance <= nearDist) {
            t = 1.0f;
        }
        else if (nearestDistance >= farDist) {
            t = 0.0f;
        }
        else {
            t = (farDist - nearestDistance) / (farDist - nearDist);
            if (t < 0.0f) t = 0.0f;
            if (t > 1.0f) t = 1.0f;
        }

        const f32 masterSfx = s_sfxSource ? s_sfxSource->volume : 1.0f;
        const f32 finalVol = maxVol * t * masterSfx;

        const f32 STOP_THRESHOLD = 0.005f;
        if (finalVol <= STOP_THRESHOLD) {
            StopDistanceLoop("annoying_classmate");
        }
    }

    void SoundManager::StopDistanceLoop(const std::string& key)
    {
        auto it = s_distanceLoops.find(key);
        if (it != s_distanceLoops.end()) {
            if (it->second.source) {
                it->second.source->Stop();
                delete it->second.source;
                it->second.source = nullptr;
            }
            it->second.playing = false;
            s_distanceLoopLastVolume.erase(key);
            s_distanceLoops.erase(it);
            std::printf("[SoundManager] StopDistanceLoop: removed distance loop '%s'\n", key.c_str());
        }

        auto lit = s_loopChannels.find(key);
        if (lit != s_loopChannels.end()) {
            if (lit->second) {
                lit->second->Stop();
                delete lit->second;
            }
            s_loopChannels.erase(lit);
            s_loopRefCount.erase(key);
            s_loopLastVolume.erase(key);
            std::printf("[SoundManager] StopDistanceLoop: removed generic loop channel '%s'\n", key.c_str());
        }

        AudioClip* targetClip = nullptr;
        auto cit = s_sfxClips.find(key);
        if (cit != s_sfxClips.end()) targetClip = cit->second;
        if (targetClip) {
            std::vector<std::string> toRemove;
            for (auto& [lk, src] : s_loopChannels) {
                if (!src) continue;
                if (src->clip == targetClip) {
                    src->Stop();
                    delete src;
                    toRemove.push_back(lk);
                }
            }
            for (auto& k : toRemove) {
                s_loopChannels.erase(k);
                s_loopRefCount.erase(k);
                s_loopLastVolume.erase(k);
                std::printf("[SoundManager] StopDistanceLoop: removed generic loop channel by clip match '%s'\n", k.c_str());
            }
        }
    }

    const SoundManager::DistanceLoopConfig& SoundManager::GetGuardLoopConfig()
    {
        return s_guardLoopConfig;
    }

    const SoundManager::DistanceLoopConfig& SoundManager::GetCCTVLoopConfig()
    {
        return s_cctvLoopConfig;
    }

    const SoundManager::DistanceLoopConfig& SoundManager::GetTerminalLoopConfig()
    {
        return s_terminalLoopConfig;
    }

    void SoundManager::EnsureInitialized()
    {
        if (!s_initialized) Initialize();
    }

    AudioClip* SoundManager::LoadMusicClip(const std::string& path)
    {
        namespace fs = std::filesystem;

        auto tryLoad = [&](const std::string& p) -> AudioClip* {
            auto* c = new AudioClip();
            if (c->LoadMusic(p)) return c;
            delete c;
            return nullptr;
            };

        if (auto* clip = tryLoad(path)) return clip;

        // Loose filename resolution first
        std::string loose = FindLooseAudioFile(fs::path(path).filename().string());
        if (!loose.empty()) {
            std::printf("[Info] SoundManager: Resolved loose music path '%s' -> '%s'\n", path.c_str(), loose.c_str());
            if (auto* clip = tryLoad(loose)) return clip;
        }

        fs::path fp(path);
        std::string filename = fp.filename().string();
        std::string ext = fp.extension().string();

        std::vector<std::string> exts;
        if (!ext.empty()) exts.push_back(ext);
        for (const auto& e : std::vector<std::string>{ ".wav", ".ogg", ".mp3", ".flac" })
            if (std::find(exts.begin(), exts.end(), e) == exts.end()) exts.push_back(e);

        auto replaceChar = [](const std::string& s, char a, char b) {
            std::string r = s; std::replace(r.begin(), r.end(), a, b); return r;
            };
        auto toLower = [](const std::string& s) {
            std::string r = s;
            std::transform(r.begin(), r.end(), r.begin(), [](unsigned char c) { return (char)std::tolower(c); });
            return r;
            };

        std::vector<std::string> nameVars = {
            filename,
            replaceChar(filename, ' ', '_'),
            replaceChar(filename, ' ', '-'),
            toLower(filename),
            toLower(replaceChar(filename, ' ', '_')),
            toLower(replaceChar(filename, ' ', '-'))
        };
        std::sort(nameVars.begin(), nameVars.end());
        nameVars.erase(std::unique(nameVars.begin(), nameVars.end()), nameVars.end());

        const std::vector<std::string> subfolders = {
            "BGM", "Guard", "AnnoyingClassMate", "Bully", "CCTV", "Door", "Player",
            "AccessCardPickup", "powerup", "Puzzle", "UI"
        };

        std::vector<std::string> candidates;
        for (const auto& nv : nameVars) {
            for (const auto& e : exts) {
                fs::path p(nv); p.replace_extension(e);
                candidates.push_back("Assets/Audio/BGM/" + p.string());
                candidates.push_back("Assets/Audio/" + p.string());
            }
        }
        for (const auto& sub : subfolders)
            for (const auto& nv : nameVars)
                candidates.push_back("Assets/Audio/" + sub + "/" + nv);
        for (const auto& nv : nameVars) {
            candidates.push_back("Assets/sound/" + nv);
            candidates.push_back("Assets/Audio/SFX/" + nv);
            candidates.push_back(nv);
        }

        for (const auto& cpath : candidates) {
            if (auto* clip = tryLoad(cpath)) {
                std::printf("[Info] SoundManager: Loaded music via fallback path: %s\n", cpath.c_str());
                return clip;
            }
        }

        std::printf("[Warning] SoundManager: LoadMusic failed for '%s' and all fallback candidates\n", path.c_str());
        return nullptr;
    }

    AudioClip* SoundManager::LoadSoundClip(const std::string& path)
    {
        namespace fs = std::filesystem;

        auto tryLoad = [&](const std::string& p) -> AudioClip* {
            auto* c = new AudioClip();
            if (c->LoadSound(p)) return c;
            delete c;
            return nullptr;
            };

        if (auto* clip = tryLoad(path)) return clip;

        fs::path fp(path);
        std::string filename = fp.filename().string();
        std::string ext = fp.extension().string();

        std::vector<std::string> exts;
        if (!ext.empty()) exts.push_back(ext);
        for (const auto& e : std::vector<std::string>{ ".wav", ".ogg", ".mp3", ".flac" })
            if (std::find(exts.begin(), exts.end(), e) == exts.end()) exts.push_back(e);

        auto replaceChar = [](const std::string& s, char a, char b) {
            std::string r = s; std::replace(r.begin(), r.end(), a, b); return r;
            };
        auto toLower = [](const std::string& s) {
            std::string r = s;
            std::transform(r.begin(), r.end(), r.begin(), [](unsigned char c) { return (char)std::tolower(c); });
            return r;
            };

        std::vector<std::string> nameVars = {
            filename,
            replaceChar(filename, ' ', '_'),
            replaceChar(filename, ' ', '-'),
            toLower(filename),
            toLower(replaceChar(filename, ' ', '_')),
            toLower(replaceChar(filename, ' ', '-'))
        };
        std::sort(nameVars.begin(), nameVars.end());
        nameVars.erase(std::unique(nameVars.begin(), nameVars.end()), nameVars.end());

        const std::vector<std::string> subfolders = {
            "Guard", "AnnoyingClassMate", "Bully", "CCTV", "Door", "Player",
            "AccessCardPickup", "powerup", "Puzzle", "UI", "BGM"
        };

        std::vector<std::string> candidates;
        for (const auto& sub : subfolders)
            for (const auto& nv : nameVars)
                candidates.push_back("Assets/Audio/" + sub + "/" + nv);
        for (const auto& nv : nameVars) {
            candidates.push_back("Assets/Audio/" + nv);
            candidates.push_back("Assets/sound/" + nv);
            candidates.push_back("Assets/Audio/SFX/" + nv);
            candidates.push_back(nv);
        }

        for (const auto& cpath : candidates) {
            if (auto* clip = tryLoad(cpath)) {
                std::printf("[Info] SoundManager: Loaded SFX via fallback path: %s\n", cpath.c_str());
                return clip;
            }
        }

        std::printf("[Warning] SoundManager: LoadSound failed for '%s' and all fallback candidates\n", path.c_str());
        return nullptr;
    }

} // namespace Engine

// Auto-cleanup to ensure resources are freed if the application doesn't
// explicitly call SoundManager::Shutdown().
namespace Engine {
    struct SoundManagerAutoShutdown {
        ~SoundManagerAutoShutdown() { SoundManager::Shutdown(); }
    };
    static SoundManagerAutoShutdown s_soundManagerAutoShutdown;
} // namespace Engine