#include "InteractionSystem.h"
#include "../ui/InventoryPanel.h"
#include "entities/player/Player.h"
#include "../engine/ResourceManager.h"
#include "Config.h"
#include <iostream>
#include <cmath>

InteractionSystem& InteractionSystem::getInstance() {
    static InteractionSystem instance;
    return instance;
}

void InteractionSystem::start(InteractionMode mode, std::shared_ptr<Item> contextItem, int sourceSlot) {
    mMode = mode;
    mContextItem = contextItem;
    mSourceSlot = sourceSlot;
    std::cout << "[InteractionSystem] Started mode: " << static_cast<int>(mode) 
              << " with item: " << (contextItem ? contextItem->name : "None")
              << " from slot: " << sourceSlot << "\n";
}

void InteractionSystem::cancel() {
    if (mMode != InteractionMode::None) {
        std::cout << "[InteractionSystem] Interaction cancelled.\n";
    }
    mMode = InteractionMode::None;
    mContextItem = nullptr;
    mSourceSlot = -1;
}

bool InteractionSystem::isCompatible(std::shared_ptr<Item> target) const {
    if (!target) return false;
    
    switch (mMode) {
        case InteractionMode::SocketStone:
            // Sockets can only be applied to Weapons and Armors with sockets
            return (target->type == ItemType::Weapon || target->type == ItemType::Armor) && (target->maxSockets > 0);
        case InteractionMode::Salvage:
            return (target->type == ItemType::Weapon || target->type == ItemType::Armor);
        case InteractionMode::Identify:
            return (target->type == ItemType::Weapon || target->type == ItemType::Armor);
        default:
            return false;
    }
}

bool InteractionSystem::onSlotClicked(int slotIndex, std::shared_ptr<Item> targetItem, InventoryPanel* invPanel, Player* player) {
    if (!isActive() || !player) return false;
    
    if (mMode == InteractionMode::SocketStone) {
        if (!isCompatible(targetItem)) {
            std::cout << "[InteractionSystem] Target item is not compatible or has no sockets!\n";
            return true; // Click consumed anyway to prevent default action
        }
        
        // Find empty socket in target item
        int emptySocketIdx = -1;
        for (int i = 0; i < targetItem->maxSockets; ++i) {
            if (i >= (int)targetItem->socketedStones.size()) {
                targetItem->socketedStones.resize(targetItem->maxSockets, nullptr);
            }
            if (!targetItem->socketedStones[i]) {
                emptySocketIdx = i;
                break;
            }
        }
        
        if (emptySocketIdx == -1) {
            std::cout << "[InteractionSystem] Target item has no empty sockets!\n";
            return true; // Click consumed
        }
        
        // Insert stone!
        targetItem->socketedStones[emptySocketIdx] = mContextItem;
        std::cout << "[InteractionSystem] Successfully socketed " << mContextItem->name 
                  << " into " << targetItem->name << " at socket " << emptySocketIdx << "\n";
        
        // Consume stone from inventory
        if (invPanel && mSourceSlot != -1) {
            invPanel->setItem(mSourceSlot, nullptr);
        }
        
        // Recalculate stats since weapon/armor stats might have changed
        player->recalculateStats();
        
        // Complete interaction
        cancel();
        return true;
    }
    
    return false;
}

bool InteractionSystem::onWorldClicked(sf::Vector2f worldPos, Player* player) {
    if (!isActive()) return false;
    
    if (mMode == InteractionMode::PlaceItem) {
        std::cout << "[InteractionSystem] Placed item " << mContextItem->name << " at " << worldPos.x << ", " << worldPos.y << "\n";
        cancel();
        return true;
    }
    
    return false;
}

void InteractionSystem::drawCursor(sf::RenderTarget& target, sf::Vector2f mousePos, ResourceManager& res, float zoom) {
    if (!isActive() || !mContextItem) return;
    
    try {
        sf::Texture& tex = res.getTexture(mContextItem->texturePath);
        sf::Sprite sprite(tex);
        sprite.setTextureRect(mContextItem->textureRect);
        
        // NO extra scale, but keep the ZOOM_FACTOR scaling (2.0f)
        float scale = cfg::Map::ZOOM_FACTOR;
        sprite.setScale({scale, scale});
        
        // Position it to the left side of the cursor
        float itemWidth = sprite.getGlobalBounds().size.x;
        sprite.setPosition(mousePos + sf::Vector2f(-itemWidth - 5.f, 0.f));
        target.draw(sprite);
    } catch (...) {}
}
