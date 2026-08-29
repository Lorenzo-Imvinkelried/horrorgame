#include "TargetFrame.h"
#include "Config.h"
#include "core/graphics/BitmapText.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>

TargetFrame::TargetFrame(sf::Texture *fontTexture)
    : mFontTexture(fontTexture) {}

void TargetFrame::load(ResourceManager &res) {
  float zoom = cfg::Map::ZOOM_FACTOR;
  unsigned int pSize =
      static_cast<unsigned int>(cfg::UI::TargetFrame::PORTRAIT_SIZE / zoom);
  if (mPortraitRT.resize({pSize, pSize})) {
    mPortraitRT.setSmooth(false);
    mPortraitReady = true;
  } else {
    std::cerr << "[TargetFrame] Failed to create portrait RenderTexture, using "
                 "fallback.\n";
    mPortraitReady = false;
  }

  try {
    sf::Texture &tex = res.getTexture("assets/ui/target_portrait_bg.png");
    mPortraitBg.emplace(tex);
    const sf::FloatRect lb = mPortraitBg->getLocalBounds();
    if (lb.size.x > 0.f && lb.size.y > 0.f) {
      float zoom = cfg::Map::ZOOM_FACTOR;
      mPortraitBg->setScale({zoom, zoom});
    }
  } catch (...) {
    mPortraitBg.reset();
  }

  try {
    mHpGreenTexture =
        &res.getTexture("assets/ui/player_frame_healthbar_green.png");
    mHpRedTexture = &res.getTexture("assets/ui/player_frame_healthbar_red.png");
    mMpBlueTexture =
        &res.getTexture("assets/ui/player_frame_healthbar_blue.png");
    mFrameBgTexture = &res.getTexture(
        "assets/ui/target_frame_bg.png"); // [NEW] Carga manual del fondo UI
    try {
      mBossFrameBgTexture = &res.getTexture("assets/ui/boss_target_frame.png");
    } catch (...) {
      mBossFrameBgTexture = nullptr;
    }
  } catch (...) {
  }
}

void TargetFrame::drawBarWithText(sf::RenderTarget &target, sf::Vector2f pos,
                                  sf::Vector2f size, float fill01, sf::Color bg,
                                  sf::Color fg, const sf::String &labelLeft,
                                  const sf::String &valueCenter,
                                  sf::Texture *fillTex, sf::Texture *bgTex) {
  float zoom = cfg::Map::ZOOM_FACTOR;

  // fondo
  if (bgTex) {
    sf::Sprite bgSprite(*bgTex);
    bgSprite.setPosition(pos);
    bgSprite.setScale({zoom, zoom});
    target.draw(bgSprite);
  } else {
    sf::RectangleShape bgRect(size);
    bgRect.setPosition(pos);
    bgRect.setFillColor(bg);
    target.draw(bgRect);
  }

  // relleno
  float w = std::clamp(fill01, 0.f, 1.f) * size.x;

  if (fillTex && w > 0.f) {
    sf::Sprite fillSprite(*fillTex);
    const sf::Vector2u texSize = fillTex->getSize();
    int fillW = static_cast<int>(texSize.x * fill01);
    fillSprite.setTextureRect(sf::IntRect({0, 0}, {fillW, (int)texSize.y}));
    fillSprite.setPosition(pos);
    fillSprite.setScale({zoom, zoom});
    target.draw(fillSprite);
  } else if (w > 0.f) {
    sf::RectangleShape fillRect({w, size.y});
    fillRect.setPosition(pos);
    fillRect.setFillColor(fg);
    target.draw(fillRect);
  }

  // textos
  BitmapText txt;
  txt.setTexture(mFontTexture);

  // etiqueta izquierda (HP / MP)
  txt.setString(labelLeft);
  const float fScale = cfg::UI::FONT_SCALE;
  txt.setScale({fScale, fScale});
  txt.setColor(sf::Color::White);
  txt.setPosition({std::floor(pos.x + cfg::UI::TargetFrame::LABEL_OFFSET_X),
                   std::floor(pos.y - cfg::UI::TargetFrame::LABEL_OFFSET_Y)});
  target.draw(txt);

  // valor centrado (cur/max)
  txt.setString(valueCenter);
  txt.setScale({fScale, fScale});
  const sf::FloatRect tb = txt.getLocalBounds();
  const float cx =
      std::floor(pos.x + size.x * 0.5f - tb.size.x * fScale * 0.5f);
  const float cy =
      std::floor(pos.y + size.y * 0.5f - tb.size.y * fScale * 0.5f);
  txt.setPosition({cx, cy});
  target.draw(txt);
}

