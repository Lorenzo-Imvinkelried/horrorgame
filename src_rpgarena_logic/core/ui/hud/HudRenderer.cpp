#include "Hud.h"
#include "Config.h"
#include "core/ui/UIPanel.h"
#include "core/managers/TitleManager.h"
#include "core/skills/SkillManager.h"
#include "core/systems/combat/CombatSystem.h"
#include "core/systems/InteractionSystem.h"
#include "entities/Entity.h"
#include "entities/player/Player.h"
#include <cmath>
#include <iomanip>
#include <sstream>

void Hud::draw(sf::RenderTarget &target, Player *player, Entity *targetEntity,
               ResourceManager &res, const SkillManager &skillMgr) {
  mTargetedEntity = targetEntity;
  // [RENDER] Este es el bucle principal de dibujo. El orden aquí define qué
  // tapa a qué.

  // Dibujamos el chat de fondo (Capa inferior de UI)
  mChatBox.draw(target);

  // [FPS] Lógica para medir la velocidad del juego
  if (mShowFps) {
    mFpsFrameCount++;
    if (mFpsClock.getElapsedTime().asSeconds() >= 0.2f) {
      float fps =
          (float)mFpsFrameCount / mFpsClock.getElapsedTime().asSeconds();

      // Si cambias cfg::Debug::ENABLE_PERF_CHAT, los FPS saldrán también en el
      // chat
      if (cfg::Debug::ENABLE_PERF_CHAT) {
        std::stringstream ss;
        ss << "[PERF] FPS: " << (int)std::round(fps)
           << " | Mobs: " << mCachedActiveEnts
           << " | Rnd: " << mCachedRenderedEnts
           << " | Chunks: " << mCachedActiveChunks;
        mChatBox.addLine(ss.str());
      }

      // Actualizamos el texto "ENTS: X/Y" que preguntaste antes
      if (mFpsText) {
        std::stringstream ss;
        ss << "FPS: " << (int)std::round(fps)
           << " | Ents: " << mCachedActiveEnts << "/" << mCachedRenderedEnts
           << " | Chunks: " << mCachedActiveChunks;
        mFpsText->setString(ss.str());

        sf::FloatRect bounds = mFpsText->getGlobalBounds();
        mFpsText->setPosition(
            {std::floor((mWindowSize.x - bounds.size.x) * 0.5f), 10.f});
      }

      mFpsFrameCount = 0;
      mFpsClock.restart();
    }

    if (mFpsText)
      target.draw(*mFpsText); // Dibuja el texto en pantalla
  }

  bool isStunned = (player && player->isStunned());
  drawHotbarBottomCenter(target, isStunned, player, &skillMgr);
  drawBagIconBottomRight(target);
  drawCharacterIcon(target);

  // Update Inventory Position if needed
  if (mInventoryOpen && !mInventoryPosInitialized) {
    // Use mWindowSize (Physical)
    const sf::Vector2f win((float)mWindowSize.x, (float)mWindowSize.y);
    sf::FloatRect bounds = mInventoryPanel.getBounds();
    const float panelX = win.x - UI_MARGIN - bounds.size.x;
    const float panelY =
        win.y - UI_MARGIN - bounds.size.y - (mSlotSize + 2.f * UI_MARGIN + 6.f);
    mInventoryPanel.setPosition(sf::Vector2f(panelX, panelY));
    mInventoryPosInitialized = true;
  }

  // Update Titles panel position if needed
  static bool titlesPosInitialized = false;
  if (mTitlesPanelOpen && !titlesPosInitialized) {
    const sf::Vector2f win((float)mWindowSize.x, (float)mWindowSize.y);
    sf::FloatRect bounds = mTitlesPanel.getBounds();
    mTitlesPanel.setPosition(
        sf::Vector2f(std::floor((win.x - bounds.size.x) * 0.5f),
                     std::floor((win.y - bounds.size.y) * 0.5f)));
    titlesPosInitialized = true;
  }

  // Draw all visible panels in Z-order (back to front)
  for (auto *panel : mPanels) {
    if (panel && panel->isVisible()) {
      if (panel == &mCharacterPanel) {
        mCharacterPanel.setEntity(player);
      }
      if (panel == &mInventoryPanel) {
        mInventoryPanel.setPlayer(player);
      }
      if (panel == &mTitlesPanel) {
        mTitlesPanel.setPlayer(player);
      }
      if (panel == &mSkillLevelUpPanel) {
        mSkillLevelUpPanel.setPlayer(player);
        mSkillLevelUpPanel.setSkillManager(&skillMgr);
      }
      if (panel == &mSkillDebugPanel) {
        mSkillDebugPanel.setPlayer(player);
        mSkillDebugPanel.setSkillManager(&skillMgr);
      }
      panel->draw(target, res);
    }
  }

  // [FIX] Draw Map Panel
  mMapPanel.draw(target);

  // Reset hover status effects
  mHoveredStatusEffect = nullptr;
  mHoveredStatusEffectDuration = 0.f;

  mPlayerFrame.draw(target, player);

  // Draw status effects under PlayerFrame
  if (player) {
    sf::FloatRect bounds = mPlayerFrame.getBounds();
    sf::Vector2f startPos(
        bounds.position.x +
            StatusEffectManager::getInstance().getPlayerOffsetX(),
        bounds.position.y +
            StatusEffectManager::getInstance().getPlayerOffsetY());
    float duration = 0.f;
    const StatusEffectInfo *hovered = mStatusEffectSystem.drawStatusEffects(
        target, player->getActiveStatusEffects(), startPos, mCurrentMousePos,
        res, duration);
    if (hovered) {
      mHoveredStatusEffect = hovered;
      mHoveredStatusEffectDuration = duration;
    }
  }

  // 4. Target Frame
  mTargetFrame.draw(target);

  // Draw status effects under TargetFrame
  if (targetEntity && targetEntity->isAlive()) {
    sf::FloatRect bounds = mTargetFrame.getBounds();
    sf::Vector2f startPos(
        bounds.position.x +
            StatusEffectManager::getInstance().getTargetOffsetX(),
        bounds.position.y +
            StatusEffectManager::getInstance().getTargetOffsetY());
    float duration = 0.f;
    const StatusEffectInfo *hovered = mStatusEffectSystem.drawStatusEffects(
        target, targetEntity->getActiveStatusEffects(), startPos,
        mCurrentMousePos, res, duration);
    if (hovered) {
      mHoveredStatusEffect = hovered;
      mHoveredStatusEffectDuration = duration;
    }
  }

  // Render Drag
  mDragSystem.render(target, mCurrentMousePos, res);

  // --- CAST BAR ---
  if (mCombatSystem && mCombatSystem->isCastingSkill()) {
    const Skill *skill = mCombatSystem->getPendingSkill();
    if (skill && mCastBarBgSprite && mCastBarFillSprite) {
      float zoom = cfg::Map::ZOOM_FACTOR;
      const sf::Vector2f win((float)mWindowSize.x, (float)mWindowSize.y);

      // Calculate base hotbar Y boundary
      float hotbarTopY = win.y - UI_MARGIN - mSlotSize;
      if (mHasValidHudBg && mHudBgSprite) {
        sf::FloatRect bounds = mHudBgSprite->getGlobalBounds();
        hotbarTopY =
            std::floor(win.y - bounds.size.y + cfg::UI::HUD_BG_OFFSET_Y * zoom);
      }

      sf::FloatRect bgLocal = mCastBarBgSprite->getLocalBounds();
      float bgWidth = bgLocal.size.x * zoom;
      float bgHeight = bgLocal.size.y * zoom;

      float x = std::floor((win.x - bgWidth) * 0.5f);
      float y = std::floor(hotbarTopY - bgHeight - 15.f);

      // Draw background
      mCastBarBgSprite->setPosition({x, y});
      target.draw(*mCastBarBgSprite);

      // Calculate progress and draw fill
      float progress = mCombatSystem->getCastProgress();
      if (progress < 0.f)
        progress = 0.f;
      else if (progress > 1.f)
        progress = 1.f;

      const sf::Texture &fillTex = mCastBarFillSprite->getTexture();
      sf::Vector2u texSize = fillTex.getSize();
      int sliceWidth = static_cast<int>(texSize.x * progress);
      mCastBarFillSprite->setTextureRect(
          sf::IntRect({0, 0}, {sliceWidth, (int)texSize.y}));
      mCastBarFillSprite->setPosition({x, y});
      target.draw(*mCastBarFillSprite);

      // Draw skill name above
      BitmapText skillNameText;
      skillNameText.setTexture(&mFontTexture);
      skillNameText.setScale({zoom, zoom});
      skillNameText.setString(skill->name);
      skillNameText.setColor(sf::Color::White);

      sf::FloatRect textBounds = skillNameText.getGlobalBounds();
      float textX = std::floor(x + (bgWidth - textBounds.size.x) * 0.5f);
      float textY = std::floor(y - textBounds.size.y - 6.f);
      skillNameText.setPosition({textX, textY});
      target.draw(skillNameText);
    }
  }

  // [FIX] Update tooltip visibility based on current state
  updateTooltip(mCurrentMousePos, &skillMgr);

  // Tooltip ENCIMA de todo
  mTooltip.draw(target, res);

  // Texto de status de combate (ej: mejora de items) ENCIMA de todo también
  drawCombatStatus(target);

  // Interaction cursor on top of everything
  InteractionSystem::getInstance().drawCursor(target, mCurrentMousePos, res,
                                              1.0f);
}

