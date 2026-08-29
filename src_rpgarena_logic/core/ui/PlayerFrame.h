//PlayerFrame.h
#pragma once
#include <SFML/Graphics.hpp>
#include <optional>
#include "../engine/ResourceManager.h"
#include "entities/player/Player.h" // Para el puntero a Player

class PlayerFrame {
public:
    // El constructor ahora necesita una referencia a la fuente que carga Hud
    PlayerFrame(sf::Texture* fontTexture);

    // Carga sus propios recursos (el retrato)
    void load(ResourceManager& res);

    // La función principal de dibujado
    // Actualizaremos el puntero interno player
    void updateRT(Player* player);
    void draw(sf::RenderTarget& target, Player* player);
    
    Player* getPlayer() const { return mPlayer; }
    sf::FloatRect getBounds() const { return mLastBounds; }

private:
    // Almacena una REFERENCIA a la fuente principal (no la carga)
    sf::Texture* mFontTexture = nullptr;
    sf::Texture* mHpTexture = nullptr; // [NEW] Green
    sf::Texture* mBgTexture = nullptr; // [NEW] Red background
    sf::Texture* mMpTexture = nullptr; // [NEW] Blue
    sf::Texture* mFrameBgTexture = nullptr; // [NEW] Full Frame UI Background
    sf::Texture* mTapsBgTexture = nullptr;  // [TAP SYSTEM] Background
    sf::Texture* mTapsFillTexture = nullptr; // [TAP SYSTEM] Fill
    Player*   mPlayer = nullptr; // Referencia al player actual
    sf::FloatRect mLastBounds;

    // [LIVE PORTRAIT] RenderTexture for real-time player portrait
    sf::RenderTexture mPortraitRT;
    bool mPortraitReady = false;

    // Recursos propios (fallback static portrait)
    std::optional<sf::Sprite> mPlayerPortrait;

    // Constantes de layout (puedes moverlas aquí)
    static constexpr float UNIFIED_SLOT_SIZE = 60.f;
    static constexpr float UI_MARGIN = 10.f;
};