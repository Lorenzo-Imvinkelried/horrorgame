#include "core/managers/UIManager.h"
#include "core/engine/Game.h"
#include "core/skills/SkillManager.h" // [SKILLS]
#include "core/systems/CultivoSystem.h" // [CULTIVO SYSTEM]
#include "Config.h"
#include <iostream>
#include <cmath>
#include <algorithm>

void UIManager::notifyEntityDeath(Entity* entity) {
    mHud.notifyEntityDeath(entity);
    
    // Also check inspection panel
    if (mInspectedEntity == entity) {
        closeInspection();
    }
    mInspectionPanel.notifyEntityDeath(entity);
}

UIManager::UIManager(ResourceManager& res)
    : mHud()
    , mFXSystem() // FXSystem no longer needs Font
    , mHealthBarSystem(&mHud.getFontTexture())
    , mExpBar(&mHud.getFontTexture())
    , mLoadingScreen()
    , mContextMenu(&mHud.getFontTexture())
    , mInspectionPanel(&mHud.getFontTexture(), mInspectSlotSprite)
    , mMiniDiameterPx(cfg::UI::MINIMAP_DIAMETER_DEFAULT)
    , mMiniMarginPx(cfg::UI::MINIMAP_MARGIN_DEFAULT)
{
    // [PIXEL PERFECT UI] Initialize mUIView to reference resolution
    mUIView.setSize({cfg::UI::LOGICAL_WIDTH, cfg::UI::LOGICAL_HEIGHT});
    mUIView.setCenter({cfg::UI::LOGICAL_WIDTH * 0.5f, cfg::UI::LOGICAL_HEIGHT * 0.5f});

    // Configure Inspection Panel Callback
    mInspectionPanel.setOnCloseCallback([this]() {
         this->closeInspection();
    });

    // Init Systems
    mHud.load(res);
    mHud.setInspectionPanel(&mInspectionPanel);
    mHealthBarSystem.load(res); // [NEW] Loading textured bars
    mExpBar.load(res);          // [NEW] Loading ExpBar textures
    mLoadingScreen.load();

    try {
        sf::Texture& tex = res.getTexture("assets/ui/slot.png");
        mInspectSlotSprite.emplace(tex);
        mInspectSlotSprite->setScale(sf::Vector2f(cfg::Map::ZOOM_FACTOR, cfg::Map::ZOOM_FACTOR));
    } catch (...) {
        mInspectSlotSprite.reset();
    }

    try {
        sf::Texture& tex = res.getTexture("assets/ui/minimap_frame.png");
        mMinimapBgSprite.emplace(tex);
    } catch (...) {}

    try {
        sf::Texture& tex = res.getTexture("assets/ui/dot_minimap.png");
        mMinimapPlayerSprite.emplace(tex);
        sf::FloatRect bounds = mMinimapPlayerSprite->getLocalBounds();
        mMinimapPlayerSprite->setOrigin({bounds.size.x * 0.5f, bounds.size.y * 0.5f});
    } catch (...) {}

    try {
        sf::Texture& tex = res.getTexture("assets/ui/cursor.png");
        mCursorSprite.emplace(tex);
        mCursorSprite->setOrigin({cfg::UI::CURSOR_HOTSPOT_X, cfg::UI::CURSOR_HOTSPOT_Y});
    } catch (...) {}


    // Default Minimap Init (will be properly sized in onResize)
    float miniWorldSizePx = static_cast<float>(cfg::UI::MINIMAP_VIEW_SIZE_TILES * cfg::Map::TILE_SIZE);
    mMiniView = sf::View(sf::FloatRect({0.f, 0.f}, {miniWorldSizePx, miniWorldSizePx}));
    
    // Wire up global instance of FXSystem if needed (Legacy support)
    FXSystem::setInstance(&mFXSystem);
}

UIManager::~UIManager() {
    FXSystem::setInstance(nullptr);
}

Hud& UIManager::getHud() { return mHud; }
FXSystem& UIManager::getFXSystem() { return mFXSystem; }

