#pragma once
#include <SFML/System/Vector2.hpp>
#include <vector>
#include <string>
#include <unordered_map>
#include <cmath>
#include <algorithm>
#include <optional>

// Tipos de datos para nuestras pistas
template <typename T>
struct Keyframe {
    float time;
    T value;
};

template <typename T>
struct Track {
    std::vector<Keyframe<T>> frames;

    T evaluate(float currentTime) const {
        if (frames.empty()) return T{};
        if (frames.size() == 1 || currentTime <= frames.front().time) return frames.front().value;
        if (frames.back().time <= currentTime) return frames.back().value;

        for (size_t i = 0; i < frames.size() - 1; ++i) {
            if (currentTime >= frames[i].time && currentTime < frames[i+1].time) {
                float t = (currentTime - frames[i].time) / (frames[i+1].time - frames[i].time);
                return lerp(frames[i].value, frames[i+1].value, t);
            }
        }
        return frames.back().value;
    }

    void insertOrUpdateKeyframe(float time, const T& value, float epsilon = 0.015f) {
        for (auto& kf : frames) {
            if (std::abs(kf.time - time) <= epsilon) {
                kf.value = value;
                return;
            }
        }
        frames.push_back({ time, value });
        std::sort(frames.begin(), frames.end(), [](const Keyframe<T>& a, const Keyframe<T>& b) {
            return a.time < b.time;
        });
    }

    bool removeKeyframeNear(float time, float epsilon = 0.02f) {
        for (auto it = frames.begin(); it != frames.end(); ++it) {
            if (std::abs(it->time - time) <= epsilon) {
                frames.erase(it);
                return true;
            }
        }
        return false;
    }

private:
    float lerp(float a, float b, float t) const { return a + (b - a) * t; }
    sf::Vector2f lerp(sf::Vector2f a, sf::Vector2f b, float t) const {
        return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
    }
};

struct AnimEvent {
    float time;
    std::string name;
};

struct AnimationClip {
    std::string name;
    float duration = 1.0f;
    bool isLoop = false;
    float loopStart = 0.0f;
    float loopEnd = -1.0f; // -1 means until duration
    
    std::vector<AnimEvent> events;

    // Un mapa de strings. La key es el nombre de la parte ("head", "body", "hand_l", etc)
    std::unordered_map<std::string, Track<sf::Vector2f>> positionTracks;
    std::unordered_map<std::string, Track<float>> rotationTracks;
    std::unordered_map<std::string, Track<sf::Vector2f>> scaleTracks;

    // Custom Layer Order (Z-Index / Render Priority) from Back (0) to Front (N-1)
    std::vector<std::string> layerOrder;

    float getEffectiveLoopStart() const {
        if (duration <= 0.001f) return 0.0f;
        return std::clamp(loopStart, 0.0f, duration);
    }

    float getEffectiveLoopEnd() const {
        if (duration <= 0.001f) return 0.0f;
        float start = getEffectiveLoopStart();
        if (loopEnd > start && loopEnd <= duration) return loopEnd;
        return duration;
    }

    // Load and save JSON
    bool loadFromFile(const std::string& path);
    bool saveToFile(const std::string& path) const;
};

struct SkeletonData {
    std::optional<sf::Vector2f> headOffset;
    std::optional<sf::Vector2f> handLOffset;
    std::optional<sf::Vector2f> handROffset;
    std::optional<sf::Vector2f> footLOffset;
    std::optional<sf::Vector2f> footROffset;
    std::optional<sf::Vector2f> weaponOffset;
    std::optional<sf::Vector2f> weaponSecondaryOffset;
    std::optional<sf::Vector2f> weaponTwoHandedOffset; // [NEW]
    std::optional<float> groundOffsetY;
    std::optional<float> stride;

    std::vector<std::string> parts;
    std::unordered_map<std::string, sf::Vector2f> offsets;

    bool loadFromFile(const std::string& path);
};