void TargetFrame::setTarget(Entity *target) {
  if (mTarget == target)
    return;

  if (mTarget && mObserverId != -1) {
    mTarget->removeStatsObserver(mObserverId);
    mObserverId = -1;
  }

  mTarget = target;

  if (mTarget) {
    mObserverId = mTarget->addStatsObserver([this]() { this->updateTexts(); });
    updateTexts();
  } else {
    mLastBounds = {};
  }
}

void TargetFrame::notifyEntityDeath(Entity *entity) {
  if (mTarget == entity) {
    setTarget(nullptr);
  }
}

void TargetFrame::updateTexts() {
  if (!mTarget)
    return;

  mNameStr = mTarget->getName();

  mCachedCurHp = mTarget->getCurrentHp();
  mCachedMaxHp = std::max(1, mTarget->getMaxHp());
  mCachedHpPct =
      static_cast<float>(mCachedCurHp) / static_cast<float>(mCachedMaxHp);

  std::ostringstream ossHp;
  ossHp << mCachedCurHp << "/" << mCachedMaxHp;
  mHpStr = ossHp.str();

  mCachedCurMp = mTarget->getCurrentMp();
  mCachedMaxMp = std::max(1, mTarget->getMaxMp());
  mCachedMpPct =
      static_cast<float>(mCachedCurMp) / static_cast<float>(mCachedMaxMp);

  std::ostringstream ossMp;
  ossMp << mCachedCurMp << "/" << mCachedMaxMp;
  mMpStr = ossMp.str();
}

void TargetFrame::updateRT() {
  if (!mTarget || !mTarget->isAlive())
    return;

  if (mPortraitReady) {
    mPortraitRT.clear(sf::Color::Transparent);

    sf::Vector2f tPos = mTarget->getPosition();
    float zoom = cfg::Map::ZOOM_FACTOR;
    float viewSize =
        cfg::UI::TargetFrame::PORTRAIT_SIZE / zoom; // Use exact virtual size
    // Offset para centrar la cámara del retrato (configurable desde Config.h)
    float offsetY = cfg::UI::TargetFrame::PORTRAIT_VIEW_OFFSET_Y;

    // Remove rounding from view center to keep the character perfectly centered
    // relative to the camera viewport, preventing sub-pixel/rounding jitter.
    sf::View portraitView({tPos.x, tPos.y + offsetY},
                          {viewSize, viewSize});
    mPortraitRT.setView(portraitView);

    mTarget->draw(mPortraitRT);

    mPortraitRT.display();
  }
}

