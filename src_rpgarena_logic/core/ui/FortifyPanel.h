#pragma once
#include "UIPanel.h"
#include "../items/Item.h"
#include <memory>
#include <functional>
#include <optional>

class FortifyPanel : public UIPanel {
public:
    FortifyPanel(sf::Texture* fontTexture, std::optional<sf::Sprite>& slotSprite);
    
    void draw(sf::RenderTarget& target, ResourceManager& res) override;
    sf::FloatRect getBounds() const override;
    void setPosition(sf::Vector2f pos) override;

    bool onMousePress(sf::Vector2f mousePos) override;
    void onMouseMove(sf::Vector2f mousePos) override;
    void onMouseRelease() override;

    void setItem(std::shared_ptr<Item> item);
    std::shared_ptr<Item> getItem() const { return mItem; }
    bool isMouseOverSlot(sf::Vector2f mousePos) const;

    void toggle() { setVisible(!mVisible); }
    void setVisible(bool visible) override;

    void setOnCloseCallback(std::function<void()> cb) { mOnCloseCallback = cb; }
    void setOnFortifyCallback(std::function<void(Item&)> cb) { mOnFortifyCallback = cb; }

    void showStatus(const std::string& msg, sf::Color color = sf::Color::White); // [NEW]

private:
    void drawSlotFallback(sf::RenderTarget& target, sf::Vector2f pos);

    sf::Vector2f mPosition;
    sf::Vector2f mSize = {128.f, 128.f}; // Size from JSON or fixed base
    
    std::shared_ptr<Item> mItem = nullptr;
    
    sf::Texture* mFontTexture = nullptr;
    std::optional<sf::Sprite>& mSlotSprite;
    
    std::optional<sf::Sprite> mBackgroundSprite;
    std::optional<sf::Sprite> mButtonSprite;
    std::optional<sf::Sprite> mButtonDownSprite; // [NEW]
    std::optional<sf::Sprite> mLoadingBarBgSprite; // [NEW]
    std::optional<sf::Sprite> mLoadingBarFillSprite; // [NEW]
    std::optional<sf::Sprite> mCloseButtonSprite;
    
    // [FORTIFY STATE]
    bool mIsButtonDown = false;
    bool mIsFortifying = false;
    sf::Clock mFortifyClock;
    
    // [STATUS MESSAGE]
    std::string mStatusMessage;
    sf::Color mStatusColor = sf::Color::White;
    sf::Clock mStatusClock;
    float mStatusDuration = 3.0f;

    bool mIsBeingDragged = false;
    sf::Vector2f mDragOffset;
    
    std::function<void()> mOnCloseCallback;
    std::function<void(Item&)> mOnFortifyCallback;
};
