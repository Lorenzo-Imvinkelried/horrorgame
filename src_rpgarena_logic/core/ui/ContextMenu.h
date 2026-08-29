#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <functional>

class ContextMenu {
public:
    using Callback = std::function<void(const std::string&)>;

    ContextMenu(sf::Texture* fontTexture);

    // Muestra el menú en la posición dada con las opciones
    void show(sf::Vector2f position, const std::vector<std::string>& options, Callback callback);
    void hide();

    // Devuelve true si el click fue consumido
    bool handleEvent(const sf::Event& ev);
    
    // Update puede ser útil para hover effects
    void update(sf::Vector2f mousePos);

    void draw(sf::RenderTarget& target);

    bool isActive() const { return mActive; }
    sf::FloatRect getBounds() const;

private:
    sf::Texture* mFontTexture = nullptr;
    bool mActive = false;
    sf::Vector2f mPosition;
    std::vector<std::string> mOptions;
    Callback mCallback;

    // Visuals
    float mWidth = 150.f;
    float mItemHeight = 25.f;
    int mHoveredIndex = -1;
};
