//Playerframe.cpp
#include "PlayerFrame.h"
#include <sstream>
#include <iomanip>
#include <iostream> // Para los std::cerr
#include "core/graphics/BitmapText.h"

// El constructor recibe la fuente de Hud.h y la guarda
PlayerFrame::PlayerFrame(sf::Texture* fontTexture) 
    : mFontTexture(fontTexture)
{
}

void PlayerFrame::load(ResourceManager& res) {
    // [LIVE PORTRAIT] Create RenderTexture for real-time player bust
    // Use the exact portrait size divided by zoom to keep a 1:1 virtual pixel ratio
    float zoom = cfg::Map::ZOOM_FACTOR;
    unsigned int pSize = static_cast<unsigned int>(cfg::UI::PlayerFrame::PORTRAIT_SIZE / zoom);
    if (mPortraitRT.resize({pSize, pSize})) {
        mPortraitRT.setSmooth(false); // Pixel art look
        mPortraitReady = true;
    } else {
        std::cerr << "[PlayerFrame] Failed to create portrait RenderTexture, using fallback.\n";
        mPortraitReady = false;
    }

    // Fallback: Load static portrait in case RenderTexture fails
    try {
        sf::Texture& tex = res.getTexture("assets/ui/player_portrait.png");
        mPlayerPortrait.emplace(tex);
        const sf::FloatRect lb = mPlayerPortrait->getLocalBounds();
        if (lb.size.x > 0.f && lb.size.y > 0.f) {
            float zoom = cfg::Map::ZOOM_FACTOR;
            mPlayerPortrait->setScale({zoom, zoom});
        }
    } catch (...) {
        mPlayerPortrait.reset(); 
    }

    try {
        mHpTexture = &res.getTexture("assets/ui/player_frame_healthbar_green.png");
        mBgTexture = &res.getTexture("assets/ui/player_frame_healthbar_red.png");
        mMpTexture = &res.getTexture("assets/ui/player_frame_healthbar_blue.png");
        mFrameBgTexture = &res.getTexture("assets/ui/player_frame_bg.png"); // [NEW] Carga manual del fondo UI
        mTapsBgTexture = &res.getTexture("assets/ui/taps_ui/taps_ui_bg.png");
        mTapsFillTexture = &res.getTexture("assets/ui/taps_ui/taps_ui_fill.png");
    } catch (...) {}
}

void PlayerFrame::updateRT(Player* player) {
    if (!player) return;
    mPlayer = player;
    
    if (mPortraitReady) {
        mPortraitRT.clear(sf::Color::Transparent);
        
        if (player->isAlive()) {
            sf::Vector2f pPos = player->getPosition();
            float zoom = cfg::Map::ZOOM_FACTOR;
            float viewSize = cfg::UI::PlayerFrame::PORTRAIT_SIZE / zoom; // Use exact virtual size
            // Offset para centrar la cámara del retrato (configurable desde Config.h)
            float offsetY = cfg::UI::PlayerFrame::PORTRAIT_VIEW_OFFSET_Y; 
            
            // Round the view center to prevent sub-pixel deformation of rotated weapons
            sf::View portraitView(
                {pPos.x, pPos.y + offsetY},
                {viewSize, viewSize}
            );
            mPortraitRT.setView(portraitView);
            
            player->getSkin().draw(mPortraitRT);
        }
        
        mPortraitRT.display();
    }
}

