#include "ContextMenu.h"
#include <iostream>
#include "core/graphics/BitmapText.h"
#include "Config.h"

ContextMenu::ContextMenu(sf::Texture* fontTexture)
    : mFontTexture(fontTexture)
{
}

void ContextMenu::show(sf::Vector2f position, const std::vector<std::string>& options, Callback callback) {
    mPosition = position;
    mOptions = options;
    mCallback = callback;
    mActive = true;
    mHoveredIndex = -1;

    // Calculate dynamic width and item height based on scaled text (like tooltip)
    BitmapText tempText;
    tempText.setTexture(mFontTexture);
    tempText.setScale({cfg::UI::FONT_SCALE, cfg::UI::FONT_SCALE});

    float maxWidth = 0.f;
    float maxItemHeight = 0.f;

    for (const auto& opt : mOptions) {
        tempText.setString(opt);
        sf::FloatRect bounds = tempText.getGlobalBounds();
        if (bounds.size.x > maxWidth) {
            maxWidth = bounds.size.x;
        }
        if (bounds.size.y > maxItemHeight) {
            maxItemHeight = bounds.size.y;
        }
    }

    // Centered alignment padding matching tooltip style
    mWidth = maxWidth + 20.f; // 10px padding on each side
    mItemHeight = maxItemHeight + 8.f; // 4px padding on top and bottom
}

void ContextMenu::hide() {
    mActive = false;
    mHoveredIndex = -1;
}

bool ContextMenu::handleEvent(const sf::Event& ev) {
    if (!mActive) return false;

    if (const auto* mb = ev.getIf<sf::Event::MouseButtonPressed>()) {
        if (mb->button == sf::Mouse::Button::Left) {
            if (mHoveredIndex >= 0 && mHoveredIndex < (int)mOptions.size()) {
                if (mCallback) {
                    mCallback(mOptions[mHoveredIndex]);
                }
                hide();
                return true; // Click consumido
            } else {
                // Click fuera cierra el menú
                hide();
                return true; // Consumido para evitar clicks en el mundo
            }
        }
    }
    return false;
}

void ContextMenu::update(sf::Vector2f mousePos) {
    if (!mActive) return;

    // Calcular hover
    float localX = mousePos.x - mPosition.x;
    float localY = mousePos.y - mPosition.y;

    if (localX >= 0 && localX <= mWidth &&
        localY >= 0 && localY <= mOptions.size() * mItemHeight) {
        mHoveredIndex = static_cast<int>(localY / mItemHeight);
    } else {
        mHoveredIndex = -1;
    }
}

void ContextMenu::draw(sf::RenderTarget& target) {
    if (!mActive) return;

    float totalHeight = mOptions.size() * mItemHeight;

    // Fondo (hereda del tooltip)
    sf::RectangleShape bg({mWidth, totalHeight});
    bg.setPosition(mPosition);
    bg.setFillColor(cfg::UI::Tooltip::BG_COLOR);
    bg.setOutlineThickness(cfg::UI::Tooltip::BORDER_SIZE);
    bg.setOutlineColor(cfg::UI::Tooltip::BORDER_COLOR);
    target.draw(bg);

    // Items
    BitmapText text;
    text.setTexture(mFontTexture);
    text.setScale({cfg::UI::FONT_SCALE, cfg::UI::FONT_SCALE});
    text.setColor(sf::Color::White);

    for (size_t i = 0; i < mOptions.size(); ++i) {
        sf::FloatRect itemRect({mPosition.x, mPosition.y + i * mItemHeight}, {mWidth, mItemHeight});

        // Highlight
        if ((int)i == mHoveredIndex) {
            sf::RectangleShape highlight({mWidth, mItemHeight});
            highlight.setPosition(itemRect.position);
            highlight.setFillColor(sf::Color(60, 60, 80, 200));
            target.draw(highlight);
        }

        text.setString(mOptions[i]);
        text.setPosition({itemRect.position.x + 10.f, itemRect.position.y + 4.f});
        target.draw(text);
    }
}

// [NEW]
sf::FloatRect ContextMenu::getBounds() const {
    if (!mActive) return sf::FloatRect();
    float totalHeight = mOptions.size() * mItemHeight;
    return sf::FloatRect(mPosition, {mWidth, totalHeight});
}