void UIManager::toggleInventory() { mHud.toggleInventory(); }
void UIManager::toggleCharacterPanel() { mHud.toggleCharacterPanel(); }
void UIManager::toggleMap() { mHud.toggleMap(); }
void UIManager::toggleFortify() { mHud.toggleFortify(); } // [NEW]
void UIManager::toggleTitlesPanel() { mHud.toggleTitlesPanel(); } // [NEW]
void UIManager::toggleCultivoPanel() { mHud.toggleCultivoPanel(); } // [CULTIVO SYSTEM]
void UIManager::toggleSkillLevelUp() { mHud.toggleSkillLevelUp(); } // [NEW]

void UIManager::update(Game& game, sf::Time dt, Player* player, int activeEnts, int renderedEnts, int activeChunks) {
    CultivoSystem::getInstance().update(dt.asSeconds());
    mHud.setItemManager(&game.getItemManager());

    // 1. Update UI Layout/Inputs Logic
    sf::Vector2i pixelPos = sf::Mouse::getPosition(game.getWindow());
    
    // [PIXEL PERFECT UI] Use Logical View for Input Mapping
    mHud.updateFps(dt, activeEnts, renderedEnts, activeChunks);
    sf::Vector2f uiMousePos = game.getWindow().mapPixelToCoords(pixelPos, mUIView);
    
    mContextMenu.update(uiMousePos);
    mHud.handleMouseMove(uiMousePos); // [FIX] Update HUD mouse position for tooltips
    mInspectionPanel.onMouseMove(uiMousePos);
    
    mInspectionPanel.onMouseMove(uiMousePos);
    
    // [FPS COUNTER]
    // mHud.updateFps(dt); // [FIX] Removed duplicate call

    mFXSystem.update(dt);
    
    // Updates that depend on Player
    if (player) {
         mExpBar.setProgress(player->getCurrentExp(), player->getNextLevelExp());
    }
}

