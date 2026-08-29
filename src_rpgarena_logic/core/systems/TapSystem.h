#pragma once
#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Time.hpp>
#include <algorithm>
#include <string>
#include <vector>

struct TapRect {
    int left = 0;
    int top = 0;
    int width = 0;
    int height = 0;
};

class TapSystem {
public:
    TapSystem();

    bool loadConfig(const std::string& path = "assets/data/taps.json");

    // --- ROW 1: CHARGES (CARGAS) ---
    int getCharges() const { return mCharges; }
    int getMaxCharges() const { return mMaxCharges; }
    void setMaxCharges(int maxC) { mMaxCharges = std::max(1, maxC); mCharges = std::clamp(mCharges, 0, mMaxCharges); }
    
    void addCharges(int count = 1);
    int consumeCharges(); // Returns current charges and resets mCharges to 0
    void resetCharges();
    float getChargeDamageMultiplier() const; // Extra damage multiplier per charge (e.g. +15% per charge)

    // --- ROW 2: HIT COUNTER / COMBO (CADENCIA DE ATAQUES) ---
    int getHitCounter() const { return mHitCounter; }
    int getRequiredHits() const { return mRequiredHits; }
    void setRequiredHits(int req) { mRequiredHits = std::max(1, req); if (mHitCounter >= mRequiredHits) mHitCounter = 0; }
    float getThresholdDamageMultiplier() const { return mThresholdDamageMultiplier; }

    // Increments hit count on basic attack. Returns true if this hit is the bonus threshold hit (e.g. 4th attack)
    bool onBasicAttackHit();
    void resetHitCounter();

    // Reset both bars
    void resetAll();

    // --- UI LAYOUT GETTERS ---
    float getUiOffsetX() const { return mUiOffsetX; }
    float getUiOffsetY() const { return mUiOffsetY; }
    const std::vector<TapRect>& getTopRowRects() const { return mTopRowRects; }
    const std::vector<TapRect>& getBottomRowRects() const { return mBottomRowRects; }

private:
    // Row 1
    int mCharges = 0;
    int mMaxCharges = 5;
    float mChargeMultiplierPerStack = 0.15f;

    // Row 2
    int mHitCounter = 0;
    int mRequiredHits = 4; // Default: 4th attack grants bonus damage
    float mThresholdDamageMultiplier = 1.5f; // +50% bonus damage on 4th hit

    // Layout config
    float mUiOffsetX = 38.0f;
    float mUiOffsetY = 31.0f;
    std::vector<TapRect> mTopRowRects;
    std::vector<TapRect> mBottomRowRects;
};
