#include "Hud.h"
#include "Config.h"
#include "core/ui/UIPanel.h"
#include "core/items/ItemManager.h"
#include "core/systems/CommandManager.h"
#include "core/systems/FortifySystem.h"
#include "entities/Entity.h"
#include "entities/player/Player.h"
#include "utils/ConsoleRedirector.h"
#include <cmath>
#include <iostream>

Hud::Hud()
    : mPlayerFrame(&mFontTexture), mCharacterPanel(&mFontTexture, mSlotSprite),
      mInventoryPanel(&mFontTexture, mSlotSprite),
      mFortifyPanel(&mFontTexture, mSlotSprite),        // [NEW]
      mItemDebugPanel(&mFontTexture, &mInventoryPanel), // [NEW]
      mTitlesPanel(&mFontTexture),                      // [NEW]
      mCultivoPanel(&mFontTexture, mSlotSprite),        // [CULTIVO SYSTEM]
      mSkillLevelUpPanel(&mFontTexture),               // [NEW]
      mSkillDebugPanel(&mFontTexture),                 // [NEW DEBUG]
      mTargetFrame(&mFontTexture), mTooltip(&mFontTexture) {
  mPanels = {&mInventoryPanel, &mCharacterPanel, &mFortifyPanel,
             &mItemDebugPanel, &mTitlesPanel,    &mCultivoPanel, &mSkillLevelUpPanel, &mSkillDebugPanel};

  mCharacterPanel.setOnCloseCallback([this]() {
    this->toggleCharacterPanel(); // Permite que el panel se cierre a sí mismo
  });

  mInventoryPanel.setOnCloseCallback([this]() { this->toggleInventory(); });

  mFortifyPanel.setOnCloseCallback([this]() { this->toggleFortify(); });

  mTitlesPanel.setOnCloseCallback([this]() { this->toggleTitlesPanel(); });

  mCultivoPanel.setOnCloseCallback([this]() { this->toggleCultivoPanel(); });
  mSkillLevelUpPanel.setOnCloseCallback([this]() { this->toggleSkillLevelUp(); });
  mSkillDebugPanel.setOnCloseCallback([this]() { this->toggleSkillDebug(); });
  mCultivoPanel.setOnConfirmCallback([this]() {
    Player *p = mPlayerFrame.getPlayer();
    if (p) {
      p->recalculateStats();
    }
  });

  mFortifyPanel.setOnFortifyCallback([this](Item &item) {
    std::cout << "[Fortify] Fortifying " << item.name << "...\n";
    FortifySystem::fortify(item);
    this->setCombatStatus(item.name + " MEJORADA", sf::Color::Yellow);
  });
}

Hud::~Hud() = default;

void Hud::updateLayout(float width, float height) {
  // [PIXEL PERFECT] El tamaño de los slots ya no depende de un porcentaje de
  // pantalla. Ahora es: TamañoBase (ej 18px) * ZoomDelMundo (ej 3.0).
  float zoom = cfg::Map::ZOOM_FACTOR;
  mSlotSize = cfg::UI::BASE_SLOT_SIZE * zoom;

  // Guarda el tamaño de la ventana para cálculos de posición (esquinas,
  // centro).
  mWindowSize = sf::Vector2u((unsigned int)width, (unsigned int)height);

  if (mSlotSprite) {
    // [PIXEL PERFECT] Forzamos la escala a ser EXACTAMENTE la del mundo.
    mSlotSprite->setScale(sf::Vector2f(zoom, zoom));
  }
  if (mBagSprite) {
    mBagSprite->setScale(sf::Vector2f(zoom, zoom));
  }
  if (mCharacterSprite) {
    mCharacterSprite->setScale(sf::Vector2f(zoom, zoom));
  }
  if (mCastBarBgSprite) {
    mCastBarBgSprite->setScale(sf::Vector2f(zoom, zoom));
  }
  if (mCastBarFillSprite) {
    mCastBarFillSprite->setScale(sf::Vector2f(zoom, zoom));
  }

  // [CHAT] POSITION
  if (width > 500) {
    float zoom = cfg::Map::ZOOM_FACTOR;
    float chatW = mChatBox.hasBgSprite() ? (mChatBox.getBgSpriteSize().x * zoom)
                                         : cfg::UI::Chat::FALLBACK_WIDTH;
    float chatH = mChatBox.hasBgSprite() ? (mChatBox.getBgSpriteSize().y * zoom)
                                         : cfg::UI::Chat::FALLBACK_HEIGHT;
    float chatX = cfg::UI::Chat::MARGIN_LEFT;
    float chatY = height - chatH - cfg::UI::Chat::MARGIN_BOTTOM;

    mChatBox.setSize({chatW, chatH});
    mChatBox.setPosition({chatX, chatY});
  }
}