void UIManager::updateRTs(Game& game, WorldManager& worldManager, EntityManager& entityMgr, Player* player, Entity* targetEntity, const TerrainDeformSystem* terrainDeform) {
    mHud.updateRTs(player, targetEntity, worldManager.getMap(), worldManager.getDecorSystem(), &entityMgr, terrainDeform);

    unsigned rtSize = (unsigned)mMiniDiameterPx;
    if (rtSize >= 2u && mMiniRT.getTexture().getSize().x > 0u) {
        if (mMiniMapTimer.getElapsedTime().asSeconds() >= cfg::UI::MINIMAP_UPDATE_RATE) {
            mMiniMapTimer.restart();
            
            // Limpiar fondo con el color verde solicitado (RGB: 162, 169, 71)
            mMiniRT.clear(sf::Color(162, 169, 71));
            
            // Establecer vista pixel-perfect a la textura
            sf::View rtPix(sf::FloatRect({0.f,0.f}, {(float)rtSize, (float)rtSize}));
            mMiniRT.setView(rtPix);

            float center = (float)rtSize / 2.f;
            sf::Vector2f radarCenter(center, center);

            if (player) {
                float radarRangeWorld = static_cast<float>(cfg::UI::MINIMAP_VIEW_SIZE_TILES * cfg::Map::TILE_SIZE);
                sf::Vector2f playerPos = player->getPosition();

                // Cargar textura minimap_dots de manera estática para máxima eficiencia
                static sf::Texture* dotsTex = nullptr;
                if (!dotsTex) {
                    try {
                        dotsTex = &game.getResources().getTexture("assets/ui/minimap_dots.png");
                    } catch (...) {
                        dotsTex = nullptr;
                    }
                }

                // Iterar sobre los enemigos activos y dibujarlos usando la textura minimap_dots
                for (const auto& ent : entityMgr.getActiveEntities()) {
                    if (!ent || !ent->isAlive() || ent.get() == player) continue;
                    
                    sf::Vector2f entPos = ent->getPosition();
                    sf::Vector2f offset = entPos - playerPos;
                    
                    // Comprobar si está dentro del círculo de cobertura del radar
                    float distSq = offset.x * offset.x + offset.y * offset.y;
                    if (distSq > radarRangeWorld * radarRangeWorld) continue;
                    
                    sf::Vector2f relOffset = (offset / radarRangeWorld) * center;
                    sf::Vector2f dotPos = radarCenter + relOffset;

                    if (dotsTex) {
                        sf::Sprite dotSprite(*dotsTex);
                        // 0,0 es rojo (mobs); 1,0 es amarillo (cuando les pega)
                        if (ent->isFlashing()) {
                            dotSprite.setTextureRect(sf::IntRect({1, 0}, {1, 1}));
                        } else {
                            dotSprite.setTextureRect(sf::IntRect({0, 0}, {1, 1}));
                        }
                        
                        // Escalar el pixel usando la escala x2 global de la UI
                        float scale = cfg::Map::ZOOM_FACTOR;
                        dotSprite.setScale({scale, scale});
                        dotSprite.setOrigin({0.5f, 0.5f});
                        dotSprite.setPosition({std::round(dotPos.x), std::round(dotPos.y)});
                        mMiniRT.draw(dotSprite);
                    } else {
                        // Fallback por si no carga la textura
                        sf::RectangleShape dot({2.f, 2.f});
                        dot.setOrigin({1.f, 1.f});
                        dot.setFillColor(ent->isFlashing() ? sf::Color::Yellow : sf::Color::Red);
                        dot.setPosition({std::round(dotPos.x), std::round(dotPos.y)});
                        mMiniRT.draw(dot);
                    }
                }

                // Dibujar al jugador usando mMinimapPlayerSprite (dot_minimap.png)
                if (mMinimapPlayerSprite) {
                    float zoom = cfg::Map::ZOOM_FACTOR;
                    mMinimapPlayerSprite->setScale({zoom, zoom});
                    mMinimapPlayerSprite->setPosition(radarCenter);
                    mMiniRT.draw(*mMinimapPlayerSprite);
                } else {
                    // Fallback
                    sf::RectangleShape pDot({3.f, 3.f});
                    pDot.setOrigin({1.5f, 1.5f});
                    pDot.setFillColor(sf::Color::Green);
                    pDot.setPosition({std::round(radarCenter.x), std::round(radarCenter.y)});
                    mMiniRT.draw(pDot);
                }
            }
            
            mMiniRT.display();
        }
    }
}

// [FIX] Update signature
void UIManager::draw(Game& game, sf::RenderTarget& target, const sf::View& worldView, EntityManager& entityMgr, WorldManager& worldManager, Player* player, Entity* targetEntity, const SkillManager& skillMgr, const class AggroSystem* aggroSystem) {
    auto& res = game.getResources();

    // 1. World UI Elements
    // sf::View worldView = target.getView(); // REMOVED - using Argument
    
    // [FIX] Pass worldView correctly to apply screen-space mapping
    // We are about to switch to Screen Space, so let's do it now? NO.
    // HealthBarSystem NOW expects to draw using ScreenCoordinates (mapCoordsToPixel).
    // So target MUST be in Screen Space (Default View) when calling draw!
    
    target.setView(target.getDefaultView()); // Switch to HD View
    
    sf::Vector2i mouseI = game.getInput().getMousePosition();
    sf::Vector2f mouseWorldPos = target.mapPixelToCoords(mouseI, worldView);
    mHealthBarSystem.draw(target, worldView, entityMgr.getActiveEntities(), player, targetEntity, mouseWorldPos, aggroSystem);
    
    // FXSystem usually draws in World Space?
    // If FXSystem draws sprites at WorldPos, it needs World View.
    // If FXSystem draws floating texts... FloatingText usually needs to be readable (Screen Space?).
    // Let's assume FXSystem draws World Particles. 
    // IF FXSystem draws damage numbers, they should be Screen Space too for sharpness?
    
    // Let's restore WorldView for FXSystem if it needs it.
    // Checking FXSystem... usually it's particles.
    // Let's restore WorldView momentarily.
    target.setView(worldView);
    mFXSystem.draw(target);
    
    // 2. Switch to Logical UI Space
    target.setView(mUIView);
    
    mExpBar.draw(target);
    
    // Minimap (Uses logical coordinates for its on-screen mask)
    drawMinimapMask(target);
    
    // HUD
    sf::Vector2i pixelPos = sf::Mouse::getPosition(game.getWindow());
    sf::Vector2f uiMousePos = target.mapPixelToCoords(pixelPos, mUIView);
    mHud.handleMouseMove(uiMousePos);

    if (mShowInspectionPanel && mInspectedEntity) {
        if (!mInspectedEntity->isAlive()) {
             mShowInspectionPanel = false;
             mInspectedEntity = nullptr;
             mInspectionPanel.setVisible(false);
        }
    }

    mHud.draw(target, player, targetEntity, game.getResources(), skillMgr); 

    // 3. Draw Overlays
    if (mContextMenu.isActive()) {
        mContextMenu.draw(target);
    }

    // 4. Draw Custom Cursor
    if (mCursorSprite) {
        if (!mCursorInitialized) {
            game.getWindow().setMouseCursorVisible(false);
            mCursorInitialized = true;
        }
        float zoom = cfg::Map::ZOOM_FACTOR;
        mCursorSprite->setScale({zoom, zoom});
        mCursorSprite->setPosition(uiMousePos);
        target.draw(*mCursorSprite);
    } else {
        if (!mCursorInitialized) {
            game.getWindow().setMouseCursorVisible(true);
            mCursorInitialized = true;
        }
    }

    // Restore View
    target.setView(worldView);
}

