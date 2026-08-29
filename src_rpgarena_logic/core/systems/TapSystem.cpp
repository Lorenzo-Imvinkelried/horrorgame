#include "TapSystem.h"
#include "utils/TinyJson.h"
#include <iostream>

TapSystem::TapSystem() 
    : mCharges(0)
    , mMaxCharges(5)
    , mChargeMultiplierPerStack(0.15f)
    , mHitCounter(0)
    , mRequiredHits(4)
    , mThresholdDamageMultiplier(1.5f)
    , mUiOffsetX(38.0f)
    , mUiOffsetY(31.0f)
{
    // Default rects in case config load fails
    mTopRowRects = {
        {1, 0, 9, 5},
        {13, 0, 9, 5},
        {25, 0, 9, 5},
        {37, 0, 9, 5},
        {49, 0, 9, 5}
    };
    mBottomRowRects = {
        {1, 6, 17, 2},
        {21, 6, 17, 2},
        {41, 6, 17, 2}
    };

    loadConfig("assets/data/taps.json");
}

bool TapSystem::loadConfig(const std::string& path) {
    try {
        json::Value root = json::parseFile(path);
        if (root.type != json::Type::Object) {
            std::cerr << "[TapSystem] WARNING: " << path << " is not an object.\n";
            return false;
        }

        const auto& obj = root.asObject();

        if (obj.count("max_charges")) mMaxCharges = obj.at("max_charges").asInt();
        if (obj.count("required_hits")) mRequiredHits = obj.at("required_hits").asInt();
        if (obj.count("charge_damage_multiplier_per_stack")) mChargeMultiplierPerStack = static_cast<float>(obj.at("charge_damage_multiplier_per_stack").asDouble());
        if (obj.count("threshold_damage_multiplier")) mThresholdDamageMultiplier = static_cast<float>(obj.at("threshold_damage_multiplier").asDouble());
        if (obj.count("ui_offset_x")) mUiOffsetX = static_cast<float>(obj.at("ui_offset_x").asDouble());
        if (obj.count("ui_offset_y")) mUiOffsetY = static_cast<float>(obj.at("ui_offset_y").asDouble());

        if (obj.count("top_row_rects") && obj.at("top_row_rects").type == json::Type::Array) {
            mTopRowRects.clear();
            for (const auto& item : obj.at("top_row_rects").asArray()) {
                if (item.type == json::Type::Array && item.asArray().size() >= 4) {
                    const auto& arr = item.asArray();
                    TapRect tr;
                    tr.left = arr[0].asInt();
                    tr.top = arr[1].asInt();
                    tr.width = arr[2].asInt();
                    tr.height = arr[3].asInt();
                    mTopRowRects.push_back(tr);
                }
            }
        }

        if (obj.count("bottom_row_rects") && obj.at("bottom_row_rects").type == json::Type::Array) {
            mBottomRowRects.clear();
            for (const auto& item : obj.at("bottom_row_rects").asArray()) {
                if (item.type == json::Type::Array && item.asArray().size() >= 4) {
                    const auto& arr = item.asArray();
                    TapRect tr;
                    tr.left = arr[0].asInt();
                    tr.top = arr[1].asInt();
                    tr.width = arr[2].asInt();
                    tr.height = arr[3].asInt();
                    mBottomRowRects.push_back(tr);
                }
            }
        }

        std::cout << "[TapSystem] Configuration loaded from " << path << " successfully.\n";
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[TapSystem] ERROR loading " << path << ": " << e.what() << "\n";
        return false;
    }
}

void TapSystem::addCharges(int count) {
    mCharges = std::clamp(mCharges + count, 0, mMaxCharges);
}

int TapSystem::consumeCharges() {
    int consumed = mCharges;
    mCharges = 0;
    return consumed;
}

void TapSystem::resetCharges() {
    mCharges = 0;
}

float TapSystem::getChargeDamageMultiplier() const {
    return 1.0f + (mCharges * mChargeMultiplierPerStack);
}

bool TapSystem::onBasicAttackHit() {
    mHitCounter++;
    if (mHitCounter >= mRequiredHits) {
        mHitCounter = 0;
        return true; // Threshold hit triggered (e.g. 4th attack)
    }
    return false;
}

void TapSystem::resetHitCounter() {
    mHitCounter = 0;
}

void TapSystem::resetAll() {
    mCharges = 0;
    mHitCounter = 0;
}