void Hud::load(ResourceManager &res) {
  mRes = &res; // Guardamos el manager para cargar ítems después

  // --- CARGA DE TEXTURAS ---
  try {
    sf::Texture &tex = res.getTexture("assets/ui/slot.png");
    mSlotSprite.emplace(tex); // Imagen del fondo de cada ítem
    mSlotSprite->setScale(
        sf::Vector2f(cfg::Map::ZOOM_FACTOR, cfg::Map::ZOOM_FACTOR));
  } catch (...) {
    mSlotSprite.reset();
  }

  try {
    sf::Texture &tex = res.getTexture("assets/ui/bag_icon.png");
    mBagSprite.emplace(tex); // Icono de la mochila (abajo derecha)
    const auto lb = mBagSprite->getLocalBounds();
    if (lb.size.x > 0.f && lb.size.y > 0.f)
      mBagSprite->setScale(
          sf::Vector2f(mSlotSize / lb.size.x, mSlotSize / lb.size.y));
  } catch (...) {
    mBagSprite.reset();
  }

  try {
    sf::Texture &tex = res.getTexture("assets/ui/character_icon.png");
    mCharacterSprite.emplace(tex); // Icono del casco/personaje
    mCharacterSprite->setScale(
        sf::Vector2f(cfg::Map::ZOOM_FACTOR, cfg::Map::ZOOM_FACTOR));
  } catch (...) {
    mCharacterSprite.reset();
  }

  // [BITMAP FONT] Cargamos la fuente principal de todo el juego.
  if (!mFontTexture.loadFromFile("assets/fonts/font.png")) {
    std::cerr << "[Hud] ERROR: No se encontró font.png\n";
  }
  mFontTexture.setSmooth(false); // Pixel-art nítido

  // Inicializamos marcos de vida/maná de jugador y objetivo
  mPlayerFrame.load(res);
  mTargetFrame.load(res);
  mItemDebugPanel.load(res); // [NEW]
  mMapPanel.load(res);       // [NEW] Map Panel

  // Inicializamos el Chat
  mChatBox.init(&mFontTexture);
  try {
    sf::Texture &chatTex = res.getTexture("assets/ui/chat_bg.png");
    mChatBox.setBgTexture(&chatTex);
  } catch (...) {
  }

  // [FIX] Inicializamos el texto de combate combat status
  mCombatStatusText.setTexture(&mFontTexture);
  mCombatStatusText.setColor(sf::Color::Red);
  mCombatStatusText.setScale({cfg::UI::FONT_SCALE, cfg::UI::FONT_SCALE});

  // Contador de FPS (amarillo y grande)
  mFpsText.emplace();
  mFpsText->setTexture(&mFontTexture);
  mFpsText->setColor(sf::Color::Yellow);
  mFpsText->setScale({cfg::UI::FONT_SCALE, cfg::UI::FONT_SCALE});
  // Redireccionamos la consola (std::cout) al chat del juego para ver logs
  mConsoleRedirector =
      std::make_unique<ConsoleRedirector>(std::cout, &mChatBox);
  std::cout << "[ChatBox] Sistema en español inicializado.\n";

  mCommandManager = std::make_unique<CommandManager>();

  try {
    sf::Texture &tex = res.getTexture("assets/ui/cast_bar_bg.png");
    mCastBarBgSprite.emplace(tex);
    mCastBarBgSprite->setScale(
        sf::Vector2f(cfg::Map::ZOOM_FACTOR, cfg::Map::ZOOM_FACTOR));
  } catch (...) {
    mCastBarBgSprite.reset();
  }

  try {
    sf::Texture &tex = res.getTexture("assets/ui/cast_bar_fill.png");
    mCastBarFillSprite.emplace(tex);
    mCastBarFillSprite->setScale(
        sf::Vector2f(cfg::Map::ZOOM_FACTOR, cfg::Map::ZOOM_FACTOR));
  } catch (...) {
    mCastBarFillSprite.reset();
  }
}