void UIManager::onResize(int w, int h) {
    // [PIXEL PERFECT UI] Always update layout using LOGICAL resolution
    mExpBar.updateLayout(cfg::UI::LOGICAL_WIDTH, cfg::UI::LOGICAL_HEIGHT);
    mHud.updateLayout(cfg::UI::LOGICAL_WIDTH, cfg::UI::LOGICAL_HEIGHT);

    // Re-calculate minimap based on LOGICAL resolution so it keeps relative size/position
    const float base = std::min(cfg::UI::LOGICAL_WIDTH, cfg::UI::LOGICAL_HEIGHT);
    const float minDiameter = cfg::UI::MINIMAP_MIN_DIAMETER;
    const float minMargin   = cfg::UI::MINIMAP_MIN_MARGIN;
    mMiniDiameterPx = std::max(minDiameter, base * cfg::UI::MINIMAP_WIDTH_FRACTION);
    mMiniMarginPx   = std::max(minMargin,   base * cfg::UI::MINIMAP_MARGIN_FRACTION);
    mMiniDiameterPx = std::min(mMiniDiameterPx, std::max(0.f, cfg::UI::LOGICAL_WIDTH - 2.f * mMiniMarginPx));
    
    unsigned rtSize = (mMiniDiameterPx >= 2.f) ? (unsigned)mMiniDiameterPx : 0u;
    if (rtSize >= 2u) {
        if (mMiniRT.getSize().x != rtSize) {
             if (!mMiniRT.resize({rtSize, rtSize})) {
                 std::cerr << "[UIManager] Error resizing Minimap RenderTexture to " << rtSize << "x" << rtSize << "\n";
             }
        }
        mMiniRT.clear(sf::Color(10, 10, 10, 0));
        mMiniRT.display();

        float r = mMiniDiameterPx * 0.5f;
        mMiniMask = sf::CircleShape(r, 64);
        mMiniMask.setOrigin({r, r});
        mMiniMask.setFillColor(sf::Color::White);
        // [MODIFIED] Se quitó el outline procedural para que el usuario dibuje su frame
        // mMiniMask.setOutlineThickness(1.f);
        // mMiniMask.setOutlineColor(sf::Color(80, 80, 80));
        mMiniMask.setTexture(&mMiniRT.getTexture());
        mMiniMask.setTextureRect(sf::IntRect({0,0}, {(int)rtSize, (int)rtSize}));
    } 
 
}

