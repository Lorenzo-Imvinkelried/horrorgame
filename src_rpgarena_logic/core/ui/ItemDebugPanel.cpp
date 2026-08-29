#include "ItemDebugPanel.h"
#include "InventoryPanel.h"
#include "../items/ItemManager.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <set>

ItemDebugPanel::ItemDebugPanel(sf::Texture* fontTexture, InventoryPanel* invPanel)
    : mFontTexture(fontTexture), mInventoryPanel(invPanel) {
}

void ItemDebugPanel::load(ResourceManager& res) {
    generateItems(res);
}

void ItemDebugPanel::setItemManager(ItemManager* itemMgr) {
    mItemMgr = itemMgr;
}

void ItemDebugPanel::generateItems(ResourceManager& res) {
    mItems.clear();
    
    if (mItemMgr) {
        std::set<std::string> loadedPaths;
        for (const auto& pair : mItemMgr->getAtlases()) {
            const auto& atlasCfg = pair.second;
            if (loadedPaths.count(atlasCfg.path)) continue;
            loadedPaths.insert(atlasCfg.path);

            try {
                sf::Texture& tex = res.getTexture(atlasCfg.path);
                int tw = tex.getSize().x;
                int th = tex.getSize().y;
                
                int cols = std::max(1, tw / atlasCfg.cellSize);
                int rows = std::max(1, th / atlasCfg.cellSize);
                
                for (int r = 0; r < rows; ++r) {
                    for (int c = 0; c < cols; ++c) {
                        auto item = std::make_shared<Item>();
                        item->id = "debug_" + std::to_string(mItems.size());
                        item->name = "Debug " + atlasCfg.type;
                        
                        if (atlasCfg.type == "Weapon") item->type = ItemType::Weapon;
                        else if (atlasCfg.type == "Armor") item->type = ItemType::Armor;
                        else if (atlasCfg.type == "Potion") item->type = ItemType::Potion;
                        else if (atlasCfg.type == "Ring") item->type = ItemType::Ring;
                        else item->type = ItemType::Misc;
                        
                        item->texturePath = atlasCfg.path;
                        item->textureRect = sf::IntRect({c * atlasCfg.cellSize, r * atlasCfg.cellSize}, {atlasCfg.cellSize, atlasCfg.cellSize});
                        item->scale = atlasCfg.defaultScale;
                        item->quality = ItemQuality::Legendary; // Ponerle algo lindo por default
                        
                        mItems.push_back(item);
                    }
                }
                std::cout << "[ItemDebugPanel] Loaded atlas " << atlasCfg.path << " (" << (cols*rows) << " items)\n";
            } catch(...) {
                std::cerr << "[ItemDebugPanel] Error loading atlas texture: " << atlasCfg.path << "\n";
            }
        }
    }

    // Cargar ítems únicos fijos reales desde la base de datos modular
    if (mItemMgr) {
        for (const auto& uid : mItemMgr->getFixedUniqueIds()) {
            auto uniqueItem = mItemMgr->createItem(uid);
            if (uniqueItem) {
                mItems.push_back(uniqueItem);
            }
        }
    }
    
    // Setup layout math
    mCols = static_cast<int>((mSize.x - MARGIN * 2.f) / (ICON_SIZE + PADDING));
    if (mCols < 1) mCols = 1;
    
    int totalRows = std::ceil((float)mItems.size() / mCols);
    float totalHeight = MARGIN * 2.f + totalRows * (ICON_SIZE + PADDING);
    
    mMaxScrollY = std::max(0.f, totalHeight - (mSize.y - 30.f));
}

