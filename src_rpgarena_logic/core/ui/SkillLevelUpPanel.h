#pragma once
#include "UIPanel.h"
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <functional>
#include "../graphics/BitmapText.h"

class Player;
class SkillManager;
class Skill;

class SkillLevelUpPanel : public UIPanel {
public:
    SkillLevelUpPanel(sf::Texture* fontTexture);
    virtual ~SkillLevelUpPanel() = default;

    void draw(sf::RenderTarget& target, ResourceManager& res) override;
    sf::FloatRect getBounds() const override;
    void setPosition(sf::Vector2f pos) override;

    bool onMousePress(sf::Vector2f mousePos) override;
    void onMouseMove(sf::Vector2f mousePos) override;
    void onMouseRelease() override;
    bool isBeingDragged() const override { return mIsBeingDragged; }
    void onMouseWheel(int delta);

    void setPlayer(Player* player) { mPlayer = player; }
    void setSkillManager(const SkillManager* skillMgr) { mSkillMgr = skillMgr; }
    void setOnCloseCallback(std::function<void()> callback) { mOnCloseCallback = callback; }
    int getHoveredSkillId() const { return mHoveredSkillId; }

private:
    void drawText(sf::RenderTarget& target, const std::string& text, sf::Vector2f pos, float scale = 1.0f, sf::Color color = sf::Color::White);

    sf::Texture* mFontTexture = nullptr;
    Player* mPlayer = nullptr;
    const SkillManager* mSkillMgr = nullptr;
    std::function<void()> mOnCloseCallback;

    sf::Vector2f mPosition;
    sf::Vector2f mSize;

    bool mIsBeingDragged = false;
    sf::Vector2f mDragOffset;
    int mHoveredUpgradeIndex = -1;
    int mHoveredSkillId = -1;
    float mScrollOffset = 0.f;

    static constexpr float PADDING = 10.f;
    static constexpr float ROW_HEIGHT = 38.f;
    static constexpr float HEADER_HEIGHT = 28.f;
};
