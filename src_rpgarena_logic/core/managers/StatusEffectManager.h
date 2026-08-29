#pragma once
#include <string>
#include <map>
#include <vector>

struct StatusEffectInfo {
    std::string id;
    std::string name;
    std::string description;
    int atlasX = 0;
    int atlasY = 0;
    bool isDebuff = true;
};

class StatusEffectManager {
public:
    static StatusEffectManager& getInstance() {
        static StatusEffectManager instance;
        return instance;
    }

    bool loadConfig(const std::string& filepath);
    const StatusEffectInfo* getEffectInfo(const std::string& id) const;

    float getPlayerOffsetX() const { return mPlayerOffsetX; }
    float getPlayerOffsetY() const { return mPlayerOffsetY; }
    float getTargetOffsetX() const { return mTargetOffsetX; }
    float getTargetOffsetY() const { return mTargetOffsetY; }
    float getHoverOffsetX() const { return mHoverOffsetX; }
    float getHoverOffsetY() const { return mHoverOffsetY; }

private:
    StatusEffectManager() = default;
    ~StatusEffectManager() = default;
    StatusEffectManager(const StatusEffectManager&) = delete;
    StatusEffectManager& operator=(const StatusEffectManager&) = delete;

    std::map<std::string, StatusEffectInfo> mEffects;
    float mPlayerOffsetX = 0.f;
    float mPlayerOffsetY = 46.f;
    float mTargetOffsetX = 0.f;
    float mTargetOffsetY = 46.f;
    float mHoverOffsetX = 0.f;
    float mHoverOffsetY = 0.f;
};