void UIManager::handleInput(Game& game, sf::Time dt, Player* player) {
    const auto& input = game.getInput(); 
    
    if (input.isActionJustPressed(Action::ToggleFps)) {
        mHud.toggleFps();
        cfg::Debug::ENABLE_PERF_LOG = !cfg::Debug::ENABLE_PERF_LOG;
    }

    // UI Toggles
    if (input.isActionJustPressed(Action::OpenInventory)) mHud.toggleInventory();
    if (input.isActionJustPressed(Action::OpenCharacterPanel)) mHud.toggleCharacterPanel();
    if (input.isActionJustPressed(Action::OpenMap)) mHud.toggleMap();
    if (input.isActionJustPressed(Action::ToggleFortify)) mHud.toggleFortify(); // [NEW]
    if (input.isActionJustPressed(Action::DebugItems)) mHud.toggleItemDebug(); // [NEW]
    if (input.isActionJustPressed(Action::OpenTitlesPanel)) mHud.toggleTitlesPanel(); // [NEW]
    if (input.isActionJustPressed(Action::OpenSkillLevelUpPanel)) mHud.toggleSkillLevelUp(); // [NEW]
    if (input.isActionJustPressed(Action::OpenSkillDebugPanel)) mHud.toggleSkillDebug(); // [NEW DEBUG]
    
    // --- REALTIME SHADOW ROTATION (DEBUG) ---
    static float baseScaleY = -1.f;
    if (baseScaleY < 0.f) {
        float startAngleRad = cfg::Shadow::SUN_ANGLE * 3.14159265f / 180.f;
        baseScaleY = cfg::Shadow::SCALE_Y * std::cos(startAngleRad);
    }

    bool angleChanged = false;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Period)) {
        float angle = cfg::Shadow::SUN_ANGLE + 60.f * dt.asSeconds(); // 60 grados por segundo
        if (angle > 75.f) angle = 75.f;
        cfg::Shadow::SUN_ANGLE = angle;
        angleChanged = true;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Comma)) {
        float angle = cfg::Shadow::SUN_ANGLE - 60.f * dt.asSeconds(); // 60 grados por segundo
        if (angle < -75.f) angle = -75.f;
        cfg::Shadow::SUN_ANGLE = angle;
        angleChanged = true;
    }

    if (angleChanged) {
        float angleRad = cfg::Shadow::SUN_ANGLE * 3.14159265f / 180.f;
        cfg::Shadow::SKEW_X = -std::tan(angleRad);
        cfg::Shadow::SCALE_Y = baseScaleY / std::cos(angleRad);
        
        static sf::Clock logClock;
        if (cfg::Debug::ENABLE_PERF_LOG && logClock.getElapsedTime().asSeconds() > 0.1f) {
            std::cout << "[DEBUG] Sun Angle: " << std::round(cfg::Shadow::SUN_ANGLE) 
                      << " deg | Skew X: " << cfg::Shadow::SKEW_X 
                      << " | Scale Y: " << cfg::Shadow::SCALE_Y << "\n";
            logClock.restart();
        }
    }

    if (player && !player->isAlive()) {
        return;
    }
    
    // Debug Restock
     if (input.isActionJustPressed(Action::DebugRestock) && player && cfg::Debug::ENABLE_WEAPONS_DEBUG) {
        std::cout << "[Debug] Resetting Inventory and Restocking...\n";
        mHud.clearInventory();
        mHud.addAllItemsToInventory(game.getItemManager(), player, player->getLevel());
    }
}

