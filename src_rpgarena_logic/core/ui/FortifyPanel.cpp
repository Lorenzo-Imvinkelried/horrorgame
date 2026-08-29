#include "FortifyPanel.h"
#include "../../Config.h"
#include "../systems/FortifySystem.h"
#include "../graphics/BitmapText.h"
#include <iostream>

FortifyPanel::FortifyPanel(sf::Texture* fontTexture, std::optional<sf::Sprite>& slotSprite)
    : mFontTexture(fontTexture), mSlotSprite(slotSprite)
{
    mPosition = { cfg::UI::FortifyPanel::X, cfg::UI::FortifyPanel::Y };
    mSize = { 128.f, 128.f }; // Default size
}

void FortifyPanel::draw(sf::RenderTarget& target, ResourceManager& res) {
    if (!mVisible) return;

    float zoom = cfg::Map::ZOOM_FACTOR;

    // 1. Draw Background
    try {
        sf::Texture& bgTex = res.getTexture("assets/ui/fortify_bg.png");
        if (!mBackgroundSprite) mBackgroundSprite.emplace(bgTex);
        mBackgroundSprite->setTexture(bgTex, true);
        mBackgroundSprite->setScale({zoom, zoom});
        mBackgroundSprite->setPosition(mPosition);
        mSize = mBackgroundSprite->getGlobalBounds().size;
        target.draw(*mBackgroundSprite);
    } catch (...) {
        sf::RectangleShape bg(mSize);
        bg.setPosition(mPosition);
        bg.setFillColor(sf::Color(40, 40, 45, 200));
        bg.setOutlineThickness(1.f);
        bg.setOutlineColor(sf::Color(100, 100, 100));
        target.draw(bg);
    }

    // 2. Draw Slot
    sf::Vector2f slotPos = mPosition + sf::Vector2f(cfg::UI::FortifyPanel::SLOT_OFFSET_X * zoom, cfg::UI::FortifyPanel::SLOT_OFFSET_Y * zoom);
    if (mSlotSprite) {
        mSlotSprite->setPosition(slotPos);
        mSlotSprite->setScale({zoom, zoom});
        target.draw(*mSlotSprite);
    } else {
        drawSlotFallback(target, slotPos);
    }

    // 3. Draw Item icon if exists
    if (mItem) {
        try {
            sf::Texture& tex = res.getTexture(mItem->texturePath);
            sf::Sprite icon(tex);
            if (mItem->textureRect.size.x > 0) icon.setTextureRect(mItem->textureRect);
            
            icon.setScale({zoom, zoom});
            // Center in slot (Assuming slot is 20x20 in art, so 20*zoom in world)
            float slotPx = 20.f * zoom; 
            sf::FloatRect b = icon.getGlobalBounds();
            icon.setPosition({slotPos.x + (slotPx - b.size.x)/2.f, slotPos.y + (slotPx - b.size.y)/2.f});
            
            // Draw aura if fortified >= 6
            if (mItem->fortificationLevel >= 6) {
                ItemAuraRenderer::drawAura(target, icon, mItem->fortificationLevel, zoom);
            }

            target.draw(icon);

            // [NEW] Draw Quality Overlay
            if (mItem->quality != ItemQuality::Common && mItem->type == ItemType::Weapon) {
                 try {
                     sf::Texture& qTex = res.getTexture("assets/items/weapons/weapons_layout.png");
                     sf::Sprite qIcon(qTex);
                     
                     if (mItem->textureRect.size.x > 0) qIcon.setTextureRect(mItem->textureRect);
                     
                     qIcon.setScale(icon.getScale());
                     qIcon.setPosition(icon.getPosition());
                     
                     sf::Color qualityColor = getQualityColor(mItem->quality);
                     qIcon.setColor(qualityColor);

                     target.draw(qIcon);
                 } catch(...) {}
            }
        } catch (...) {}
    }

    // 4. Draw button
    sf::Vector2f btnPos = mPosition + sf::Vector2f(cfg::UI::FortifyPanel::BUTTON_OFFSET_X * zoom, cfg::UI::FortifyPanel::BUTTON_OFFSET_Y * zoom);
    try {
        if (mIsButtonDown) {
            sf::Texture& btnDownTex = res.getTexture("assets/ui/fortify_button_click.png");
            if (!mButtonDownSprite) mButtonDownSprite.emplace(btnDownTex);
            mButtonDownSprite->setTexture(btnDownTex, true);
            mButtonDownSprite->setScale({zoom, zoom});
            mButtonDownSprite->setPosition(btnPos);
            target.draw(*mButtonDownSprite);
        } else {
            sf::Texture& btnTex = res.getTexture("assets/ui/fortify_button.png");
            if (!mButtonSprite) mButtonSprite.emplace(btnTex);
            mButtonSprite->setTexture(btnTex, true);
            mButtonSprite->setScale({zoom, zoom});
            mButtonSprite->setPosition(btnPos);
            target.draw(*mButtonSprite);
        }
    } catch (...) {
        sf::RectangleShape btn({60.f * zoom, 20.f * zoom});
        btn.setPosition(btnPos);
        btn.setFillColor(mIsButtonDown ? sf::Color(150, 80, 80) : sf::Color(100, 50, 50));
        target.draw(btn);
    }
    
    // [NEW] Draw Loading Bar Base (Always visible)
    sf::Vector2f barPos = mPosition + sf::Vector2f(cfg::UI::FortifyPanel::LOADING_BAR_OFFSET_X * zoom, cfg::UI::FortifyPanel::LOADING_BAR_OFFSET_Y * zoom);
    try {
        sf::Texture& barBgTex = res.getTexture("assets/ui/fortify_loading_bar.png");
        if (!mLoadingBarBgSprite) mLoadingBarBgSprite.emplace(barBgTex);
        mLoadingBarBgSprite->setTexture(barBgTex, true);
        mLoadingBarBgSprite->setScale({zoom, zoom});
        mLoadingBarBgSprite->setPosition(barPos);
        target.draw(*mLoadingBarBgSprite);
    } catch(...) {
        // Fallback loading bar background
        sf::RectangleShape bg({100.f * zoom, 10.f * zoom});
        bg.setPosition(barPos);
        bg.setFillColor(sf::Color(50, 50, 50));
        target.draw(bg);
    }

    // Draw Loading Bar Fill and handle completion if fortifying
    if (mIsFortifying && mItem) {
        float elapsed = mFortifyClock.getElapsedTime().asSeconds();
        float duration = cfg::UI::FortifyPanel::LOADING_TIME_SECONDS;
        float percent = std::min(elapsed / duration, 1.0f);
        
        try {
            // Draw fill
            sf::Texture& barFillTex = res.getTexture("assets/ui/fortify_loading_bar_up.png");
            if (!mLoadingBarFillSprite) mLoadingBarFillSprite.emplace(barFillTex);
            mLoadingBarFillSprite->setTexture(barFillTex, true);
            
            // Crop based on percentage
            sf::Vector2u fillSize = barFillTex.getSize();
            int croppedWidth = static_cast<int>(fillSize.x * percent);
            mLoadingBarFillSprite->setTextureRect(sf::IntRect({0, 0}, {croppedWidth, static_cast<int>(fillSize.y)}));
            
            mLoadingBarFillSprite->setScale({zoom, zoom});
            mLoadingBarFillSprite->setPosition(barPos);
            target.draw(*mLoadingBarFillSprite);
        } catch(...) {
            // Fallback loading bar fill
            sf::RectangleShape fill({100.f * percent * zoom, 10.f * zoom});
            fill.setPosition(barPos);
            fill.setFillColor(sf::Color(0, 200, 0));
            target.draw(fill);
        }
        
        // Complete fortify
        if (percent >= 1.0f) {
            if (mOnFortifyCallback) {
                mOnFortifyCallback(*mItem);
            }
            mIsFortifying = false;
            mIsButtonDown = false;
        }
    }

    // 5. Draw Close Button
    sf::Vector2f closeBtnPos = mPosition + sf::Vector2f(cfg::UI::FortifyPanel::CLOSE_BTN_X * zoom, cfg::UI::FortifyPanel::CLOSE_BTN_Y * zoom);
    try {
        sf::Texture& closeTex = res.getTexture("assets/ui/button_close.png");
        if (!mCloseButtonSprite) mCloseButtonSprite.emplace(closeTex);
        mCloseButtonSprite->setTexture(closeTex, true);
        mCloseButtonSprite->setScale({zoom, zoom});
        mCloseButtonSprite->setPosition(closeBtnPos);
        target.draw(*mCloseButtonSprite);
    } catch (...) {
        sf::RectangleShape close({cfg::UI::FortifyPanel::CLOSE_BTN_SIZE * zoom, cfg::UI::FortifyPanel::CLOSE_BTN_SIZE * zoom});
        close.setPosition(closeBtnPos);
        close.setFillColor(sf::Color(150, 50, 50));
        target.draw(close);
    }

    // 6. Draw Status Message
    if (!mStatusMessage.empty() && mStatusClock.getElapsedTime().asSeconds() < mStatusDuration) {
        BitmapText statusTxt;
        statusTxt.setTexture(mFontTexture);
        statusTxt.setScale({zoom * 0.5f, zoom * 0.5f}); // Smaller text
        statusTxt.setColor(mStatusColor);
        statusTxt.setString(mStatusMessage);
        
        // Position below the button
        statusTxt.setPosition({mPosition.x + 10.f * zoom, btnPos.y + 25.f * zoom});
        target.draw(statusTxt);
    }
}

