#include "SoundManager.hpp"

#include <cstdio>
#include <filesystem>

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

        // Provide a sensible default config so audio still works when the JSON file is missing
        config.bgm.clear();
        config.sfx.clear();

        // BGM (Assets/Audio/BGM)
        config.bgm["main_menu"] = "Assets/Audio/BGM/main_menu.mp3";
        config.bgm["fps_gameplay"] = "Assets/Audio/BGM/Game BGM.wav";
        config.bgm["credits"] = "Assets/Audio/BGM/credits.mp3";

        // SFX (common keys used throughout the game)
        // SFX (point to nested folder)
        // SFX (Assets/Audio/SFX)
        config.sfx["ui_click"] = "Assets/Audio/SFX/ui_click.wav";
        config.sfx["ui_hover"] = "Assets/Audio/SFX/ui_hover.wav";
        config.sfx["ui_toggle"] = "Assets/Audio/SFX/ui_toggle.wav";
        config.sfx["access_card"] = "Assets/Audio/SFX/Access card.wav";
        config.sfx["card_collect"] = "Assets/Audio/SFX/Book Pickup.wav";
        config.sfx["terminal_hack"] = "Assets/Audio/SFX/Access Denied.wav";
        config.sfx["cctv_disabled"] = "Assets/Audio/SFX/CCTV shutdown.wav";
        config.sfx["door_open"] = "Assets/Audio/SFX/Door open.wav";
        config.sfx["door_close"] = "Assets/Audio/SFX/Door close.wav";
        config.sfx["door_locked"] = "Assets/Audio/SFX/Access Denied.wav";
        config.sfx["zone_enter"] = "Assets/Audio/SFX/zone_enter.wav";
        config.sfx["cctv_alert"] = "Assets/Audio/SFX/CCTV caught.wav";
        config.sfx["win"] = "Assets/Audio/SFX/win.wav";
        config.sfx["lose"] = "Assets/Audio/SFX/lose.wav";
        config.sfx["guard_proximity_loop"] = "Assets/Audio/SFX/guard nearby.wav";
        config.sfx["guard_chasing"] = "Assets/Audio/SFX/Guard Chasing.wav";
        config.sfx["cctv_hum_loop"] = "Assets/Audio/SFX/CCTV ambience.wav";
        config.sfx["terminal_buzz_loop"] = "Assets/Audio/SFX/Terminal buzz.wav";
        config.sfx["annoying_classmate"] = "Assets/Audio/SFX/Annoying Classmate.wav";
        config.sfx["full_escalation"] = "Assets/Audio/SFX/Full Escalation.wav";
        config.sfx["jump"] = "Assets/Audio/SFX/jump.wav";
        config.sfx["player_run"] = "Assets/Audio/SFX/player run.wav";
        config.sfx["player_walking"] = "Assets/Audio/SFX/player walking.wav";
        config.sfx["push"] = "Assets/Audio/SFX/push.wav";
        config.sfx["npc_alert"] = "Assets/Audio/SFX/NPC Alert.wav";

        // settings: keep defaults already present in AudioConfig implementation
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
        } else {
            std::printf("[Warning] SoundManager: Failed to load BGM '%s' from %s\n", key.c_str(), path.c_str());
            delete clip;
        }
    }

    for (const auto& [key, path] : config.sfx) {
        auto* clip = LoadSoundClip(path);
        if (clip && clip->IsLoaded()) {
            s_sfxClips[key] = clip;
        } else {
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

    for (auto& [_, clip] : s_bgmClips) {
        delete clip;
    }
    for (auto& [_, clip] : s_sfxClips) {
        delete clip;
    }

    s_bgmClips.clear();
    s_sfxClips.clear();
    s_currentBGMKey.clear();
    s_pendingBGMKey.clear();
    s_bgmFadeTimer = 0.0f;
    s_bgmFadeState = BGMFadeState::None;
    s_stopAfterFadeOut = false;

    delete s_bgmSource;
    delete s_sfxSource;
    s_bgmSource = nullptr;
    s_sfxSource = nullptr;

    s_initialized = false;
}

void SoundManager::Update(f32 deltaTime)
{
    EnsureInitialized();

    if (!s_bgmSource || deltaTime <= 0.0f) {
        return;
    }

    if (s_bgmFadeState == BGMFadeState::None) {
        return;
    }

    // Clamp deltaTime to avoid abrupt fades during large frame drops (e.g., scene loading)
    if (deltaTime > 0.1f) {
        deltaTime = 0.1f;
    }

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

    if (s_currentBGMKey == key && s_bgmFadeState != BGMFadeState::FadingOut) {
        return;
    }

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
    if (s_sfxSource) {
        s_sfxSource->Stop();
    }

    for (auto& [_, channel] : s_distanceLoops) {
        if (channel.source) {
            channel.source->Stop();
        }
        channel.playing = false;
    }
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
    if (!s_sfxSource) {
        return;
    }

    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;

    s_sfxSource->SetVolume(volume);
}

void SoundManager::UpdateDistanceLoop(const std::string& key, f32 distance, f32 nearDistance, f32 farDistance, f32 maxVolume)
{
    EnsureInitialized();

    if (nearDistance < 0.0f) nearDistance = 0.0f;
    if (farDistance <= nearDistance) farDistance = nearDistance + 0.01f;
    if (maxVolume < 0.0f) maxVolume = 0.0f;
    if (maxVolume > 1.0f) maxVolume = 1.0f;

    auto clipIt = s_sfxClips.find(key);
    if (clipIt == s_sfxClips.end() || !clipIt->second || !clipIt->second->IsLoaded()) {
        return;
    }

    auto& channel = s_distanceLoops[key];
    if (!channel.source) {
        channel.source = new AudioSource();
        channel.source->loop = true;
        channel.source->clip = clipIt->second;
        channel.source->SetVolume(0.0f);
    }

    if (distance < 0.0f || distance >= farDistance) {
        if (channel.playing) {
            channel.source->Stop();
            channel.playing = false;
        }
        return;
    }

    if (!channel.playing) {
        channel.source->clip = clipIt->second;
        channel.source->loop = true;
        channel.source->volume = 1.0f;
        channel.source->Play();
        channel.source->SetVolume(0.0f);
        channel.playing = true;
    }

    f32 t = 0.0f;
    if (distance <= nearDistance) {
        t = 1.0f;
    } else {
        t = (farDistance - distance) / (farDistance - nearDistance);
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;
    }

    channel.source->SetVolume(maxVolume * t);
}

void SoundManager::UpdateGuardProximityLoop(f32 nearestDistance)
{
    UpdateDistanceLoop(kGuardLoopKey, nearestDistance, s_guardLoopConfig.nearDistance, s_guardLoopConfig.farDistance, s_guardLoopConfig.maxVolume);
}

void SoundManager::UpdateCCTVHumLoop(f32 nearestDistance)
{
    UpdateDistanceLoop(kCCTVHumLoopKey, nearestDistance, s_cctvLoopConfig.nearDistance, s_cctvLoopConfig.farDistance, s_cctvLoopConfig.maxVolume);
}

void SoundManager::UpdateNPCAnnoyingLoop(f32 nearestDistance)
{
    // Use terminal loop config for NPCs as a placeholder if not tuned separately
    UpdateDistanceLoop("annoying_classmate", nearestDistance, s_terminalLoopConfig.nearDistance, s_terminalLoopConfig.farDistance, 0.9f);
}

void SoundManager::UpdateTerminalBuzzLoop(f32 nearestDistance)
{
    UpdateDistanceLoop(kTerminalBuzzLoopKey, nearestDistance, s_terminalLoopConfig.nearDistance, s_terminalLoopConfig.farDistance, s_terminalLoopConfig.maxVolume);
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
    if (!s_initialized) {
        Initialize();
    }
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

    // Try original path first
    if (auto* clip = tryLoad(path)) return clip;

    // Build fallback candidates based on filename
    fs::path fp(path);
    std::string filename = fp.filename().string();

    std::vector<std::string> candidates;
    // Try project Audio BGM and generic Audio folders first
    candidates.push_back(std::string("Assets/Audio/BGM/") + filename);
    candidates.push_back(std::string("Assets/Audio/") + filename);

    // Try common subfolders inside Assets/Audio (project uses categorized folders)
    const std::vector<std::string> audioSubfolders = {
        "BGM", "Guard", "AnnoyingClassMate", "Bully", "CCTV", "Door", "Player",
        "AccessCardPickup", "powerup", "Puzzle", "UI"
    };
    for (const auto& sub : audioSubfolders) {
        candidates.push_back(std::string("Assets/Audio/") + sub + "/" + filename);
    }

    // legacy locations
    candidates.push_back(std::string("Assets/sound/") + filename);
    candidates.push_back(std::string("Assets/Audio/SFX/") + filename);

    for (const auto& cpath : candidates) {
        if (auto* clip = tryLoad(cpath)) {
            std::printf("[Info] SoundManager: Loaded music via fallback path: %s\n", cpath.c_str());
            return clip;
        }
    }

    std::printf("[Warning] SoundManager: LoadMusic failed for '%s' and fallback candidates\n", path.c_str());
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

    // Try original path first
    if (auto* clip = tryLoad(path)) return clip;

    // Build fallback candidates based on filename
    fs::path fp(path);
    std::string filename = fp.filename().string();

    std::vector<std::string> candidates;

    // Prefer specific categorized subfolders inside Assets/Audio
    const std::vector<std::string> audioSubfolders = {
        "Guard", "AnnoyingClassMate", "Bully", "CCTV", "Door", "Player",
        "AccessCardPickup", "powerup", "Puzzle", "UI", "BGM"
    };
    for (const auto& sub : audioSubfolders) {
        candidates.push_back(std::string("Assets/Audio/") + sub + "/" + filename);
    }

    // Then generic Audio and legacy locations
    candidates.push_back(std::string("Assets/Audio/") + filename);
    candidates.push_back(std::string("Assets/sound/") + filename);
    candidates.push_back(std::string("Assets/Audio/SFX/") + filename);

    for (const auto& cpath : candidates) {
        if (auto* clip = tryLoad(cpath)) {
            std::printf("[Info] SoundManager: Loaded SFX via fallback path: %s\n", cpath.c_str());
            return clip;
        }
    }

    std::printf("[Warning] SoundManager: LoadSound failed for '%s' and fallback candidates\n", path.c_str());
    return nullptr;
}

} // namespace Engine