void UIManager::handleEvent(Game& game, const sf::Event& ev, Player* player) {
    // Unfocus chat if clicked outside (either left or right click)
    if (mHud.isChatFocused()) {
        if (const auto* mb = ev.getIf<sf::Event::MouseButtonPressed>()) {
            sf::Vector2f uiPos = game.getWindow().mapPixelToCoords(mb->position, mUIView);
            if (!mHud.isMouseOverChat(uiPos)) {
                mHud.setChatFocused(false);
            }
        }
    }

    // [NEW] HUD / Chat Handling (Captures keyboard focus)
    mHud.handleEvent(ev, player);

    // 1. Context Menu
    if (mContextMenu.handleEvent(ev)) return; 

    // [CHAT TEXT SELECTION] Map mouse to logical space for click-to-position and drag-select
    if (const auto* mb = ev.getIf<sf::Event::MouseButtonPressed>()) {
        if (mb->button == sf::Mouse::Button::Left) {
            sf::Vector2f uiPos = game.getWindow().mapPixelToCoords(mb->position, mUIView);
            mHud.handleChatTextPress(uiPos);
        }
    }
    if (const auto* mm2 = ev.getIf<sf::Event::MouseMoved>()) {
        sf::Vector2f uiPos = game.getWindow().mapPixelToCoords(mm2->position, mUIView);
        mHud.handleChatTextMove(uiPos);
    }
    if (const auto* mr = ev.getIf<sf::Event::MouseButtonReleased>()) {
        if (mr->button == sf::Mouse::Button::Left) {
            mHud.handleChatTextRelease();
        }
    }

    // 2. HUD
    if (const auto* m = ev.getIf<sf::Event::MouseButtonReleased>()) {
        if (m->button == sf::Mouse::Button::Left) mHud.handleMouseRelease();
    }
    if (const auto* m = ev.getIf<sf::Event::MouseMoved>()) {
        // [PIXEL PERFECT UI] Map mouse to logical space
        sf::Vector2f uiMousePos = game.getWindow().mapPixelToCoords(m->position, mUIView);
        mHud.handleMouseMove(uiMousePos);
    }

    // 3. Inspection Panel
    if (const auto* mb = ev.getIf<sf::Event::MouseButtonReleased>()) {
        mInspectionPanel.onMouseRelease();
    }
    
    // 4. Scroll (Supports Touchpad & Mouse Wheel)
    if (const auto* wheel = ev.getIf<sf::Event::MouseWheelScrolled>()) {
        if (wheel->wheel == sf::Mouse::Wheel::Vertical) {
            int delta = static_cast<int>(wheel->delta);
            if (delta == 0 && wheel->delta != 0.f) {
                delta = (wheel->delta > 0.f) ? 1 : -1;
            }
            mHud.handleScroll(delta);
        }
    }
}

bool UIManager::isMouseOverUI(sf::Vector2f mousePos) const {
    if (mHud.checkBagClick(mousePos)) return true;
    if (mHud.checkCharacterClick(mousePos)) return true;
    if (mHud.isMouseOverUI(mousePos)) return true;
    
    if (mContextMenu.isActive() && mContextMenu.getBounds().contains(mousePos)) return true;
    if (mShowInspectionPanel && mInspectionPanel.getBounds().contains(mousePos)) return true;
    
    return false;
}

// [NEW]
bool UIManager::handleInteract(sf::Vector2f mousePos, Player* player) {
    if (mHud.checkBagClick(mousePos)) { mHud.toggleInventory(); return true; }
    if (mHud.checkCharacterClick(mousePos)) { mHud.toggleCharacterPanel(); return true; }
    
    if (mHud.isMouseOverUI(mousePos)) { 
        if (mHud.handleMousePress(mousePos)) return true; 
    } 

    if (player && !player->isAlive()) return false;
    
    return false;
}

// [NEW]
bool UIManager::handleRightClick(sf::Vector2f mousePos, Player* player, Entity* targetedEntity, EntityManager& em, ResourceManager& res, const InputManager& input) {
    // [NEW] Persisten Inspection Panel Right-Click Blocking
    if (mShowInspectionPanel && mInspectionPanel.getBounds().contains(mousePos)) {
        mHud.handleRightClick(mousePos, player, res, input);
        return true; 
    }

    // 1A. Context Menu (Target Frame)
    if (mHud.getTargetFrameBounds().contains(mousePos)) {
            mContextMenu.show(mousePos, {"INSPECCIONAR"}, [this, targetedEntity, &em](const std::string& opt) {
                if (opt == "INSPECCIONAR") {
                     // [SAFETY] Check if entity pointer is still valid!
                     if (em.isValid(targetedEntity)) {
                        inspectEntity(targetedEntity);
                     } else {
                         std::cout << "[UIManager] Warn: Cannot inspect, entity no longer invalid/exists.\n";
                     }
                }
            });
            return true;
    }

    // 1B. HUD
    if (mHud.handleRightClick(mousePos, player, res, input)) return true;
    
    if (player && !player->isAlive()) return false;
    
    return false;
}