sf::FloatRect FortifyPanel::getBounds() const {
    return sf::FloatRect(mPosition, mSize);
}

void FortifyPanel::setPosition(sf::Vector2f pos) {
    mPosition = pos;
}

bool FortifyPanel::onMousePress(sf::Vector2f mousePos) {
    if (!mVisible) return false;

    float zoom = cfg::Map::ZOOM_FACTOR;

    // 1. Check for close button (Highest Priority)
    sf::Vector2f closeBtnPos = mPosition + sf::Vector2f(cfg::UI::FortifyPanel::CLOSE_BTN_X * zoom, cfg::UI::FortifyPanel::CLOSE_BTN_Y * zoom);
    // Use exact configured size
    sf::FloatRect closeBounds(closeBtnPos, {cfg::UI::FortifyPanel::CLOSE_BTN_SIZE * zoom, cfg::UI::FortifyPanel::CLOSE_BTN_SIZE * zoom}); 
    if (closeBounds.contains(mousePos)) {
        if (mOnCloseCallback) mOnCloseCallback();
        else mVisible = false; // Fallback
        return true;
    }

    // 2. Check for close/drag (standard title bar area)
    sf::FloatRect titleBar(mPosition, {mSize.x, 30.f * zoom});
    if (titleBar.contains(mousePos)) {
        mIsBeingDragged = true;
        mDragOffset = mousePos - mPosition;
        return true;
    }

    // Check for button press
    sf::Vector2f btnPos = mPosition + sf::Vector2f(cfg::UI::FortifyPanel::BUTTON_OFFSET_X * zoom, cfg::UI::FortifyPanel::BUTTON_OFFSET_Y * zoom);
    sf::FloatRect btnBounds(btnPos, {60.f * zoom, 20.f * zoom});
    if (btnBounds.contains(mousePos)) {
        mIsButtonDown = true;
        if (mItem && !mIsFortifying) {
            mIsFortifying = true;
            mFortifyClock.restart();
        }
        return true;
    }

    if (getBounds().contains(mousePos)) return true;

    return false;
}

