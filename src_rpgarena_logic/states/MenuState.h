#pragma once

#include "GameState.h"
#include "../core/graphics/BitmapText.h"
#include "../core/ui/LoadingScreen.h"
#include <SFML/Graphics.hpp>
#include <optional>

class Game;

class MenuState : public GameState {
public:
    explicit MenuState(Game &game);
    ~MenuState() override = default;

    void handleInput(Game &game, sf::Time dt) override;
    void update(Game &game, sf::Time dt) override;
    
    void drawWorld(Game &game, sf::RenderTarget &target) override;
    void drawUI(Game &game, sf::RenderTarget &target) override;
    void draw(Game &game, sf::RenderTarget &target) override;
    
    void onResize(Game &game, int w, int h) override;
    void handleEvent(Game &game, const sf::Event &ev) override;

private:
    sf::Texture *mFontTexture = nullptr;
    BitmapText mTitleText;
    BitmapText mPlayText;
    
    sf::RectangleShape mPlayButton;
    sf::RectangleShape mAnimatorButton; // [ANIMATOR STUDIO]
    sf::RectangleShape mMuteButton; // [NEW]
    
    BitmapText mAnimatorText; // [ANIMATOR STUDIO]
    BitmapText mMuteText; // [NEW]
    
    sf::View mView;
    
    // Custom Cursor
    std::optional<sf::Sprite> mCursorSprite;
    bool mHasCursor = false;
    
    bool mHoveringPlay = false;
    bool mHoveringAnimator = false; // [ANIMATOR STUDIO]
    bool mHoveringMute = false; // [NEW]
};