void UIManager::renderMinimap(Game& game, sf::RenderTarget& target, WorldManager& worldManager, Player* player) {
    // Moved to updateRTs and drawMinimapMask
}

void UIManager::drawMinimapMask(sf::RenderTarget& target) {
    // [PIXEL PERFECT UI] Minimap mask is now drawn in logical view coordinates
    float halfD = mMiniDiameterPx * 0.5f;
    float cx = cfg::UI::LOGICAL_WIDTH - mMiniMarginPx - halfD;
    float cy = mMiniMarginPx + halfD;
    
    cx = std::max(halfD + mMiniMarginPx, std::min(cx, cfg::UI::LOGICAL_WIDTH - halfD - mMiniMarginPx));
    cy = std::max(halfD + mMiniMarginPx, std::min(cy, cfg::UI::LOGICAL_HEIGHT - halfD - mMiniMarginPx));

    // Si existe el frame manual, lo dibujamos debajo (o encima, según convenga. Lo ponemos debajo por si tiene fondo ciego, 
    // pero usualmente el frame envuelve. Si es un anillo, puede ir encima. Lo pongo debajo para que el mapa quede dentro).
    // Si el usuario quiere que esté por encima para tapar bordes, lo dibuja después.
    mMiniMask.setPosition({cx, cy});
    target.draw(mMiniMask);

    if (mMinimapBgSprite) {
        float zoom = cfg::Map::ZOOM_FACTOR;
        // Centrar el sprite del frame manual en el mismo lugar que el minimapa
        sf::FloatRect bounds = mMinimapBgSprite->getLocalBounds();
        mMinimapBgSprite->setOrigin({bounds.size.x * 0.5f, bounds.size.y * 0.5f});
        mMinimapBgSprite->setPosition({cx, cy});
        mMinimapBgSprite->setScale({zoom, zoom}); // [ADDED] Escala al igual que el resto
        target.draw(*mMinimapBgSprite);
    }
}

void UIManager::inspectEntity(Entity* entity) {
    if (entity) {
        mInspectedEntity = entity;
        mShowInspectionPanel = true;
        mInspectionPanel.setEntity(entity);
        mInspectionPanel.setPosition({400.f, 150.f});
        mInspectionPanel.setVisible(true);
        mHud.bringToFront(&mInspectionPanel);
    }
}

void UIManager::closeInspection() {
    mInspectedEntity = nullptr;
    mShowInspectionPanel = false;
    mInspectionPanel.setEntity(nullptr);
    mInspectionPanel.setVisible(false);
}

void UIManager::showLoadingScreen(sf::RenderWindow& window, const std::string& worldID) {
    mLoadingScreen.show(window, worldID);
}

void UIManager::drawLoadingScreen(sf::RenderTarget& target, float progress, const std::string& worldID) {
    mLoadingScreen.draw(target, progress, worldID);
}

void UIManager::waitForLoading(sf::RenderWindow& window, sf::Time time) {
    mLoadingScreen.waitForMinTime(window, time);
}

sf::Vector2f UIManager::clampViewCenter(sf::Vector2f desired, sf::Vector2f viewSize, sf::Vector2u mapPx) const {
    const float halfW = viewSize.x * 0.5f;
    const float halfH = viewSize.y * 0.5f;
    const float minX = halfW;
    const float minY = halfH;
    const float maxX = std::max(minX, static_cast<float>(mapPx.x) - halfW);
    const float maxY = std::max(minY, static_cast<float>(mapPx.y) - halfH);
    return { std::clamp(desired.x, minX, maxX), std::clamp(desired.y, minY, maxY) };
}
