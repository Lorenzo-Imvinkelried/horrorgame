#pragma once
#include <SFML/Graphics.hpp>

// Forward declaration
class Game;

class GameState {
public:
    virtual ~GameState() = default;

    // Métodos vitales que Game llamará
    virtual void handleInput(Game& game, sf::Time dt) = 0;
    virtual void update(Game& game, sf::Time dt) = 0;
    
    // [RENDER SPLIT] High Res UI vs Low Res World
    virtual void drawWorld(Game& game, sf::RenderTarget& target) = 0; // Target = Low Res Texture
    virtual void drawUI(Game& game, sf::RenderTarget& target) = 0;    // Target = High Res Window
    
    // Deprecated single draw
    virtual void draw(Game& game, sf::RenderTarget& target) { 
        drawWorld(game, target); 
        drawUI(game, target); 
    }
    virtual void onResize(Game& game, int w, int h) {} // Opcional
    virtual void handleEvent(Game& game, const sf::Event& ev) {}
};