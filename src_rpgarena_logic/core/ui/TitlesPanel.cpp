#include "TitlesPanel.h"
#include "entities/player/Player.h"
#include "core/managers/TitleManager.h"
#include "Config.h"
#include <algorithm>
#include <iostream>


TitlesPanel::TitlesPanel(sf::Texture* fontTexture)
    : mFontTexture(fontTexture)
{
    float zoom = cfg::Map::ZOOM_FACTOR;
    mSize = { 180.f * zoom, 240.f * zoom };
    mPosition = { 200.f, 150.f };
}

void TitlesPanel::setPosition(sf::Vector2f pos) {
    mPosition = pos;
}

sf::FloatRect TitlesPanel::getBounds() const {
    return sf::FloatRect(mPosition, mSize);
}

void TitlesPanel::draw(sf::RenderTarget& target, ResourceManager& res) {
    float zoom = cfg::Map::ZOOM_FACTOR;
    mSize = { 180.f * zoom, 240.f * zoom };

    const float panelX = std::floor(mPosition.x);
    const float panelY = std::floor(mPosition.y);
    const float padding = PADDING * zoom;

    // 1. Draw Panel Background
    sf::RectangleShape bg(mSize);
    bg.setPosition({panelX, panelY});
    bg.setFillColor(sf::Color(15, 15, 20, 230)); // Acrylic/Glassmorphism feel
    bg.setOutlineThickness(1.5f * zoom);
    bg.setOutlineColor(sf::Color(112, 84, 8, 255)); // Theme Gold Border
    target.draw(bg);

    // 2. Draw Panel Header
    BitmapText headerText;
    headerText.setTexture(mFontTexture);
    headerText.setScale({cfg::UI::FONT_SCALE, cfg::UI::FONT_SCALE});
    headerText.setString("TITULOS");
    headerText.setColor(sf::Color(255, 215, 0)); // Gold
    sf::FloatRect headerBounds = headerText.getLocalBounds();
    headerText.setPosition({
        std::floor(panelX + (mSize.x - headerBounds.size.x * cfg::UI::FONT_SCALE) * 0.5f),
        std::floor(panelY + padding)
    });
    target.draw(headerText);

    // 3. Draw Close Button
    sf::FloatRect closeBtnRect(
        { panelX + mSize.x - 20.f * zoom, panelY + padding },
        { 14.f * zoom, 14.f * zoom }
    );
    BitmapText closeText;
    closeText.setTexture(mFontTexture);
    closeText.setScale({cfg::UI::FONT_SCALE, cfg::UI::FONT_SCALE});
    closeText.setString("X");
    closeText.setColor(sf::Color(180, 0, 0));
    closeText.setPosition({closeBtnRect.position.x + 2.f * zoom, closeBtnRect.position.y});
    target.draw(closeText);

    // 4. Draw Separating Line
    sf::RectangleShape line({mSize.x - 2.f * padding, 1.5f * zoom});
    line.setPosition({panelX + padding, panelY + 28.f * zoom});
    line.setFillColor(sf::Color(112, 84, 8, 150));
    target.draw(line);

    // 5. Draw Titles List
    const auto& titles = TitleManager::getInstance().getAllTitles();
    float rowHeight = ROW_HEIGHT * zoom;
    float listStartY = panelY + 32.f * zoom + mScrollOffset;
    float listBottomY = panelY + mSize.y - padding;

    for (size_t i = 0; i < titles.size(); ++i) {
        const Title& title = titles[i];
        float rowY = listStartY + i * rowHeight;

        // Clip items outside the scrollable view area
        if (rowY < panelY + 28.f * zoom || rowY + rowHeight > listBottomY + 4.f * zoom) {
            continue;
        }

        sf::FloatRect rowBounds({panelX + padding, rowY}, {mSize.x - 2.f * padding, rowHeight - 4.f * zoom});

        bool isEquipped = (mPlayer && mPlayer->getActiveTitleId() == title.id);

        if (isEquipped) {
            sf::RectangleShape rowBg({rowBounds.size.x, rowBounds.size.y});
            rowBg.setPosition(rowBounds.position);
            rowBg.setFillColor(sf::Color(218, 165, 32, 30)); // Soft gold highlight without borders
            target.draw(rowBg);
        } else if (mHoveredIndex == static_cast<int>(i)) {
            sf::RectangleShape rowBg({rowBounds.size.x, rowBounds.size.y});
            rowBg.setPosition(rowBounds.position);
            rowBg.setFillColor(sf::Color(255, 255, 255, 15)); // Soft hover highlight without borders
            target.draw(rowBg);
        }

        // Draw Title Name Text
        BitmapText titleText;
        titleText.setTexture(mFontTexture);
        titleText.setScale({cfg::UI::FONT_SCALE, cfg::UI::FONT_SCALE});
        std::string displayName = isEquipped ? "> " + title.name : title.name;
        titleText.setString(displayName);
        titleText.setColor(title.color);
        titleText.setPosition({rowBounds.position.x + 6.f * zoom, rowBounds.position.y + 4.f * zoom});
        target.draw(titleText);

    }
}

