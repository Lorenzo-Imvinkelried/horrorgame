#pragma once

#include "states/GameState.h"
#include "AnimatorStudio.h"
#include "AnimatorUI.h"
#include <SFML/Graphics.hpp>
#include <optional>

class Game;

enum class StudioDragMode {
    None,
    PanCamera,
    MoveBone,
    RotateBone
};

class AnimatorState : public GameState {
public:
    explicit AnimatorState(Game& game);
    ~AnimatorState() override = default;

    void handleInput(Game& game, sf::Time dt) override;
    void update(Game& game, sf::Time dt) override;
    
    void drawWorld(Game& game, sf::RenderTarget& target) override;
    void drawUI(Game& game, sf::RenderTarget& target) override;
    void draw(Game& game, sf::RenderTarget& target) override;
    
    void onResize(Game& game, int w, int h) override;
    void handleEvent(Game& game, const sf::Event& ev) override;

private:
    void resetCamera();

private:
    AnimatorStudio mStudio;
    AnimatorUI mUI;

    sf::View mWorldView;
    sf::View mUiView;

    float mZoomLevel = 2.5f;
    sf::Vector2f mCameraCenter = { 0.f, -10.f };

    StudioDragMode mDragMode = StudioDragMode::None;
    sf::Vector2i mPanStartMouse;
    sf::Vector2f mPanStartCenter;
    sf::Vector2f mLastWorldMouse;
    float mDragStartAngle = 0.f;

    // Custom Cursor
    std::optional<sf::Sprite> mCursorSprite;
    bool mHasCursor = false;
};
