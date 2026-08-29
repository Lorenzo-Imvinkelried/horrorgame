#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include "../graphics/BitmapText.h"

class LoadingScreen {
public:
    LoadingScreen();

    // Loads resources (font texture and bar textures)
    void load();

    // Displays the loading screen immediately (with initial progress 0)
    void show(sf::RenderWindow& window, const std::string& worldName);

    // Draws the loading screen with a specific progress (0.0 to 1.0)
    void draw(sf::RenderTarget& target, float progress, const std::string& worldName);

    // Waits until 'minDuration' has passed since 'show' was called, animating progress
    void waitForMinTime(sf::RenderWindow& window, sf::Time minDuration);

private:
    sf::Texture mFontTexture; // Usará assets/fonts/font.png
    BitmapText mTitleText;    // Usará BitmapText en lugar de sf::Text
    BitmapText mSubtitleText; // Usará BitmapText en lugar de sf::Text
    sf::RectangleShape mBackground;
    
    sf::Texture mBarBgTexture;
    sf::Texture mBarFillTexture;
    bool mTexturesLoaded = false;
    
    std::string mWorldName;
    sf::Clock mClock; // Measures time since 'show'
};