bool TitlesPanel::onMousePress(sf::Vector2f mousePos) {
    float zoom = cfg::Map::ZOOM_FACTOR;
    float padding = PADDING * zoom;

    // 1. Close button click check
    sf::FloatRect closeBtnRect(
        { mPosition.x + mSize.x - 20.f * zoom, mPosition.y + padding },
        { 14.f * zoom, 14.f * zoom }
    );
    if (closeBtnRect.contains(mousePos)) {
        if (mOnCloseCallback) mOnCloseCallback();
        return true;
    }

    // 2. Drag check (Header area)
    sf::FloatRect titleBar(mPosition, { mSize.x, 30.f * zoom });
    if (titleBar.contains(mousePos)) {
        mIsBeingDragged = true;
        mDragOffset = mousePos - mPosition;
        return true;
    }

    // 3. Selection row check
    const auto& titles = TitleManager::getInstance().getAllTitles();
    float rowHeight = ROW_HEIGHT * zoom;
    float listStartY = mPosition.y + 32.f * zoom + mScrollOffset;
    float listBottomY = mPosition.y + mSize.y - padding;

    for (size_t i = 0; i < titles.size(); ++i) {
        float rowY = listStartY + i * rowHeight;
        if (rowY < mPosition.y + 28.f * zoom || rowY + rowHeight > listBottomY + 4.f * zoom) {
            continue; // Clipped, ignore clicks
        }

        sf::FloatRect rowBounds({mPosition.x + padding, rowY}, {mSize.x - 2.f * padding, rowHeight - 4.f * zoom});
        if (rowBounds.contains(mousePos)) {
            if (mPlayer) {
                if (mPlayer->getActiveTitleId() == titles[i].id) {
                    mPlayer->setActiveTitle(""); // Toggle unequip
                    std::cout << "[TitlesPanel] Unequipped title.\n";
                } else {
                    mPlayer->setActiveTitle(titles[i].id); // Equip
                    std::cout << "[TitlesPanel] Equipped title: " << titles[i].name << "\n";
                }
            }
            return true;
        }
    }

    // Consume press inside panel
    if (getBounds().contains(mousePos)) {
        return true;
    }

    return false;
}

void TitlesPanel::onMouseMove(sf::Vector2f mousePos) {
    if (mIsBeingDragged) {
        mPosition.x = std::floor(mousePos.x - mDragOffset.x);
        mPosition.y = std::floor(mousePos.y - mDragOffset.y);
    }

    // Update hovered row index
    mHoveredIndex = -1;
    if (getBounds().contains(mousePos)) {
        float zoom = cfg::Map::ZOOM_FACTOR;
        float padding = PADDING * zoom;
        const auto& titles = TitleManager::getInstance().getAllTitles();
        float rowHeight = ROW_HEIGHT * zoom;
        float listStartY = mPosition.y + 32.f * zoom + mScrollOffset;
        float listBottomY = mPosition.y + mSize.y - padding;

        for (size_t i = 0; i < titles.size(); ++i) {
            float rowY = listStartY + i * rowHeight;
            if (rowY < mPosition.y + 28.f * zoom || rowY + rowHeight > listBottomY + 4.f * zoom) {
                continue;
            }
            sf::FloatRect rowBounds({mPosition.x + padding, rowY}, {mSize.x - 2.f * padding, rowHeight - 4.f * zoom});
            if (rowBounds.contains(mousePos)) {
                mHoveredIndex = static_cast<int>(i);
                break;
            }
        }
    }
}

void TitlesPanel::onMouseRelease() {
    mIsBeingDragged = false;
}

void TitlesPanel::onMouseWheel(int delta) {
    float zoom = cfg::Map::ZOOM_FACTOR;
    const auto& titles = TitleManager::getInstance().getAllTitles();
    float totalListHeight = titles.size() * ROW_HEIGHT * zoom;
    float padding = PADDING * zoom;
    float visibleHeight = mSize.y - 32.f * zoom - padding;

    // Only scroll if contents exceed the visible height
    if (totalListHeight > visibleHeight) {
        float scrollSpeed = 15.f * zoom;
        mScrollOffset += delta * scrollSpeed;

        // Clamp scroll offset
        float maxScroll = 0.f;
        float minScroll = visibleHeight - totalListHeight;
        mScrollOffset = std::clamp(mScrollOffset, minScroll, maxScroll);
    } else {
        mScrollOffset = 0.f;
    }
}

const Title* TitlesPanel::getHoveredTitle() const {
    if (mHoveredIndex < 0) return nullptr;
    const auto& titles = TitleManager::getInstance().getAllTitles();
    if (static_cast<size_t>(mHoveredIndex) >= titles.size()) return nullptr;
    return &titles[mHoveredIndex];
}
