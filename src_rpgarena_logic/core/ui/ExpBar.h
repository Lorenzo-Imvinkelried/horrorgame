#pragma once
#include <SFML/Graphics.hpp>
#include "core/graphics/BitmapText.h"

class ResourceManager;

class ExpBar {
public:
    ExpBar(sf::Texture* fontTexture);

    // Carga de recursos
    void load(ResourceManager& res);

    // Configura tamaño y posición (se llamará al iniciar y al redimensionar ventana)
    void updateLayout(float windowWidth, float windowHeight);

    // Actualiza el progreso visual (0.0 a 1.0)
    // currentExp: experiencia actual
    // nextLevelExp: experiencia necesaria para el siguiente nivel
    void setProgress(float currentExp, float nextLevelExp);

    void draw(sf::RenderTarget& target) const;

private:

    sf::Texture* mFontTexture = nullptr;
    BitmapText   mText; 

    sf::RectangleShape mBackground;
    sf::RectangleShape mFill;

    // Configuración visual
    // Configuración visual
    // Configuración visual
    float mHeight; 
    float mBottomOffset; 

    // Opcional Texturas
    const sf::Texture* mBgTexture = nullptr;
    const sf::Texture* mFillTexture = nullptr;
    bool mTexturesLoaded = false;
};