void FortifyPanel::onMouseMove(sf::Vector2f mousePos) {
    if (mIsBeingDragged) {
        mPosition = mousePos - mDragOffset;
    }
    
    // Cancel fortify if mouse moves out of button
    if (mIsButtonDown) {
        float zoom = cfg::Map::ZOOM_FACTOR;
        sf::Vector2f btnPos = mPosition + sf::Vector2f(cfg::UI::FortifyPanel::BUTTON_OFFSET_X * zoom, cfg::UI::FortifyPanel::BUTTON_OFFSET_Y * zoom);
        sf::FloatRect btnBounds(btnPos, {60.f * zoom, 20.f * zoom});
        if (!btnBounds.contains(mousePos)) {
            mIsButtonDown = false;
        }
    }
}

void FortifyPanel::onMouseRelease() {
    mIsBeingDragged = false;
    mIsButtonDown = false;
}

void FortifyPanel::setItem(std::shared_ptr<Item> item) {
    mItem = item;
    if (!mItem) {
        mIsFortifying = false;
    }
}

bool FortifyPanel::isMouseOverSlot(sf::Vector2f mousePos) const {
    if (!mVisible) return false;
    float zoom = cfg::Map::ZOOM_FACTOR;
    sf::Vector2f slotPos = mPosition + sf::Vector2f(cfg::UI::FortifyPanel::SLOT_OFFSET_X * zoom, cfg::UI::FortifyPanel::SLOT_OFFSET_Y * zoom);
    sf::FloatRect slotBounds(slotPos, {20.f * zoom, 20.f * zoom});
    return slotBounds.contains(mousePos);
}

void FortifyPanel::setVisible(bool visible) {
    UIPanel::setVisible(visible);
    if (!visible) {
        mIsBeingDragged = false;
    }
}

void FortifyPanel::showStatus(const std::string& msg, sf::Color color) {
    mStatusMessage = msg;
    mStatusColor = color;
    mStatusClock.restart();
}

void FortifyPanel::drawSlotFallback(sf::RenderTarget& target, sf::Vector2f pos) {
    float zoom = cfg::Map::ZOOM_FACTOR;
    sf::RectangleShape r({20.f * zoom, 20.f * zoom});
    r.setPosition(pos);
    r.setFillColor(sf::Color(20, 20, 20, 180));
    r.setOutlineThickness(1.f);
    r.setOutlineColor(sf::Color(80, 80, 80));
    target.draw(r);
}