void TargetFrame::draw(sf::RenderTarget &target) {
  if (!mTarget || !mTarget->isAlive()) {
    mLastBounds = {};
    return;
  }

  const sf::Vector2f winSize = target.getView().getSize();
  const float pad = 8.f;
  const float barW = cfg::UI::TargetFrame::BAR_WIDTH;
  const float barH = cfg::UI::TargetFrame::BAR_HEIGHT;
  float zoom = cfg::Map::ZOOM_FACTOR;

  float currentBarW = barW;
  float currentBarH = barH;
  if (mHpGreenTexture) {
    currentBarW = static_cast<float>(mHpGreenTexture->getSize().x) * zoom;
    currentBarH = static_cast<float>(mHpGreenTexture->getSize().y) * zoom;
  }

  const float barsTotalH =
      cfg::UI::TargetFrame::NAME_OFFSET_Y + 2.f * currentBarH + 8.f + 2.f * pad;

  const float frameW =
      cfg::UI::TargetFrame::PORTRAIT_SIZE + currentBarW + pad * 3;
  const float frameH =
      std::max(cfg::UI::TargetFrame::PORTRAIT_SIZE + pad * 2.f, barsTotalH);

  const float frameX = std::floor((winSize.x - frameW) * 0.5f);
  const float frameY = std::floor(cfg::UI::TargetFrame::MARGIN);

  sf::Texture* bgTexToUse = mFrameBgTexture;
  if (mTarget && mTarget->isBoss() && mBossFrameBgTexture) {
    bgTexToUse = mBossFrameBgTexture;
  }

  if (bgTexToUse) {
    sf::Sprite bgSprite(*bgTexToUse);
    bgSprite.setPosition({frameX, frameY});
    bgSprite.setScale(
        {zoom,
         zoom}); // [RESTORED] Escala 1x para que coincida con el pixel art
    target.draw(bgSprite);
    mLastBounds = bgSprite.getGlobalBounds();
  } else {
    mLastBounds = sf::FloatRect({frameX, frameY}, {frameW, frameH});
  }

  // Posición original de la interfaz (sin multiplicar por zoom, ya que los
  // offsets parecen estar en espacio de pantalla)
  const sf::Vector2f portraitPos{
      frameX + cfg::UI::TargetFrame::PORTRAIT_OFFSET_X,
      frameY + cfg::UI::TargetFrame::PORTRAIT_OFFSET_Y};
  if (mTarget && mTarget->isAlive()) {
    // 1. Guardar la vista HUD actual
    sf::View oldView = target.getView();

    // 2. Calcular las coordenadas en píxeles de la ventana física para el Viewport
    float portSize = cfg::UI::TargetFrame::PORTRAIT_SIZE;
    sf::Vector2i pixelMin = target.mapCoordsToPixel(portraitPos, oldView);
    sf::Vector2i pixelMax = target.mapCoordsToPixel(portraitPos + sf::Vector2f(portSize, portSize), oldView);
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

    // 4. Configurar la cámara del retrato centrada en el objetivo
    sf::Vector2f tPos = mTarget->getPosition();
    float viewSize = cfg::UI::TargetFrame::PORTRAIT_SIZE / zoom;
    float offsetY = cfg::UI::TargetFrame::PORTRAIT_VIEW_OFFSET_Y;

    sf::View portraitView({tPos.x, tPos.y + offsetY}, {viewSize, viewSize});
    portraitView.setViewport(viewport);

    // 5. Dibujar el objetivo directamente a la pantalla con la nueva vista
    target.setView(portraitView);
    mTarget->draw(target);

    // 6. Restaurar la vista HUD original
    target.setView(oldView);
  } else if (mPortraitBg) {
    mPortraitBg->setPosition(portraitPos);
    target.draw(*mPortraitBg);
  } else {
    sf::RectangleShape r({cfg::UI::TargetFrame::PORTRAIT_SIZE,
                          cfg::UI::TargetFrame::PORTRAIT_SIZE});
    r.setPosition(portraitPos);
    r.setFillColor(sf::Color(255, 0, 255, 100));
    target.draw(r);
  }

  // --- 8. Nivel ---
  std::string lvlStr = "Lvl: " + std::to_string(mTarget->getLevel());
  BitmapText lvlTxt;
  lvlTxt.setTexture(mFontTexture);
  lvlTxt.setString(lvlStr);
  const float fScale = cfg::UI::FONT_SCALE;
  lvlTxt.setScale({fScale, fScale});
  const sf::FloatRect lvlTb = lvlTxt.getLocalBounds();
  lvlTxt.setOrigin({lvlTb.size.x / 2.f, 0.f});
  lvlTxt.setPosition(
      {std::floor(portraitPos.x + cfg::UI::TargetFrame::PORTRAIT_SIZE / 2.f),
       std::floor(portraitPos.y + cfg::UI::TargetFrame::PORTRAIT_SIZE +
                  4.f * zoom)});
  lvlTxt.setColor(sf::Color::White);
  target.draw(lvlTxt);

  const sf::Vector2f textBlock{
      frameX + cfg::UI::TargetFrame::TEXT_BLOCK_OFFSET_X,
      frameY + cfg::UI::TargetFrame::TEXT_BLOCK_OFFSET_Y};

  BitmapText nameTxt;
  nameTxt.setTexture(mFontTexture);
  nameTxt.setScale({cfg::UI::FONT_SCALE, cfg::UI::FONT_SCALE});
  nameTxt.setString(mNameStr);
  nameTxt.setColor(sf::Color::White);
  nameTxt.setPosition(textBlock);
  target.draw(nameTxt);

  const float hpBarX = frameX + cfg::UI::TargetFrame::HP_BAR_X * zoom;
  const float hpBarY = frameY + cfg::UI::TargetFrame::HP_BAR_Y * zoom;
  const float mpBarX = frameX + cfg::UI::TargetFrame::MP_BAR_X * zoom;
  const float mpBarY = frameY + cfg::UI::TargetFrame::MP_BAR_Y * zoom;

  drawBarWithText(target, {hpBarX, hpBarY}, {currentBarW, currentBarH},
                  mCachedHpPct, sf::Color(20, 20, 20, 200),
                  sf::Color(220, 50, 50), "", mHpStr, mHpGreenTexture,
                  mHpRedTexture);

  drawBarWithText(target, {mpBarX, mpBarY}, {currentBarW, currentBarH},
                  mCachedMpPct, sf::Color(20, 20, 20, 200),
                  sf::Color(50, 50, 220), "", mMpStr, mMpBlueTexture,
                  mHpRedTexture);
}
