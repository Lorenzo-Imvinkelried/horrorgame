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

class SkillDebugPanel : public UIPanel {
public:
    SkillDebugPanel(sf::Texture* fontTexture);
    virtual ~SkillDebugPanel() = default;

    void draw(sf::RenderTarget& target, ResourceManager& res) override;
    sf::FloatRect getBounds() const override;
    void setPosition(sf::Vector2f pos) override;

    bool onMousePress(sf::Vector2f mousePos) override;
    void onMouseMove(sf::Vector2f mousePos) override;
    void onMouseRelease() override;
    void onMouseWheel(int delta);
    bool isBeingDragged() const override { return mIsBeingDragged; }

    // Key handling for hotbar assignment (1-9, 0)
    bool onKeyPress(sf::Keyboard::Key key);

    void setPlayer(Player* player) { mPlayer = player; }
    void setSkillManager(const SkillManager* skillMgr) { mSkillMgr = skillMgr; }
    void setOnCloseCallback(std::function<void()> callback) { mOnCloseCallback = callback; }

    int getSelectedSkillId() const { return mSelectedSkillId; }
    void setSelectedSkillId(int id) { mSelectedSkillId = id; }
    int getHoveredSkillId() const { return mHoveredSkillId; }

private:
    void drawText(sf::RenderTarget& target, const std::string& text, sf::Vector2f pos, float scale = 1.0f, sf::Color color = sf::Color::White);

    sf::Texture* mFontTexture = nullptr;
    Player* mPlayer = nullptr;
    const SkillManager* mSkillMgr = nullptr;
    std::function<void()> mOnCloseCallback;

    sf::Vector2f mPosition{ 100.f, 80.f };
    sf::Vector2f mSize{ 400.f, 340.f };

    bool mIsBeingDragged = false;
    sf::Vector2f mDragOffset;

    int mSelectedSkillId = -1;
    int mHoveredSkillId = -1;
    float mScrollOffset = 0.f;

    static constexpr float PADDING = 10.f;
    static constexpr float HEADER_HEIGHT = 28.f;
};