void ItemDebugPanel::draw(sf::RenderTarget& target, ResourceManager& res) {
    if (!isVisible()) return;
    
    // Draw background
    sf::RectangleShape bg(mSize);
    bg.setPosition(mPosition);
    bg.setFillColor(sf::Color(20, 20, 20, 230));
    bg.setOutlineThickness(2.f);
    bg.setOutlineColor(sf::Color(100, 100, 100));
    target.draw(bg);
    
    // Draw Title Bar Background
    sf::RectangleShape titleBar({mSize.x, 30.f});
    titleBar.setPosition(mPosition);
    titleBar.setFillColor(sf::Color(50, 50, 50, 255));
    target.draw(titleBar);
    
    float startY = mPosition.y + 30.f + MARGIN;
    float clipMinY = mPosition.y + 30.f;
    float clipMaxY = mPosition.y + mSize.y;
    
    for (size_t i = 0; i < mItems.size(); ++i) {
        int r = i / mCols;
        int c = i % mCols;
        
        float ix = mPosition.x + MARGIN + c * (ICON_SIZE + PADDING);
        float iy = startY + r * (ICON_SIZE + PADDING) - mScrollY;
        
        if (iy + ICON_SIZE < clipMinY || iy > clipMaxY) continue; // Basic clipping
        
        // Draw item bg box
        sf::RectangleShape box({ICON_SIZE, ICON_SIZE});
        box.setPosition({ix, iy});
        box.setFillColor(sf::Color(40,40,40));
        target.draw(box);
        
        auto& item = mItems[i];
        try {
            sf::Texture& tex = res.getTexture(item->texturePath);
            sf::Sprite spr(tex);
            spr.setTextureRect(item->textureRect);
            
            // scale to fit ICON_SIZE
            float scaleX = ICON_SIZE / item->textureRect.size.x;
            float scaleY = ICON_SIZE / item->textureRect.size.y;
            spr.setScale({scaleX, scaleY});
            spr.setPosition({ix, iy});
            target.draw(spr);
            
        } catch(...) {}
    }
}

void ItemDebugPanel::setPosition(sf::Vector2f pos) { mPosition = pos; }

sf::FloatRect ItemDebugPanel::getBounds() const {
    return sf::FloatRect(mPosition, mSize);
}

void ItemDebugPanel::setVisible(bool visible) {
    UIPanel::setVisible(visible);
    if (!visible) mIsBeingDragged = false;
}

bool ItemDebugPanel::onMousePress(sf::Vector2f mousePos) {
    if (!isVisible() || !getBounds().contains(mousePos)) return false;
    
    // Title bar drag
    if (mousePos.y < mPosition.y + 30.f) {
        mIsBeingDragged = true;
        mDragOffset = mousePos - mPosition;
        return true;
    }
    
    // Check if clicked an item for right click auto-loot
    if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Right)) {
        auto item = getItemAt(mousePos);
        if (item) {
             addItemToInventory(item);
             return true;
        }
    }
    
    // Si hacemos click izquierdo sobre un item devolvemos true para que HUD 
    // lo reciba y pueda iniciar el Drag and Drop.
    if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
        if (getItemAt(mousePos) != nullptr) return true;
    }

    return true; // Clicked on panel background
}

std::shared_ptr<Item> ItemDebugPanel::getItemAt(sf::Vector2f mousePos) const {
    if (!isVisible() || !getBounds().contains(mousePos)) return nullptr;
    
    float startY = mPosition.y + 30.f + MARGIN;
    for (size_t i = 0; i < mItems.size(); ++i) {
        int r = i / mCols;
        int c = i % mCols;
        float ix = mPosition.x + MARGIN + c * (ICON_SIZE + PADDING);
        float iy = startY + r * (ICON_SIZE + PADDING) - mScrollY;
        
        sf::FloatRect iconRect({ix, iy}, {ICON_SIZE, ICON_SIZE});
        if (iconRect.contains(mousePos)) {
            return mItems[i];
        }
    }
    return nullptr;
}

void ItemDebugPanel::onMouseRelease() {
    mIsBeingDragged = false;
}

void ItemDebugPanel::onMouseMove(sf::Vector2f mousePos) {
    if (mIsBeingDragged) {
        mPosition.x = mousePos.x - mDragOffset.x;
        mPosition.y = mousePos.y - mDragOffset.y;
    }
}

void ItemDebugPanel::onMouseWheel(int delta) {
    if (!isVisible()) return;
    float scrollAmount = 40.f; 
    mScrollY -= delta * scrollAmount;
    
    mScrollY = std::max(0.f, std::min(mScrollY, mMaxScrollY));
}

void ItemDebugPanel::addItemToInventory(std::shared_ptr<Item> item) {
    if (!mInventoryPanel) return;
    int slot = mInventoryPanel->findEmptySlot();
    if (slot != -1) {
        auto newItem = std::make_shared<Item>(*item);
        mInventoryPanel->setItem(slot, newItem);
    } else {
         std::cerr << "[ItemDebugPanel] Inventario lleno, no se pudo añadir.\n";
    }
}
