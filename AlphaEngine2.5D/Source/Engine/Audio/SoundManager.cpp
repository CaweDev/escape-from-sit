#include "SoundManager.hpp"

#include <cstdio>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <unordered_map>
#include <vector>

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
    } catch (...) {
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
        if (!(clip && clip->IsLoaded())) {
            // Try resolving by key (loose filename search)
            std::string resolved = FindLooseAudioFile(key);
            if (!resolved.empty()) {
                std::printf("[Info] SoundManager: Attempting to load SFX '%s' via resolved path: %s\n", key.c_str(), resolved.c_str());
                delete clip;
                clip = LoadSoundClip(resolved);
            }
        }

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

    // Stop and free any generic loop channels
    for (auto& [k, src] : s_loopChannels) {
        if (src) {
            src->Stop();
            delete src;
        }
    }
    s_loopChannels.clear();
    s_loopRefCount.clear();

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

    // Clear any resolver cache used for loose filename matching to free memory
    try {
        s_loosePathCache.clear();
        s_loosePathCache.rehash(0);
    } catch (...) {}

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

void SoundManager::PlayLoop(const std::string& key, f32 volume)
{
    EnsureInitialized();

    auto it = s_sfxClips.find(key);
    if (it == s_sfxClips.end() || !it->second || !it->second->IsLoaded()) return;

    auto lit = s_loopChannels.find(key);
    if (lit == s_loopChannels.end() || !lit->second) {
        AudioSource* src = new AudioSource();
        src->loop = true;
        src->clip = it->second;
        // Apply master SFX volume so generic loops respect global SFX settings
        f32 masterSfx = s_sfxSource ? s_sfxSource->volume : 1.0f;
        src->SetVolume(volume * masterSfx);
        src->Play();
        s_loopChannels[key] = src;
        s_loopRefCount[key] = 1;
    } else {
        // Already playing; bump refcount and update volume
        s_loopRefCount[key]++;
        f32 masterSfx = s_sfxSource ? s_sfxSource->volume : 1.0f;
        lit->second->SetVolume(volume * masterSfx);
    }
}

void SoundManager::StopLoop(const std::string& key)
{
    auto lit = s_loopChannels.find(key);
    if (lit == s_loopChannels.end() || !lit->second) return;

    auto rit = s_loopRefCount.find(key);
    if (rit != s_loopRefCount.end()) {
        rit->second--;
        if (rit->second > 0) return; // still referenced elsewhere
    }

    // stop and delete
    lit->second->Stop();
    delete lit->second;
    s_loopChannels.erase(lit);
    s_loopRefCount.erase(key);
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
        // Start channel at the current global SFX master volume so loops respect user SFX volume
        channel.source->volume = s_sfxSource ? s_sfxSource->volume : 1.0f;
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

    // Apply master SFX volume so distance loops scale with overall SFX settings
    f32 masterSfx = s_sfxSource ? s_sfxSource->volume : 1.0f;
    channel.source->SetVolume(maxVolume * t * masterSfx);
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
    // Use configured maxVolume so loudness follows audio config instead of a hardcoded value
    UpdateDistanceLoop("annoying_classmate", nearestDistance, s_terminalLoopConfig.nearDistance, s_terminalLoopConfig.farDistance, s_terminalLoopConfig.maxVolume);
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

    // Helper: build additional fallback candidate paths by trying
    // common subfolders, alternative filename variants (spaces -> '_', '-')
    // and common audio extensions.
    auto buildCandidates = [&](const fs::path& fp, const std::vector<std::string>& subfolders) {
        std::vector<std::string> out;

        std::string filename = fp.filename().string();
        std::string stem = fp.stem().string();
        std::string ext = fp.extension().string();

        // common extensions to try (original extension first if present)
        std::vector<std::string> exts;
        if (!ext.empty()) exts.push_back(ext);
        const std::vector<std::string> extraExts = { ".wav", ".ogg", ".mp3", ".flac" };
        for (const auto& e : extraExts) {
            if (exts.end() == std::find(exts.begin(), exts.end(), e)) exts.push_back(e);
        }

        // name variants
        std::vector<std::string> nameVars;
        nameVars.push_back(filename);
        // replace spaces
        auto replaceChar = [&](const std::string& s, char a, char b){ std::string r = s; std::replace(r.begin(), r.end(), a, b); return r; };
        nameVars.push_back(replaceChar(filename, ' ', '_'));
        nameVars.push_back(replaceChar(filename, ' ', '-'));

        // lowercase variants
        auto toLower = [&](const std::string& s){ std::string r = s; std::transform(r.begin(), r.end(), r.begin(), [](unsigned char c){ return std::tolower(c); }); return r; };
        nameVars.push_back(toLower(filename));
        nameVars.push_back(toLower(replaceChar(filename, ' ', '_')));
        nameVars.push_back(toLower(replaceChar(filename, ' ', '-')));

        // ensure unique
        std::sort(nameVars.begin(), nameVars.end());
        nameVars.erase(std::unique(nameVars.begin(), nameVars.end()), nameVars.end());

        // prefer project Audio BGM and generic Audio folders
        for (const auto& nv : nameVars) {
            for (const auto& e : exts) {
                out.push_back(std::string("Assets/Audio/BGM/") + nv);
                out.push_back(std::string("Assets/Audio/") + nv);
            }
        }

        // try categorized subfolders
        for (const auto& sub : subfolders) {
            for (const auto& nv : nameVars) {
                out.push_back(std::string("Assets/Audio/") + sub + "/" + nv);
            }
        }

        // legacy locations
        for (const auto& nv : nameVars) {
            out.push_back(std::string("Assets/sound/") + nv);
            out.push_back(std::string("Assets/Audio/SFX/") + nv);
        }

        // Finally try the raw filename (without any directory) with extensions
        for (const auto& nv : nameVars) {
            out.push_back(nv);
        }

        return out;
    };

    // Try original path first
    if (auto* clip = tryLoad(path)) return clip;

    // If not found, try loose filename resolution (scan Assets/Audio)
    std::string loose = FindLooseAudioFile(path.empty() ? std::string() : std::filesystem::path(path).filename().string());
    if (!loose.empty()) {
        std::printf("[Info] SoundManager: Resolved loose SFX path '%s' -> '%s'\n", path.c_str(), loose.c_str());
        if (auto* clip = tryLoad(loose)) return clip;
    }

    fs::path fp(path);
    const std::vector<std::string> audioSubfolders = {
        "BGM", "Guard", "AnnoyingClassMate", "Bully", "CCTV", "Door", "Player",
        "AccessCardPickup", "powerup", "Puzzle", "UI"
    };

    auto candidates = buildCandidates(fp, audioSubfolders);
    for (const auto& cpath : candidates) {
        bool exists = false;
        try { exists = fs::exists(fs::path(cpath)); } catch (...) { exists = false; }
        std::printf("[Debug] SoundManager: Trying music candidate: %s (exists=%d)\n", cpath.c_str(), exists ? 1 : 0);
        if (auto* clip = tryLoad(cpath)) {
            std::printf("[Info] SoundManager: Loaded music via fallback path: %s\n", cpath.c_str());
            return clip;
        }
    }

    // Try loose filename resolution for music as well
    std::string looseMusic = FindLooseAudioFile(fp.filename().string());
    if (!looseMusic.empty()) {
        std::printf("[Info] SoundManager: Resolved loose music path '%s' -> '%s'\n", path.c_str(), looseMusic.c_str());
        if (auto* clip = tryLoad(looseMusic)) return clip;
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

    fs::path fp(path);
    const std::vector<std::string> audioSubfolders = {
        "Guard", "AnnoyingClassMate", "Bully", "CCTV", "Door", "Player",
        "AccessCardPickup", "powerup", "Puzzle", "UI", "BGM"
    };

    // reuse same candidate builder used for music
    auto buildCandidatesLocal = [&](const fs::path& p) {
        // reuse logic from LoadMusicClip buildCandidates
        std::vector<std::string> out;

        std::string filename = p.filename().string();
        std::string ext = p.extension().string();

        std::vector<std::string> exts;
        if (!ext.empty()) exts.push_back(ext);
        const std::vector<std::string> extraExts = { ".wav", ".ogg", ".mp3", ".flac" };
        for (const auto& e : extraExts) {
            if (exts.end() == std::find(exts.begin(), exts.end(), e)) exts.push_back(e);
        }

        auto replaceChar = [&](const std::string& s, char a, char b){ std::string r = s; std::replace(r.begin(), r.end(), a, b); return r; };
        auto toLower = [&](const std::string& s){ std::string r = s; std::transform(r.begin(), r.end(), r.begin(), [](unsigned char c){ return std::tolower(c); }); return r; };

        std::vector<std::string> nameVars;
        nameVars.push_back(filename);
        nameVars.push_back(replaceChar(filename, ' ', '_'));
        nameVars.push_back(replaceChar(filename, ' ', '-'));
        nameVars.push_back(toLower(filename));
        nameVars.push_back(toLower(replaceChar(filename, ' ', '_')));
        nameVars.push_back(toLower(replaceChar(filename, ' ', '-')));

        std::sort(nameVars.begin(), nameVars.end());
        nameVars.erase(std::unique(nameVars.begin(), nameVars.end()), nameVars.end());

        for (const auto& sub : audioSubfolders) {
            for (const auto& nv : nameVars) {
                out.push_back(std::string("Assets/Audio/") + sub + "/" + nv);
            }
        }

        for (const auto& nv : nameVars) {
            out.push_back(std::string("Assets/Audio/") + nv);
            out.push_back(std::string("Assets/sound/") + nv);
            out.push_back(std::string("Assets/Audio/SFX/") + nv);
            out.push_back(nv);
        }

        return out;
    };

    auto candidates = buildCandidatesLocal(fp);
    for (const auto& cpath : candidates) {
        bool exists = false;
        try { exists = fs::exists(fs::path(cpath)); } catch (...) { exists = false; }
        std::printf("[Debug] SoundManager: Trying SFX candidate: %s (exists=%d)\n", cpath.c_str(), exists ? 1 : 0);
        if (auto* clip = tryLoad(cpath)) {
            std::printf("[Info] SoundManager: Loaded SFX via fallback path: %s\n", cpath.c_str());
            return clip;
        }
    }

    std::printf("[Warning] SoundManager: LoadSound failed for '%s' and fallback candidates\n", path.c_str());
    return nullptr;
}

} // namespace Engine

// Auto-cleanup to ensure resources are freed if the application doesn't
// explicitly call SoundManager::Shutdown(). This helps prevent static
// allocations from being reported as leaks by the CRT at process exit.
namespace Engine {
struct SoundManagerAutoShutdown {
    ~SoundManagerAutoShutdown() { SoundManager::Shutdown(); }
};

static SoundManagerAutoShutdown s_soundManagerAutoShutdown;
} // namespace Engine