void Hud::setItemManager(ItemManager *itemMgr) {
  if (mItemMgr == itemMgr)
    return;
  mItemMgr = itemMgr;
  mItemDebugPanel.setItemManager(itemMgr);
  if (mRes) {
    mItemDebugPanel.load(*mRes);
  }
}

void Hud::notifyEntityDeath(Entity *entity) {
  mTargetFrame.notifyEntityDeath(entity);
  mCharacterPanel.notifyEntityDeath(entity);
}

void Hud::updateRTs(Player *player, Entity *targetEntity,
                    const ChunkedTileMap &map, const DecorSystem &decor,
                    const class EntityManager *entityMgr,
                    const TerrainDeformSystem *terrainDeform) {
  mTargetedEntity = targetEntity;
  mPlayerFrame.updateRT(player);
  mTargetFrame.setTarget(targetEntity);
  mTargetFrame.updateRT();
  if (isMapOpen()) {
    mMapPanel.updateTexture(
        map, decor, player ? player->getPosition() : sf::Vector2f(0.f, 0.f),
        entityMgr, terrainDeform);
  }
}

void Hud::updateMap(const ChunkedTileMap &map, const DecorSystem &decor,
                    sf::Vector2f playerPos) {
  mMapPanel.updateTexture(map, decor, playerPos);
}

void Hud::setCombatStatus(const std::string &text, sf::Color color) {
  mCombatStatusText.setString(text);
  mCombatStatusText.setColor(color);
  mCombatStatusClock.restart();
}

void Hud::drawCombatStatus(sf::RenderTarget &target) {
  if (mCombatStatusClock.getElapsedTime().asSeconds() < mCombatStatusDuration) {
    const sf::Vector2f winSize((float)mWindowSize.x, (float)mWindowSize.y);
    sf::FloatRect textBounds = mCombatStatusText.getGlobalBounds();
    mCombatStatusText.setPosition(
        sf::Vector2f(std::floor((winSize.x - textBounds.size.x) * 0.5f),
                     std::floor(winSize.y * cfg::UI::COMBAT_STATUS_Y_PERCENT)));
    target.draw(mCombatStatusText);
  }
}

void Hud::toggleInventory() {
  if (mInventoryOpen) {
    // Closing... check if dragging from inventory
    if (mDragSystem.isDragging() &&
        mDragSystem.getSource() == DragSource::Inventory) {
      mDragSystem.cancelDrag(mPlayerFrame.getPlayer(), mInventoryPanel,
                             mCharacterPanel, mFortifyPanel, *mRes);
    }
  } else {
    bringToFront(&mInventoryPanel);
  }
  mInventoryOpen = !mInventoryOpen;
  mInventoryPanel.setVisible(
      mInventoryOpen); // [REFACTOR] Sync visibility state
}

void Hud::toggleFortify() {
  if (mFortifyPanel.isVisible()) {
    // [SAFETY] Return item to inventory if panel is closing
    auto item = mFortifyPanel.getItem();

    // OR check if we are currently dragging from Fortify
    if (mDragSystem.isDragging() &&
        mDragSystem.getSource() == DragSource::Fortify) {
      mDragSystem.cancelDrag(mPlayerFrame.getPlayer(), mInventoryPanel,
                             mCharacterPanel, mFortifyPanel, *mRes);
      item = nullptr; // Already handled by cancel
    }

    if (item) {
      int slot = findEmptyInventorySlot();
      if (slot != -1) {
        mInventoryPanel.setItem(slot, item);
        mFortifyPanel.setItem(nullptr);
        std::cout << "[Fortify] Item returned to inventory.\n";
      }
    }
  } else {
    bringToFront(&mFortifyPanel);
  }
  mFortifyPanel.toggle();
}

