#pragma once
#include "UIPanel.h"
#include "../items/Item.h"
#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <optional>

class CultivoPanel : public UIPanel {
public:
    CultivoPanel(sf::Texture* fontTexture, std::optional<sf::Sprite>& slotSprite);

    void draw(sf::RenderTarget& target, ResourceManager& res) override;
    sf::FloatRect getBounds() const override;
    void setPosition(sf::Vector2f pos) override;

    bool onMousePress(sf::Vector2f mousePos) override;
    void onMouseMove(sf::Vector2f mousePos) override;
    void onMouseRelease() override;

    void setItem(std::shared_ptr<Item> item);
    std::shared_ptr<Item> getItem() const;

    bool isMouseOverSlot(sf::Vector2f mousePos) const;

    void toggle() { setVisible(!mVisible); }
    void setVisible(bool visible) override;

    void setOnCloseCallback(std::function<void()> cb) { mOnCloseCallback = cb; }
    void setOnConfirmCallback(std::function<void()> cb) { mOnConfirmCallback = cb; }

    void loadLayout();

private:
    void drawSlotFallback(sf::RenderTarget& target, sf::Vector2f pos);
    void refreshAvailableStats();

    sf::Vector2f mPosition;
    sf::Vector2f mSize = {240.f, 260.f};

    // Layout offsets and texts from assets/data/cultivo-panel.json
    sf::Vector2f mTitleOffset = {15.f, 10.f};
    std::string mTitleText = "Cultivo de Equipo";

    sf::Vector2f mItemSlotOffset = {24.f, 25.f};

    sf::Vector2f mItemNameOffset = {48.f, 25.f};
    sf::Vector2f mItemLevelOffset = {48.f, 39.f};
    std::string mItemLevelPrefix = "Cultivo Nivel: ";
    sf::Vector2f mPlaceholderOffset = {48.f, 30.f};
    std::string mPlaceholderText = "Arrastra un item aqui";

    sf::Vector2f mExpBarOffset = {24.f, 65.f};
    sf::Vector2f mExpTextOffset = {24.f, 77.f};
    std::string mExpTextPrefix = "EXP: ";

    sf::Vector2f mStatsHeaderOffset = {24.f, 82.f};
    std::string mStatsHeaderUnlocked = "Seleccionar Stats:";
    std::string mStatsHeaderLocked = "Stats Cultivadas:";
    sf::Vector2f mSocketsOffset = {24.f, 96.f};
    sf::Vector2f mSocketLabelsOffset = {40.f, 95.f};
    float mStatLineSpacing = 14.f;

    sf::Vector2f mButtonCultivarOffset = {59.f, 171.f};
    sf::Vector2f mButtonTextOffset = {74.f, 175.f};
    std::string mButtonTextActive = "Cultivar";
    std::string mButtonTextLocked = "[Cultivo Activo]";

    sf::Texture* mFontTexture = nullptr;
    std::optional<sf::Sprite>& mSlotSprite;

    std::optional<sf::Sprite> mBackgroundSprite;

    // Draggable window state
    bool mIsBeingDragged = false;
    sf::Vector2f mDragOffset;

    // UI callbacks
    std::function<void()> mOnCloseCallback;
    std::function<void()> mOnConfirmCallback;

    // Temp stat selection while unlocked
    std::vector<std::pair<std::string, std::string>> mAvailableStats;
    std::vector<bool> mSelectedFlags;
};
