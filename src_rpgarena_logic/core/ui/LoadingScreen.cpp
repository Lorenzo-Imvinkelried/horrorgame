#include "LoadingScreen.h"
#include "Config.h"
#include <iostream>
#include <cmath>
#include <algorithm>

LoadingScreen::LoadingScreen() 
{
    mBackground.setFillColor(sf::Color::Black);
}

void LoadingScreen::load() {
    // Cargar la textura de fuente pixel-art del juego
    if (!mFontTexture.loadFromFile("assets/fonts/font.png")) {
        std::cerr << "[LoadingScreen] WARN: Could not load assets/fonts/font.png\n";
    }
    mFontTexture.setSmooth(false);
    
    // Configurar texturas en los textos bitmap
    mTitleText.setTexture(&mFontTexture);
    mTitleText.setScale({cfg::UI::FONT_SCALE, cfg::UI::FONT_SCALE});
    mTitleText.setColor(sf::Color::White);
    
    mSubtitleText.setTexture(&mFontTexture);
    mSubtitleText.setScale({cfg::UI::FONT_SCALE, cfg::UI::FONT_SCALE});
    mSubtitleText.setColor(sf::Color(200, 200, 200));

    // Cargar texturas de la barra de carga
    if (!mBarBgTexture.loadFromFile("assets/ui/loading_bar_bg.png")) {
        std::cerr << "[LoadingScreen] WARN: Could not load loading_bar_bg.png\n";
        mTexturesLoaded = false;
    } else if (!mBarFillTexture.loadFromFile("assets/ui/loading_bar_fill.png")) {
        std::cerr << "[LoadingScreen] WARN: Could not load loading_bar_fill.png\n";
        mTexturesLoaded = false;
    } else {
        mBarBgTexture.setSmooth(false);
        mBarFillTexture.setSmooth(false);
        mTexturesLoaded = true;
    }
}

void LoadingScreen::show(sf::RenderWindow& window, const std::string& worldName) {
    mWorldName = worldName;
    mClock.restart(); // Start timer

    // Render Once con 0% de progreso
    window.setView(window.getDefaultView());
    window.clear(sf::Color::Black);
    draw(window, 0.0f, worldName);
    window.display();
}

void LoadingScreen::draw(sf::RenderTarget& target, float progress, const std::string& worldName) {
    // 1. Configurar textos en mayúsculas para estilo retro
    std::string upperWorld = worldName;
    std::transform(upperWorld.begin(), upperWorld.end(), upperWorld.begin(), ::toupper);
    mTitleText.setString("ENTRANDO A " + upperWorld);
    
    // Centrar Título
    sf::Vector2f targetSize = target.getView().getSize();
    sf::FloatRect titleBounds = mTitleText.getLocalBounds();
    mTitleText.setOrigin({std::round(titleBounds.size.x * 0.5f), std::round(titleBounds.size.y * 0.5f)});
    mTitleText.setPosition({std::round(targetSize.x / 2.f), std::round(targetSize.y * 0.35f)});

    // Configurar Subtítulo
    int percent = static_cast<int>(progress * 100.f);
    mSubtitleText.setString("CARGANDO... " + std::to_string(percent) + "%");
    sf::FloatRect subBounds = mSubtitleText.getLocalBounds();
    mSubtitleText.setOrigin({std::round(subBounds.size.x * 0.5f), std::round(subBounds.size.y * 0.5f)});
    mSubtitleText.setPosition({std::round(targetSize.x / 2.f), std::round(targetSize.y * 0.65f)});

    // Redimensionar fondo
    mBackground.setSize(targetSize);
    mBackground.setPosition({0.f, 0.f});

    // Dibujar fondo
    target.draw(mBackground);
    
    // Dibujar textos pixel-art
    target.draw(mTitleText);
    target.draw(mSubtitleText);
    
    // Dibujar barra de progreso
    if (mTexturesLoaded) {
        float zoom = cfg::Map::ZOOM_FACTOR;
        
        sf::Sprite bgSprite(mBarBgTexture);
        sf::Vector2u bgSize = mBarBgTexture.getSize();
        
        float barX = std::round((targetSize.x - bgSize.x * zoom) * 0.5f);
        float barY = std::round(targetSize.y * 0.48f);
        
        bgSprite.setPosition({barX, barY});
        bgSprite.setScale({zoom, zoom});
        target.draw(bgSprite);
        
        sf::Sprite fillSprite(mBarFillTexture);
        sf::Vector2u fillSize = mBarFillTexture.getSize();
        
        float ratio = std::clamp(progress, 0.0f, 1.0f);
        int sliceWidth = static_cast<int>(fillSize.x * ratio);
        
        fillSprite.setTextureRect(sf::IntRect({0, 0}, {sliceWidth, (int)fillSize.y}));
        fillSprite.setPosition({barX, barY});
        fillSprite.setScale({zoom, zoom});
        target.draw(fillSprite);
    }
}

void LoadingScreen::waitForMinTime(sf::RenderWindow& window, sf::Time minDuration) {
    while (mClock.getElapsedTime() < minDuration) {
        // Poll events to keep window responsive (prevent "Not Responding")
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
                return;
            }
        }
        
        float progress = std::min(1.f, mClock.getElapsedTime().asSeconds() / minDuration.asSeconds());
        
        window.setView(window.getDefaultView());
        window.clear(sf::Color::Black);
        draw(window, progress, mWorldName);
        window.display();
        
        sf::sleep(sf::milliseconds(10));
    }
}
