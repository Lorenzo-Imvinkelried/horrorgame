#pragma once
#include <SFML/Graphics.hpp>
#include <optional>
#include <vector>
#include <memory> // Required for std::shared_ptr
#include "entities/player/Player.h" 
#include "../engine/ResourceManager.h"
#include "../items/Item.h"
#include "tooltip/Tooltip.h" // Nuevo
#include "MapPanel.h" // [NEW]

#include "UIPanel.h" // New include for inheritance

class InventoryPanel : public UIPanel { // Inherit from UIPanel
public:
    InventoryPanel(sf::Texture* fontTexture, std::optional<sf::Sprite>& slotSprite);
    void setOnCloseCallback(std::function<void()> callback);
    void draw(sf::RenderTarget& target, ResourceManager& res) override;


    // Métodos para Drag & Drop
    int getSlotAt(sf::Vector2f mousePos) const;
    std::shared_ptr<Item> getItem(int slotIndex);
    void setItem(int slotIndex, std::shared_ptr<Item> item);
    void setVisible(bool visible) override; // [FIX] Reset drag state on hide
    int findEmptySlot() const; // [NEW]

    // --- ¡ESTAS SON LAS FUNCIONES QUE FALTABAN! ---
    void setPosition(sf::Vector2f pos) override;
    sf::FloatRect getBounds() const override;
    bool onMousePress(sf::Vector2f mousePos) override;
    void onMouseRelease() override;
    void onMouseMove(sf::Vector2f mousePos) override;
    bool isBeingDragged() const override { return mIsBeingDragged; }
    // ---

    void setPlayer(const Player* player) { mPlayerPtr = player; }

private:
    void drawSlotFallback(sf::RenderTarget& target, sf::Vector2f pos);
    void drawCoinText(sf::RenderTarget& target, float zoom, const std::string& str, float basePointX, float baseY);

private:
    const Player* mPlayerPtr = nullptr;
    sf::Texture* mFontTexture = nullptr;
    std::optional<sf::Sprite>& mSlotSprite;

    // Almacenamiento de items (el tamaño se define en el cpp)
    std::vector<std::shared_ptr<Item>> mItems;

    // --- Variables de Estado ---
    sf::Vector2f mPosition;
    sf::Vector2f mSize; // Ahora será el tamaño dictado por el sprite (x2)
    bool         mIsBeingDragged = false;
    sf::Vector2f mDragOffset;

    // --- Estandarización UI ---
    std::optional<sf::Sprite> mBackgroundSprite;
    bool mHasValidBackground = false; 
    std::function<void()> mOnCloseCallback;
    // ---

    // Constantes
    static constexpr int INV_COLS = 8;
    static constexpr int INV_ROWS = 5;
    static constexpr float UNIFIED_SLOT_SIZE = 60.f;
    static constexpr float UNIFIED_SLOT_MARGIN = 6.f;
    static constexpr float UI_MARGIN = 10.f;
};