#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include <string>
#include "../items/Item.h"

enum class InteractionMode {
    None,
    SocketStone,
    Salvage,
    Identify,
    PlaceItem
};

class InteractionSystem {
public:
    static InteractionSystem& getInstance();

    void start(InteractionMode mode, std::shared_ptr<Item> contextItem, int sourceSlot = -1);
    void cancel();

    bool isActive() const { return mMode != InteractionMode::None; }
    InteractionMode getMode() const { return mMode; }
    std::shared_ptr<Item> getContextItem() const { return mContextItem; }
    int getSourceSlot() const { return mSourceSlot; }

    bool isCompatible(std::shared_ptr<Item> target) const;
    
    // Intercept slot click. Returns true if consumed.
    bool onSlotClicked(int slotIndex, std::shared_ptr<Item> targetItem, class InventoryPanel* invPanel, class Player* player);
    
    // Intercept world click. Returns true if consumed.
    bool onWorldClicked(sf::Vector2f worldPos, class Player* player);

    void drawCursor(sf::RenderTarget& target, sf::Vector2f mousePos, class ResourceManager& res, float zoom);

private:
    InteractionSystem() = default;
    ~InteractionSystem() = default;
    
    InteractionMode mMode = InteractionMode::None;
    std::shared_ptr<Item> mContextItem = nullptr;
    int mSourceSlot = -1;
};