void Hud::drawCharacterIcon(sf::RenderTarget &target) {
  const sf::Vector2f win((float)mWindowSize.x, (float)mWindowSize.y);
  const sf::Vector2f charPos(win.x - UI_MARGIN * 2 - mSlotSize * 2,
                             win.y - UI_MARGIN - mSlotSize);
  if (mCharacterSprite) {
    mCharacterSprite->setPosition(charPos);
    target.draw(*mCharacterSprite);
    mCharacterIconBounds = mCharacterSprite->getGlobalBounds();
  } else {
    drawSlotFallback(target, charPos);
    mCharacterIconBounds =
        sf::FloatRect(charPos, sf::Vector2f(mSlotSize, mSlotSize));
  }
}

void Hud::drawHotbarBottomCenter(sf::RenderTarget &target, bool isStunned,
                                 Player *player, const SkillManager *skillMgr) {
  const sf::Vector2f win((float)mWindowSize.x, (float)mWindowSize.y);
  float zoom = cfg::Map::ZOOM_FACTOR;

  const float totalW = cfg::UI::HOTBAR_SLOTS * mSlotSize +
                       (cfg::UI::HOTBAR_SLOTS - 1) * cfg::UI::SLOT_MARGIN;

  // Attempt lazy load
  try {
    if (!mHasValidHudBg) {
      sf::Texture &tex = mRes->getTexture("assets/ui/hud_bg.png");
      mHudBgSprite.emplace(tex);
      mHasValidHudBg = true;
    }
  } catch (...) {
    mHasValidHudBg = false;
    mHudBgSprite.reset();
  }

  sf::Vector2f origin;

  if (mHasValidHudBg && mHudBgSprite) {
    mHudBgSprite->setScale({zoom, zoom});
    sf::FloatRect bounds = mHudBgSprite->getGlobalBounds();

    // Centered horizontally, bottom aligned with optional config offset
    sf::Vector2f bgPos(
        std::floor((win.x - bounds.size.x) * 0.5f),
        std::floor(win.y - bounds.size.y + cfg::UI::HUD_BG_OFFSET_Y * zoom));
    mHudBgSprite->setPosition(bgPos);
    target.draw(*mHudBgSprite);

    // Let slots sit relative to the background + an offset, centered by
    // default.
    origin = sf::Vector2f(
        std::floor((win.x - totalW) * 0.5f +
                   cfg::UI::HUD_SLOTS_OFFSET_X * zoom),
        std::floor(bgPos.y +
                   cfg::UI::HUD_SLOTS_OFFSET_Y *
                       zoom) // User can push slots down via this Y offset
    );
  } else {
    // Fallback transparent box
    origin = sf::Vector2f(std::floor((win.x - totalW) * 0.5f),
                          std::floor(win.y - UI_MARGIN - mSlotSize));
    sf::RectangleShape bg(
        sf::Vector2f(totalW + 2.f * UI_MARGIN, mSlotSize + 2.f * UI_MARGIN));
    bg.setPosition(sf::Vector2f(origin.x - UI_MARGIN, origin.y - UI_MARGIN));
    bg.setFillColor(sf::Color(0, 0, 0, 130));
    bg.setOutlineThickness(1.f);
    bg.setOutlineColor(sf::Color(40, 40, 40));
    target.draw(bg);
  }

  for (int i = 0; i < cfg::UI::HOTBAR_SLOTS; ++i) {
    const sf::Vector2f pos(origin.x + i * (mSlotSize + cfg::UI::SLOT_MARGIN),
                           origin.y);
    // Dibuja el recuadro del slot
    if (mSlotSprite) {
      mSlotSprite->setPosition(pos);
      target.draw(*mSlotSprite);
    } else {
      drawSlotFallback(target, pos);
    }

    // [HABILIDADES] Si el jugador tiene una skill puesta en este slot, la
    // dibujamos
    if (player && skillMgr) {
      int skillId = player->getEquippedSkill(i);
      if (skillId != -1) {
        const Skill *skill = skillMgr->getSkill(skillId);
        if (skill && skill->iconTexture) {
          sf::Sprite icon(*skill->iconTexture);
          icon.setTextureRect(
              sf::IntRect({skill->atlasX, skill->atlasY}, {16, 16}));
          float zoom = cfg::Map::ZOOM_FACTOR;

          // [PIXEL PERFECT] El icono se escala exactamente igual que el mundo.
          // Si el icono es 16x16, se verá de 48x48 en escala 3.0.
          icon.setScale({zoom, zoom});

          // Centrado manual (2px de margen con slot=20, icon=16)
          float margin =
              std::floor((cfg::UI::BASE_SLOT_SIZE - cfg::UI::BASE_ICON_SIZE) *
                         0.5f * zoom);

          icon.setPosition(
              {std::floor(pos.x + margin), std::floor(pos.y + margin)});
          target.draw(icon);

          float iconSizePixels = cfg::UI::BASE_ICON_SIZE * zoom;

          // [COOLDOWN] Si la habilidad se usó hace poco, dibujamos la sombra de
          // espera
          float cdCurrent = player->getSkillCooldown(skillId);
          if (cdCurrent > 0.f) {
            float cdTotal = skill->cooldown;
            float ratio = cdCurrent / cdTotal; // Proporción de tiempo restante

            // Sombra que sube/baja según el tiempo
            sf::RectangleShape cdOverlay(
                sf::Vector2f(iconSizePixels, iconSizePixels * ratio));
            cdOverlay.setPosition(
                {pos.x + margin,
                 pos.y + margin + (iconSizePixels - (iconSizePixels * ratio))});
            cdOverlay.setFillColor(sf::Color(0, 0, 0, 180));
            target.draw(cdOverlay);

            // Texto con los segundos restantes (centrado)
            BitmapText cdText;
            cdText.setTexture(&mFontTexture);
            cdText.setScale({zoom, zoom});
            std::stringstream ss;
            ss << std::fixed << std::setprecision(1)
               << cdCurrent; // Solo 1 decimal
            cdText.setString(ss.str());
            cdText.setColor(sf::Color::White);

            sf::FloatRect bounds = cdText.getLocalBounds();
            cdText.setPosition(
                {pos.x + mSlotSize / 2.f - (bounds.size.x * 2.f) / 2.f,
                 pos.y + mSlotSize / 2.f - (bounds.size.y * 2.f) / 2.f});
            target.draw(cdText);
          }
        }
      }
    }
    // [DEATH / STUN / NO MANA] Si el jugador está muerto o no tiene maná,
    // oscurecemos el slot interno de 16x16. Si está aturdido, oscurecemos toda
    // la barra.
    if (player && !player->isAlive()) {
      float margin = std::floor(
          (cfg::UI::BASE_SLOT_SIZE - cfg::UI::BASE_ICON_SIZE) * 0.5f * zoom);
      float iconSizePixels = cfg::UI::BASE_ICON_SIZE * zoom;
      sf::RectangleShape deathOverlay(
          sf::Vector2f(iconSizePixels, iconSizePixels));
      deathOverlay.setPosition({pos.x + margin, pos.y + margin});
      deathOverlay.setFillColor(sf::Color(0, 0, 0, 200));
      target.draw(deathOverlay);
    } else if (isStunned) {
      sf::RectangleShape darkOverlay(sf::Vector2f(mSlotSize, mSlotSize));
      darkOverlay.setPosition(pos);
      darkOverlay.setFillColor(sf::Color(0, 0, 0, 200));
      target.draw(darkOverlay);
    } else if (player && skillMgr) {
      int skillId = player->getEquippedSkill(i);
      if (skillId != -1) {
        const Skill *skill = skillMgr->getSkill(skillId);
        if (skill && player->getCurrentMp() < skill->manaCost) {
          float margin =
              std::floor((cfg::UI::BASE_SLOT_SIZE - cfg::UI::BASE_ICON_SIZE) *
                         0.5f * zoom);
          float iconSizePixels = cfg::UI::BASE_ICON_SIZE * zoom;
          sf::RectangleShape manaOverlay(
              sf::Vector2f(iconSizePixels, iconSizePixels));
          manaOverlay.setPosition({pos.x + margin, pos.y + margin});
          manaOverlay.setFillColor(sf::Color(0, 0, 0, 200));
          target.draw(manaOverlay);
        }
      }
    }

    // [HOTKEY LABELS] Draw labels at (3, 3) * zoom
    static const std::string labels[] = {"1", "2", "3", "4", "5", "6",
                                         "7", "8", "9", "0", "'", "¡"};
    if (i < 12) {
      BitmapText labelText;
      labelText.setTexture(&mFontTexture);
      labelText.setScale({zoom, zoom});
      labelText.setString(labels[i]);
      labelText.setColor(sf::Color::White);
      labelText.setPosition({pos.x + 3.f * zoom, pos.y + 3.f * zoom});
      target.draw(labelText);
    }
  }
}

void Hud::drawSlotFallback(sf::RenderTarget &target, sf::Vector2f pos) {
  sf::RectangleShape r(sf::Vector2f(mSlotSize, mSlotSize));
  r.setPosition(pos);
  r.setFillColor(sf::Color(30, 30, 30, 220));
  r.setOutlineThickness(1.f);
  r.setOutlineColor(sf::Color(90, 90, 90));
  target.draw(r);
}

void Hud::drawBagIconBottomRight(sf::RenderTarget &target) {
  const sf::Vector2f win((float)mWindowSize.x, (float)mWindowSize.y);
  const sf::Vector2f bagPos(win.x - UI_MARGIN - mSlotSize,
                            win.y - UI_MARGIN - mSlotSize);
  if (mBagSprite) {
    mBagSprite->setPosition(bagPos);
    target.draw(*mBagSprite);
    mBagIconBounds = mBagSprite->getGlobalBounds();
  } else {
    drawSlotFallback(target, bagPos);
    mBagIconBounds = sf::FloatRect(bagPos, sf::Vector2f(mSlotSize, mSlotSize));
  }
}