void Hud::toggleCharacterPanel() {
  if (mCharacterPanelOpen) {
    // Closing... check if dragging from character
    if (mDragSystem.isDragging() &&
        mDragSystem.getSource() == DragSource::Character) {
      mDragSystem.cancelDrag(mPlayerFrame.getPlayer(), mInventoryPanel,
                             mCharacterPanel, mFortifyPanel, *mRes);
      // [FIX] Reset hidden slot
      mCharacterPanel.setHiddenSlot(-1);
    }
  } else {
    bringToFront(&mCharacterPanel);
  }
  mCharacterPanelOpen = !mCharacterPanelOpen;
  mCharacterPanel.setVisible(
      mCharacterPanelOpen); // [REFACTOR] Sync visibility state
}

void Hud::toggleTitlesPanel() {
  if (mTitlesPanelOpen) {
    mTitlesPanel.onMouseRelease(); // Stop dragging on close
  } else {
    bringToFront(&mTitlesPanel);
  }
  mTitlesPanelOpen = !mTitlesPanelOpen;
  mTitlesPanel.setVisible(mTitlesPanelOpen);
}

void Hud::toggleCultivoPanel() {
  if (mCultivoPanel.isVisible()) {
    mCultivoPanel.onMouseRelease();
    mCultivoPanel.setVisible(false);
  } else {
    bringToFront(&mCultivoPanel);
    mCultivoPanel.setVisible(true);
  }
}

void Hud::toggleSkillLevelUp() {
  if (mSkillLevelUpPanel.isVisible()) {
    mSkillLevelUpPanel.onMouseRelease();
    mSkillLevelUpPanel.setVisible(false);
  } else {
    bringToFront(&mSkillLevelUpPanel);
    mSkillLevelUpPanel.setVisible(true);
  }
}

void Hud::toggleSkillDebug() {
  if (mSkillDebugPanel.isVisible()) {
    mSkillDebugPanel.onMouseRelease();
    mSkillDebugPanel.setVisible(false);
  } else {
    bringToFront(&mSkillDebugPanel);
    mSkillDebugPanel.setVisible(true);
  }
}

void Hud::toggleItemDebug() {
  bool current = mItemDebugPanel.isVisible();
  if (!current) {
    bringToFront(&mItemDebugPanel);
  }
  mItemDebugPanel.setVisible(!current);
}

void Hud::closeAllPanels(Player *player) {
  if (mDragSystem.isDragging()) {
    mDragSystem.cancelDrag(player, mInventoryPanel, mCharacterPanel,
                           mFortifyPanel, *mRes, mInspectionPanelPtr, &mCultivoPanel);
    mCharacterPanel.setHiddenSlot(-1);
  }

  if (mFortifyPanel.isVisible()) {
    auto item = mFortifyPanel.getItem();
    if (item) {
      int slot = findEmptyInventorySlot();
      if (slot != -1) {
        mInventoryPanel.setItem(slot, item);
        mFortifyPanel.setItem(nullptr);
        std::cout << "[Fortify] Item returned to inventory upon death.\n";
      }
    }
    mFortifyPanel.setVisible(false);
  }

  if (mCultivoPanel.isVisible()) {
    mCultivoPanel.onMouseRelease();
    mCultivoPanel.setVisible(false);
  }

  mInventoryOpen = false;
  mInventoryPanel.setVisible(false);

  mCharacterPanelOpen = false;
  mCharacterPanel.setVisible(mCharacterPanelOpen);

  mTitlesPanelOpen = false;
  mTitlesPanel.setVisible(false);
  mTitlesPanel.onMouseRelease();

  mMapPanel.close();
  mItemDebugPanel.setVisible(false);
}

void Hud::updateFps(sf::Time dt, int activeEntities, int renderedEntities,
                    int activeChunks) {
  mCachedActiveEnts = activeEntities;
  mCachedRenderedEnts = renderedEntities;
  mCachedActiveChunks = activeChunks;

  // [INPUT] Update ChatBox key-hold repeat (backspace, arrows, delete)
  mChatBox.update(dt.asSeconds());
}
