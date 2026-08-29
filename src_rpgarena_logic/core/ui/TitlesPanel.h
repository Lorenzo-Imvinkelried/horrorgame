#pragma once
#include "UIPanel.h"
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <functional>
#include "../graphics/BitmapText.h"

class Player;
struct Title;

class TitlesPanel : public UIPanel {
public:
    TitlesPanel(sf::Texture* fontTexture);
    virtual ~TitlesPanel() = default;

    void draw(sf::RenderTarget& target, ResourceManager& res) override;
    sf::FloatRect getBounds() const override;
    void setPosition(sf::Vector2f pos) override;

    bool onMousePress(sf::Vector2f mousePos) override;
    void onMouseMove(sf::Vector2f mousePos) override;
    void onMouseRelease() override;
    bool isBeingDragged() const override { return mIsBeingDragged; }
    void onMouseWheel(int delta); // Dynamic Scrolling

    void setPlayer(Player* player) { mPlayer = player; }
    void setOnCloseCallback(std::function<void()> callback) { mOnCloseCallback = callback; }
    const Title* getHoveredTitle() const;

private:
    sf::Texture* mFontTexture = nullptr;
    Player* mPlayer = nullptr;
    std::function<void()> mOnCloseCallback;

    sf::Vector2f mPosition;
    sf::Vector2f mSize;

    bool mIsBeingDragged = false;
    sf::Vector2f mDragOffset;
    int mHoveredIndex = -1;
    float mScrollOffset = 0.f;

    static constexpr float PADDING = 10.f;
    static constexpr float ROW_HEIGHT = 24.f;
};
