#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
#include "UIPanel.h"
#include "../../Config.h"
#include "../items/Item.h"
#include "../engine/ResourceManager.h"

class InventoryPanel;
class Player;

class ItemDebugPanel : public UIPanel {
public:
    ItemDebugPanel(sf::Texture* fontTexture, InventoryPanel* invPanel);
    
    void load(ResourceManager& res);
    void setItemManager(class ItemManager* itemMgr);
    void draw(sf::RenderTarget& target, ResourceManager& res) override;
    
    void setPosition(sf::Vector2f pos) override;
    sf::FloatRect getBounds() const override;
    std::shared_ptr<Item> getItemAt(sf::Vector2f pos) const; // [NEW]
    
    bool onMousePress(sf::Vector2f mousePos) override;
    void onMouseRelease() override;
    void onMouseMove(sf::Vector2f mousePos) override;
    void onMouseWheel(int delta);
    
    void setVisible(bool visible) override;

private:
    void generateItems(ResourceManager& res);
    void addItemToInventory(std::shared_ptr<Item> item);

    sf::Texture* mFontTexture;
    InventoryPanel* mInventoryPanel; 
    class ItemManager* mItemMgr = nullptr;
    
    sf::Vector2f mPosition{200.f, 100.f};
    sf::Vector2f mSize{600.f, 500.f};
    
    std::vector<std::shared_ptr<Item>> mItems;
    
    // Scrolling
    float mScrollY = 0.f;
    float mMaxScrollY = 0.f;
    
    // Dragging
    bool mIsBeingDragged = false;
    sf::Vector2f mDragOffset;
    
    // Layout
    static constexpr float ICON_SIZE = 32.f;
    static constexpr float MARGIN = 15.f;
    static constexpr float PADDING = 8.f;
    int mCols = 10;
};