// Esta es la función que movimos de Hud.cpp
void PlayerFrame::draw(sf::RenderTarget& target, Player* player) {
    if (!player || !mFontTexture) return;
    mPlayer = player; // UPDATE MEMBER POINTER!

    // --- 1. Layout y Posiciones ---
    const float zoom = cfg::Map::ZOOM_FACTOR;
    const float frameX = UI_MARGIN;
    const float frameY = UI_MARGIN;
    const float portraitSize = cfg::UI::PlayerFrame::PORTRAIT_SIZE;
    const float padding = cfg::UI::PlayerFrame::PORTRAIT_OFFSET_X; // Was hardcoded 8.f
    
    // [DYNAMIC] Usar el tamaño de la textura si existe, si no, usar config
    float barWidth = cfg::UI::PlayerFrame::BAR_WIDTH;
    float barHeight = cfg::UI::PlayerFrame::BAR_HEIGHT;
    
    if (mBgTexture) {
        barWidth = static_cast<float>(mBgTexture->getSize().x);
        barHeight = static_cast<float>(mBgTexture->getSize().y);
    }

    const float frameW = portraitSize + (barWidth * zoom) + padding * 3;
    const float frameH = portraitSize + padding * 2 + 20.f; 

    // --- 2. Fondo (MANUAL UI SPRITE) ---
    if (mFrameBgTexture) {
        sf::Sprite bgSprite(*mFrameBgTexture);
        bgSprite.setPosition({ frameX, frameY });
        bgSprite.setScale({ zoom, zoom }); // [RESTORED] Escala 1x para que coincida con el pixel art del juego
        target.draw(bgSprite);
        mLastBounds = bgSprite.getGlobalBounds();
    } else {
        mLastBounds = sf::FloatRect({frameX, frameY}, {frameW, frameH});
    }

    // --- 3. Retrato (LIVE PORTRAIT) ---
    // Posición original de la interfaz (sin multiplicar por zoom, ya que los offsets parecen estar en espacio de pantalla)
    const sf::Vector2f portraitPos = { frameX + cfg::UI::PlayerFrame::PORTRAIT_OFFSET_X, frameY + cfg::UI::PlayerFrame::PORTRAIT_OFFSET_Y };
    
    if (player->isAlive()) {
        // 1. Guardar la vista HUD actual
        sf::View oldView = target.getView();

        // 2. Calcular las coordenadas en píxeles de la ventana física para el Viewport
        sf::Vector2i pixelMin = target.mapCoordsToPixel(portraitPos, oldView);
        sf::Vector2i pixelMax = target.mapCoordsToPixel(portraitPos + sf::Vector2f(portraitSize, portraitSize), oldView);
        sf::Vector2i pixelSize = pixelMax - pixelMin;

        sf::Vector2u targetSize = target.getSize();

        // 3. Crear el viewport normalizado
        sf::FloatRect viewport({0.f, 0.f}, {1.f, 1.f});
        if (targetSize.x > 0 && targetSize.y > 0) {
            viewport = sf::FloatRect(
                {static_cast<float>(pixelMin.x) / targetSize.x,
                 static_cast<float>(pixelMin.y) / targetSize.y},
                {static_cast<float>(pixelSize.x) / targetSize.x,
                 static_cast<float>(pixelSize.y) / targetSize.y}
            );
        }

        // 4. Configurar la cámara del retrato centrada en el jugador
        sf::Vector2f pPos = player->getPosition();
        float viewSize = cfg::UI::PlayerFrame::PORTRAIT_SIZE / zoom;
        float offsetY = cfg::UI::PlayerFrame::PORTRAIT_VIEW_OFFSET_Y;

        sf::View portraitView({pPos.x, pPos.y + offsetY}, {viewSize, viewSize});
        portraitView.setViewport(viewport);

        // 5. Dibujar el jugador directamente a la pantalla con la nueva vista
        target.setView(portraitView);
        player->getSkin().draw(target);

        // 6. Restaurar la vista HUD original
        target.setView(oldView);
    } else if (mPlayerPortrait) {
        // Fallback: Static portrait
        mPlayerPortrait->setPosition(portraitPos);
        target.draw(*mPlayerPortrait);
    } else {
        sf::RectangleShape r({portraitSize, portraitSize});
        r.setPosition(portraitPos);
        r.setFillColor(sf::Color(20,20,20));
        target.draw(r);
    }

    // --- 4. Preparar Texto (Nombre, HP/MP, Nivel) ---
    BitmapText txt;
    txt.setTexture(mFontTexture);
    txt.setScale({zoom, zoom});
    
    const float textBlockX = frameX + cfg::UI::PlayerFrame::TEXT_BLOCK_OFFSET_X;
    const float textBlockY = frameY + cfg::UI::PlayerFrame::TEXT_BLOCK_OFFSET_Y;

    // --- 5. Nombre del Jugador ---
    txt.setString("Player"); 
    txt.setColor(sf::Color::White);
    txt.setPosition({textBlockX, textBlockY});
    target.draw(txt);

    // --- 6. Barra de HP ---
    const float hpBarX = frameX + cfg::UI::PlayerFrame::HP_BAR_X * zoom;
    const float hpBarY = frameY + cfg::UI::PlayerFrame::HP_BAR_Y * zoom;
    float hpPercent = 0.f;
    if (player->getMaxHp() > 0) { 
        hpPercent = (float)player->getCurrentHp() / (float)player->getMaxHp();
    }
    
    // 6a. Background (RED TEXTURE)
    if (mBgTexture) {
        sf::Sprite bgSprite(*mBgTexture);
        bgSprite.setPosition({ hpBarX, hpBarY });
        bgSprite.setScale({ zoom, zoom });
        target.draw(bgSprite);
    } else {
        sf::RectangleShape hpBg({ barWidth, barHeight });
        hpBg.setPosition({ hpBarX, hpBarY });
        hpBg.setFillColor(sf::Color(80, 20, 20)); 
        target.draw(hpBg);
    }

    // 6b. Fill (GREEN TEXTURE - HP% Width)
    if (mHpTexture) {
        sf::Sprite hpSprite(*mHpTexture);
        const sf::Vector2u texSize = mHpTexture->getSize();
        int fillWidth = static_cast<int>(texSize.x * hpPercent);
        hpSprite.setTextureRect(sf::IntRect({0, 0}, {fillWidth, (int)texSize.y}));
        hpSprite.setPosition({ hpBarX, hpBarY });
        hpSprite.setScale({ zoom, zoom });
        target.draw(hpSprite);
    } else {
        sf::RectangleShape hpFill({ barWidth * hpPercent, barHeight });
        hpFill.setPosition({ hpBarX, hpBarY });
        hpFill.setFillColor(sf::Color(220, 50, 50)); 
        target.draw(hpFill);
    }

    // Texto de HP
    std::string hpStr = std::to_string(player->getCurrentHp()) + "/" + std::to_string(player->getMaxHp());
    txt.setString(hpStr);
    const float fScale = cfg::UI::FONT_SCALE;
    txt.setScale({fScale, fScale});
    
    // Recalcular centrado con la nueva escala
    const sf::FloatRect textBounds = txt.getLocalBounds();
    float currentBarWidth = (mBgTexture ? mBgTexture->getSize().x : barWidth) * zoom;
    float currentBarHeight = (mBgTexture ? mBgTexture->getSize().y : barHeight) * zoom;
    
    float textX = std::floor(hpBarX + (currentBarWidth - textBounds.size.x * fScale) * 0.5f);
    float textY = std::floor(hpBarY + (currentBarHeight - textBounds.size.y * fScale) * 0.5f);
    txt.setPosition({textX, textY});
    txt.setColor(sf::Color::White);
    target.draw(txt);

    // --- 7. Barra de MP ---
    const float mpBarX = frameX + cfg::UI::PlayerFrame::MP_BAR_X * zoom;
    const float mpBarY = frameY + cfg::UI::PlayerFrame::MP_BAR_Y * zoom;
    float mpPercent = 0.f;
    if (player->getMaxMp() > 0) {
         mpPercent = (float)player->getCurrentMp() / (float)player->getMaxMp();
    }

    // 7a. Background (RED TEXTURE)
    if (mBgTexture) {
        sf::Sprite bgSprite(*mBgTexture);
        bgSprite.setPosition({ mpBarX, mpBarY });
        bgSprite.setScale({ zoom, zoom });
        target.draw(bgSprite);
    } else {
        sf::RectangleShape mpBg({ currentBarWidth, currentBarHeight });
        mpBg.setPosition({ mpBarX, mpBarY });
        mpBg.setFillColor(sf::Color(20, 20, 80)); 
        target.draw(mpBg);
    }

    // 7b. Fill (BLUE TEXTURE)
    if (mMpTexture) {
        sf::Sprite mpSprite(*mMpTexture);
        const sf::Vector2u texSize = mMpTexture->getSize();
        int fillWidth = static_cast<int>(texSize.x * mpPercent);
        mpSprite.setTextureRect(sf::IntRect({0, 0}, {fillWidth, (int)texSize.y}));
        mpSprite.setPosition({ mpBarX, mpBarY });
        mpSprite.setScale({ zoom, zoom });
        target.draw(mpSprite);
    } else {
        sf::RectangleShape mpFill({ currentBarWidth * mpPercent, currentBarHeight });
        mpFill.setPosition({ mpBarX, mpBarY });
        mpFill.setFillColor(sf::Color(50, 50, 220)); 
        target.draw(mpFill);
    }

    // Texto de MP
    std::string mpStr = std::to_string(player->getCurrentMp()) + "/" + std::to_string(player->getMaxMp());
    txt.setString(mpStr);
    txt.setScale({fScale, fScale});
    const sf::FloatRect mpTb = txt.getLocalBounds();
    const float mpcx = std::floor(mpBarX + (currentBarWidth - mpTb.size.x * fScale) * 0.5f);
    const float mpcy = std::floor(mpBarY + (currentBarHeight - mpTb.size.y * fScale) * 0.5f);
    txt.setPosition({mpcx, mpcy});
    target.draw(txt);

    // --- 8. Nivel ---
    std::string lvlStr = "Lvl: " + std::to_string(player->getLevel());
    txt.setString(lvlStr);
    txt.setScale({fScale, fScale});
    const sf::FloatRect lvlTb = txt.getLocalBounds();
    txt.setOrigin({lvlTb.size.x / 2.f, 0.f});
    txt.setPosition({
        std::floor(portraitPos.x + portraitSize / 2.f),
        std::floor(portraitPos.y + portraitSize + 4.f * zoom)
    });
    target.draw(txt);

    // --- 9. TAP SYSTEM (CARGAS Y CONTADOR DE COMBOS AT (38, 31)) ---
    if (mTapsBgTexture) {
        const TapSystem& taps = player->getTapSystem();
        const float tapsX = std::floor(frameX + taps.getUiOffsetX() * zoom);
        const float tapsY = std::floor(frameY + taps.getUiOffsetY() * zoom);

        sf::Sprite tapsBgSprite(*mTapsBgTexture);
        tapsBgSprite.setPosition({ tapsX, tapsY });
        tapsBgSprite.setScale({ zoom, zoom });
        target.draw(tapsBgSprite);

        if (mTapsFillTexture) {
            // Row 1: Charges (Fila Superior) - Render segment blocks
            const auto& topRects = taps.getTopRowRects();
            int charges = taps.getCharges();

            for (int i = 0; i < charges && i < static_cast<int>(topRects.size()); ++i) {
                const auto& r = topRects[i];
                sf::Sprite fillSegment(*mTapsFillTexture);
                fillSegment.setTextureRect(sf::IntRect({r.left, r.top}, {r.width, r.height}));
                fillSegment.setPosition({ tapsX + (r.left * zoom), tapsY + (r.top * zoom) });
                fillSegment.setScale({ zoom, zoom });
                target.draw(fillSegment);
            }

            // Row 2: Combo / Hit Counter (Fila Inferior) - Render segment blocks
            const auto& bottomRects = taps.getBottomRowRects();
            int hits = taps.getHitCounter();
            int numBottomBlocks = static_cast<int>(bottomRects.size());

            // 1st hit -> Block 1, 2nd hit -> Block 2, 3rd hit -> Block 3 (All 3 full!), 4th hit -> Bonus damage & reset
            int blocksToFill = std::clamp(hits, 0, numBottomBlocks);

            for (int j = 0; j < blocksToFill && j < numBottomBlocks; ++j) {
                const auto& r = bottomRects[j];
                sf::Sprite fillSegment(*mTapsFillTexture);
                fillSegment.setTextureRect(sf::IntRect({r.left, r.top}, {r.width, r.height}));
                fillSegment.setPosition({ tapsX + (r.left * zoom), tapsY + (r.top * zoom) });
                fillSegment.setScale({ zoom, zoom });
                target.draw(fillSegment);
            }
        }
    }
}