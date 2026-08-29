//TargetFrame.h
#pragma once
#include <SFML/Graphics.hpp>
#include <optional>
#include "../engine/ResourceManager.h"
#include "entities/Entity.h" // base class
#include "Config.h"

class TargetFrame {
public:
    explicit TargetFrame(sf::Texture* fontTexture);
    void load(ResourceManager& res);
    
    // [OPTIMIZATION] Event-based update
    void setTarget(Entity* target);
    void updateRT();
    void draw(sf::RenderTarget& target); // No arg draw
    void notifyEntityDeath(Entity* entity); // [SAFETY]
    
    sf::FloatRect getBounds() const { return mLastBounds; }

private:
    void updateTexts(); // Rebuild cached strings

private:
    sf::FloatRect mLastBounds;
    
    // Cached State
    Entity* mTarget = nullptr;
    int mObserverId = -1;
    
    // Cached Data for Drawing
    std::string mNameStr;
    std::string mWeightStr;
    
    int mCachedCurHp = 0;
    int mCachedMaxHp = 1;
    float mCachedHpPct = 0.f;
    std::string mHpStr;

    int mCachedCurMp = 0;
    int mCachedMaxMp = 1;
    float mCachedMpPct = 0.f;
    std::string mMpStr;

private:
    // helpers de texto
    void drawBarWithText(sf::RenderTarget& target,
                         sf::Vector2f pos, sf::Vector2f size,
                         float fill01,
                         sf::Color bg, sf::Color fg,
                         const sf::String& labelLeft,   // ej: "HP"
                         const sf::String& valueCenter, // ej: "450/600 (75%)"
                         sf::Texture* fillTex = nullptr,// [NEW]
                         sf::Texture* bgTex = nullptr   // [NEW]
                         );

private:
    sf::Texture* mFontTexture = nullptr;
    sf::Texture* mHpGreenTexture = nullptr; // [NEW]
    sf::Texture* mHpRedTexture = nullptr;   // [NEW]
    sf::Texture* mMpBlueTexture = nullptr;  // [NEW]
    sf::Texture* mFrameBgTexture = nullptr; // [NEW] Full Frame UI Background
    sf::Texture* mBossFrameBgTexture = nullptr; // [NEW] Boss Frame UI Background
    std::optional<sf::Sprite> mPortraitBg; // fondo (opcional)

    // [LIVE PORTRAIT] RenderTexture for real-time target portrait
    sf::RenderTexture mPortraitRT;
    bool mPortraitReady = false;

    // Constantes de layout
    // static constexpr float PORTRAIT_SIZE = cfg::UI::TargetFrame::PORTRAIT_SIZE; // Alias if valid C++20
    // static constexpr float UI_MARGIN     = cfg::UI::TargetFrame::MARGIN;
    // Or just rely on cfg:: usage in .cpp and remove these private constants
    // Let's remove them to force usage of Config
};
