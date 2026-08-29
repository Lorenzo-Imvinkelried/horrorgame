// ExpBar.cpp
#include "ExpBar.h"
#include <algorithm>
#include <string>
#include <sstream>
#include <iomanip> // Para setprecision
#include <cmath>   // Para floor

#include "Config.h" // <-- Importante
#include "../engine/ResourceManager.h" // <-- Necesario para cargar texturas

// Constructor recibe fuente
ExpBar::ExpBar(sf::Texture* fontTexture) 
    : mFontTexture(fontTexture)    
{
    // Cargar config
    mHeight        = cfg::UI::EXP_BAR_HEIGHT;
    mBottomOffset  = cfg::UI::EXP_BAR_BOTTOM_OFFSET;

    // 1. Configurar Barras
    mBackground.setFillColor(sf::Color(20, 20, 20, 255));
    mBackground.setOutlineThickness(1.0f);
    mBackground.setOutlineColor(sf::Color::Black);

    mFill.setFillColor(sf::Color(180, 50, 250)); // Violeta

    // 2. Configurar Texto
    mText.setTexture(mFontTexture);
    mText.setScale({cfg::UI::FONT_SCALE, cfg::UI::FONT_SCALE}); 
    mText.setColor(sf::Color::White);
}

void ExpBar::load(ResourceManager& res) {
    try {
        mBgTexture = &res.getTexture("assets/ui/exp_bar_bg.png");
        mFillTexture = &res.getTexture("assets/ui/exp_bar_fill.png");
        mTexturesLoaded = true;
    } catch (...) {
        mTexturesLoaded = false;
        mBgTexture = nullptr;
        mFillTexture = nullptr;
    }
}

void ExpBar::updateLayout(float windowWidth, float windowHeight) {
    // 1. Calcular tamaño de slots (Mismo que en Hud.cpp) para alineamiento
    float zoom = cfg::Map::ZOOM_FACTOR;
    float slotSize = cfg::UI::BASE_SLOT_SIZE * zoom;

    // 2. Calcular ancho total del Hotbar
    // Width = (Slots * Size) + ((Slots-1) * Margin)
    float totalW = cfg::UI::HOTBAR_SLOTS * slotSize + (cfg::UI::HOTBAR_SLOTS - 1) * cfg::UI::SLOT_MARGIN;
    
    // Robustez: asegurar que no sea negativo
    if (totalW < 10.f) totalW = 10.f;

    // 3. Centrar la barra
    float posX = (windowWidth - totalW) / 2.0f;
    float posY = windowHeight - mBottomOffset * zoom;

    float barH = mHeight * zoom; // Altura escalada íntegra (x3)
    
    if (mTexturesLoaded && mBgTexture && mFillTexture) {
        // [PIXEL PERFECT] El usuario provee la textura. Forzamos su tamaño por zoom
        totalW = mBgTexture->getSize().x * zoom;
        barH = mBgTexture->getSize().y * zoom;
        posX = (windowWidth - totalW) / 2.0f; // Recalculamos posición centrándola con el tamaño real de la imagen
    }
    
    mBackground.setSize({totalW, barH});
    mBackground.setPosition({posX, posY});
    mFill.setPosition({posX, posY});
    
    // Mantenemos la altura del relleno actualizada
    float currentFillW = mFill.getSize().x;
    mFill.setSize({currentFillW, barH});
}

void ExpBar::setProgress(float currentExp, float nextLevelExp) {
    if (nextLevelExp <= 0.f) nextLevelExp = 1.f;

    // 1. Actualizar Barra
    float ratio = currentExp / nextLevelExp;
    ratio = std::clamp(ratio, 0.0f, 1.0f);
    float maxWidth = mBackground.getSize().x;
    
    mFill.setSize({maxWidth * ratio, mBackground.getSize().y}); // Sincronizo altura con la del fondo

    // 2. Actualizar Texto: "150 / 200 (75%)"
    float percent = ratio * 100.0f;
    
    std::stringstream ss;
    ss << (int)currentExp << " / " << (int)nextLevelExp << " (" << std::fixed << std::setprecision(1) << percent << "%)";
    mText.setString(ss.str());

    // 3. Centrar el texto en la barra
    const float fScale = cfg::UI::FONT_SCALE;
    mText.setScale({fScale, fScale});
    sf::FloatRect textBounds = mText.getLocalBounds();
    float zoom = cfg::Map::ZOOM_FACTOR;
    
    // Centrar en el fondo de la barra
    float textX = std::floor(mBackground.getPosition().x + (mBackground.getSize().x - textBounds.size.x * fScale) * 0.5f);
    float textY = std::floor(mBackground.getPosition().y + (mBackground.getSize().y - textBounds.size.y * fScale) * 0.5f);
    mText.setPosition({textX, textY});
}

void ExpBar::draw(sf::RenderTarget& target) const {
    if (mTexturesLoaded && mBgTexture && mFillTexture) {
        float zoom = cfg::Map::ZOOM_FACTOR;
        sf::Vector2f pos = mBackground.getPosition(); // Obtenemos pos calculada en Layout
        
        // 1. Dibujamos Fondo
        sf::Sprite bgSprite(*mBgTexture);
        bgSprite.setPosition(pos);
        bgSprite.setScale({zoom, zoom});
        target.draw(bgSprite);
        
        // 2. Dibujamos Fill (Slicing)
        sf::Sprite fillSprite(*mFillTexture);
        const sf::Vector2u texSize = mFillTexture->getSize();
        
        // Ratio calculado desde el RectangleShape (mFillWidth / mBgWidth)
        float ratio = mFill.getSize().x / mBackground.getSize().x;
        int sliceWidth = static_cast<int>(texSize.x * ratio);
        
        // Cortamos el ancho según porcentaje
        fillSprite.setTextureRect(sf::IntRect({0, 0}, {sliceWidth, (int)texSize.y}));
        fillSprite.setPosition(pos);
        fillSprite.setScale({zoom, zoom});
        target.draw(fillSprite);
        
    } else {
        target.draw(mBackground);
        target.draw(mFill);
    }
    
    target.draw(mText); // Dibujar texto al final en ambos casos
}