#pragma once
#include <memory>
#include <vector>
#include <string>
#include <utility>
#include <SFML/Graphics.hpp>
#include "../items/Item.h"

class ResourceManager;

class CultivoSystem {
public:
    static CultivoSystem& getInstance();

    // Set / Get currently cultivated item
    void setCultivatedItem(std::shared_ptr<Item> item);
    std::shared_ptr<Item> getCultivatedItem() const { return mCultivatedItem; }
    bool hasCultivatedItem() const { return mCultivatedItem != nullptr; }
    void reset() { mCultivatedItem = nullptr; mAnimTimer = 0.f; mAnimationFrame = 0; }

    // Adds EXP earned from kills. Returns true if item leveled up in cultivation.
    bool addExp(int amount);

    // EXP required for cultivation level up
    static int getExpForNextLevel(int level);

    // Re-calculates item.cultivoBonusStats based on level and selected stats
    static void recalculateCultivoBonusStats(Item& item);

    // Returns available non-zero stat IDs and display names for an item
    static std::vector<std::pair<std::string, std::string>> getAvailableStats(const Item& item);

    // Animation & Indicator System (8 FPS, 4 frames loop)
    void update(float dt);
    int getAnimationFrame() const { return mAnimationFrame; }
    sf::IntRect getAnimationFrameRect() const;
    void drawIndicator(sf::RenderTarget& target, ResourceManager& res, sf::Vector2f slotPos, float zoom);

private:
    CultivoSystem() = default;
    std::shared_ptr<Item> mCultivatedItem = nullptr;

    float mAnimTimer = 0.0f;
    int mAnimationFrame = 0;
};
