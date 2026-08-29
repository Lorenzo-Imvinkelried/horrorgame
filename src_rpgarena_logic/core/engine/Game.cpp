#include "Game.h"
#include "core/managers/ConfigManager.h"
#include "states/MenuState.h"
#include "states/playing/PlayingState.h" // Incluimos el estado inicial
#include "utils/Random.h"
#include "core/systems/SkillUpgradeSystem.h"
#include <tracy/Tracy.hpp>


Game::Game()
    : mRenderSprite(mRenderTexture.getTexture())
    , mLootManager(mItemManager) {
  // [CONFIG] Load runtime config FIRST
  ConfigManager::getInstance().loadConfig("assets/data/config.json");

  // [VIRTUAL RESOLUTION SETUP]
  // 1. Config W/H = Internal Resolution (Performance)
  // 2. Window W/H = Desktop Resolution (Quality/Fullscreen)

  // [VIRTUAL RESOLUTION SETUP]
  // 1. Internal W/H = Game Rendering Resolution (Performance + Pixel Look)
  // 2. Window W/H   = actual Screen Window Size (Fullscreen/Desktop)

  sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
  int winW = (cfg::Window::WIDTH > 0) ? cfg::Window::WIDTH : desktop.size.x;
  int winH = (cfg::Window::HEIGHT > 0) ? cfg::Window::HEIGHT : desktop.size.y;

  int configInternalW = cfg::Window::INTERNAL_WIDTH;
  int configInternalH = cfg::Window::INTERNAL_HEIGHT;

  int internalW = winW;
  int internalH = winH;
  int scaleMultiplier = 1;

  // Si hay resolución interna configurada, calculamos el "Integer Scale"
  // perfecto
  if (configInternalH > 0) {
    scaleMultiplier = std::max(1, winH / configInternalH);

    // Recalculamos la resolucion interna para que sea un divisor exacto de la
    // ventana
    internalW = winW / scaleMultiplier;
    internalH = winH / scaleMultiplier;
  }

  if (internalW <= 0 || internalH <= 0 ||
      (internalW == winW && internalH == winH)) {
    // If virtual resolution is not requested (or matches window size),
    // we still use virtual resolution at 1:1 scale to enable PostFX (which requires a RenderTexture)
    mUseVirtualResolution = true;
    if (!mRenderTexture.resize({static_cast<unsigned int>(winW), static_cast<unsigned int>(winH)})) {
      mUseVirtualResolution = false;
      mWindow.create(
          sf::VideoMode({(unsigned)winW, (unsigned)winH}), cfg::Window::TITLE,
          cfg::Window::FULLSCREEN ? sf::State::Fullscreen : sf::State::Windowed);
    } else {
      mRenderTexture.setSmooth(false);
      mRenderSprite.setTexture(mRenderTexture.getTexture(), true);
      mRenderSprite.setScale({1.f, 1.f});
      mWindow.create(
          sf::VideoMode({(unsigned)winW, (unsigned)winH}), cfg::Window::TITLE,
          cfg::Window::FULLSCREEN ? sf::State::Fullscreen : sf::State::Windowed);
    }
  } else {
    // [MODO OPTIMIZADO]
    mUseVirtualResolution = true;

    // 1. Crear Render Texture (Resolución Interna Baja)
    if (!mRenderTexture.resize({static_cast<unsigned int>(internalW),
                                static_cast<unsigned int>(internalH)})) {
      // Fallback si falla
      mUseVirtualResolution = false;
      mWindow.create(sf::VideoMode({(unsigned)winW, (unsigned)winH}),
                     cfg::Window::TITLE,
                     cfg::Window::FULLSCREEN ? sf::State::Fullscreen
                                             : sf::State::Windowed);
    } else {
      // Texture OK
      mRenderTexture.setSmooth(false); // Crisp Pixel Art look
      mRenderSprite.setTexture(mRenderTexture.getTexture(), true);

      // 2. Crear Ventana REAL
      mWindow.create(sf::VideoMode({(unsigned)winW, (unsigned)winH}),
                     cfg::Window::TITLE,
                     cfg::Window::FULLSCREEN ? sf::State::Fullscreen
                                             : sf::State::Windowed);

      // 3. Escalar el Sprite con el Integer Scale exacto
      sf::Vector2u windowSize = mWindow.getSize();
      float scale = static_cast<float>(scaleMultiplier);
      mRenderSprite.setScale({scale, scale});

      // Centrar por si hay un residuo matemático (ej. 1366 / 3 = 455. Sobra 1
      // pixel)
      float offsetX = (windowSize.x - (internalW * scale)) / 2.0f;
      float offsetY = (windowSize.y - (internalH * scale)) / 2.0f;
      mRenderSprite.setPosition({offsetX, offsetY});
    }
  }

  // Configuración inicial de refresco y framerate
  if (cfg::Window::VSYNC) {
    mWindow.setVerticalSyncEnabled(true);
  } else {
    mWindow.setVerticalSyncEnabled(false);
  }
  mWindow.setFramerateLimit(0); // Usamos nuestro propio frame pacer de alta precisión
  mWindow.setKeyRepeatEnabled(false);

  // [WINDOW ICON] Cargar icono de la ventana y barra de tareas
  sf::Image icon;
  if (icon.loadFromFile("assets/data/game_icon.png")) {
    mWindow.setIcon(icon.getSize(), icon.getPixelsPtr());
  } else {
    std::cerr << "[Game] WARNING: No se pudo cargar el icono: assets/data/game_icon.png\n";
  }

  // Inicializar Random engine
  Random::init();

  // [POST FX] Init post-processing (reads cfg::PostFX::ENABLED)
  mPostFX.init();

  mLootManager.loadLootTables("assets/data/loot_tables.json");
  mGoldSystem.loadGoldDrops("assets/data/gold_drops.json");
  SkillUpgradeSystem::getInstance().loadConfig("assets/data/skill_upgrades.json");

  // INICIAR EL ESTADO DE JUEGO

  // Procedural System - No DB file needed
  // mItemManager.loadDatabase("assets/data/items.dat");

  changeState(std::make_unique<MenuState>(*this));
}

Game::~Game() {}

void Game::changeState(std::unique_ptr<GameState> state) {
  mStates.clear();
  mPostFX.setDesaturateInstantly(0.f); // Reset grayscale effect when changing states
  pushState(std::move(state));
}

void Game::pushState(std::unique_ptr<GameState> state) {
  mStates.push_back(std::move(state));
}

void Game::popState() {
  if (!mStates.empty()) {
    mStates.pop_back();
    mPostFX.setDesaturateInstantly(0.f); // Reset grayscale effect when popping states
  }
}

// [FIXED TIME STEP]
// 60 UPS (Updates Per Second) = 16.66ms per update
static const sf::Time TimePerFrame = sf::seconds(1.f / 60.f);

static sf::Time g_lastDrawTime;

void Game::run() {
  sf::Clock clock;
  sf::Clock frameClock;
  
  static sf::Time accumEvents, accumInput, accumUpdate, accumRender, accumDraw, accumTotal;
  static int frameCount = 0;

  while (mWindow.isOpen()) {
    frameClock.restart();
    sf::Time dt = clock.restart();

    // 1. Process Window Events (Populate InputManager)
    sf::Clock eventsClock;
    processEvents();
    sf::Time eventsTime = eventsClock.getElapsedTime();

    // 2. Handle Game Input (ONCE per frame, using populated InputManager)
    sf::Clock inputClock;
    if (mWindow.hasFocus()) {
      mInputManager.update(); // Update continuous key states
    } else {
      mInputManager.clearAllStates(); // Clear continuous and single-press keys
                                      // if window lost focus
    }

    {
      // [NATIVE] Just ensure it matches window
      mInputManager.setMousePosition(sf::Mouse::getPosition(mWindow));
    }

    if (!mStates.empty()) {
      // Pass 'dt' for smooth input handling (camera, movement)
      mStates.back()->handleInput(*this, dt);
    }
    sf::Time inputTime = inputClock.getElapsedTime();

    // 3. Physics/Logic Update (Variable Time Step with clamp)
    sf::Clock updateClock;
    // Prevent huge spikes if window is dragged or minimized
    sf::Time safeDt = dt;
    if (safeDt.asSeconds() > 0.1f) {
      safeDt = sf::seconds(0.1f);
    }

    update(safeDt);
    mPostFX.update(safeDt); // [POST FX] Advance grain animation timer
    sf::Time updateTime = updateClock.getElapsedTime();

    // 4. Clear Input State (Ready for next frame)
    mInputManager.clearState();

    // 5. Render
    sf::Clock renderClock;
    render();
    sf::Time renderTime = renderClock.getElapsedTime();

    // 6. Limitador de Frames de Alta Precisión (Hybrid Sleep + Micro-Spin)
    if (!cfg::Window::VSYNC && cfg::Window::FPS_LIMIT > 0) {
      const sf::Time targetFrameTime = sf::seconds(1.0f / static_cast<float>(cfg::Window::FPS_LIMIT));
      sf::Time elapsed = frameClock.getElapsedTime();
      while (elapsed < targetFrameTime) {
        sf::Time remaining = targetFrameTime - elapsed;
        if (remaining.asMilliseconds() > 2) {
          sf::sleep(sf::milliseconds(1));
        }
        elapsed = frameClock.getElapsedTime();
      }
    }

    sf::Time totalFrameTime = frameClock.getElapsedTime();

    // [DEBUG PERF] Print every 1s (approx 60 frames)
    accumEvents += eventsTime;
    accumInput += inputTime;
    accumUpdate += updateTime;
    accumRender += renderTime;
    accumDraw += g_lastDrawTime;
    accumTotal += totalFrameTime;
    frameCount++;

    if (accumTotal.asSeconds() >= 1.0f) {
      if (cfg::Debug::ENABLE_PERF_LOG) {
        float fps = (float)frameCount / accumTotal.asSeconds();
        std::cout << "[PERF] FPS: " << fps 
                  << " | Avg Loop: " << (accumTotal.asMicroseconds() / frameCount) / 1000.0f << "ms\n"
                  << "       Events:  " << (accumEvents.asMicroseconds() / frameCount) / 1000.0f << "ms\n"
                  << "       Input:   " << (accumInput.asMicroseconds() / frameCount) / 1000.0f << "ms\n"
                  << "       Update:  " << (accumUpdate.asMicroseconds() / frameCount) / 1000.0f << "ms\n"
                  << "       Draw:    " << (accumDraw.asMicroseconds() / frameCount) / 1000.0f << "ms\n"
                  << "       Display: " << ((accumRender - accumDraw).asMicroseconds() / frameCount) / 1000.0f << "ms\n"
                  << std::endl;
      }
      accumEvents = sf::Time::Zero;
      accumInput = sf::Time::Zero;
      accumUpdate = sf::Time::Zero;
      accumRender = sf::Time::Zero;
      accumDraw = sf::Time::Zero;
      accumTotal = sf::Time::Zero;
      frameCount = 0;
    }

    FrameMark;
  }
}

void Game::processEvents() {
  while (auto ev = mWindow.pollEvent()) {
    if (ev->is<sf::Event::Closed>())
      mWindow.close();

    // [INPUT MAPPING] Virtual Resolution Correction
    sf::Event processedEvent = *ev;
    /* [FIX] DISABLED SCALING - We use HD Input for mapPixelToCoords
    if (mUseVirtualResolution) {
        sf::Vector2u winSize = mWindow.getSize();
        sf::Vector2u virtSize = mRenderTexture.getSize();

        // Calculate scale factors (Window / Virtual)
        // Example: 1600 / 800 = 2.0
        float sX = (float)winSize.x / (float)virtSize.x;
        float sY = (float)winSize.y / (float)virtSize.y;

        if (auto* m = processedEvent.getIf<sf::Event::MouseMoved>()) {
            auto newM = *m;
            newM.position.x = static_cast<int>(newM.position.x / sX);
            newM.position.y = static_cast<int>(newM.position.y / sY);
            processedEvent = newM;
        }
        else if (auto* b =
    processedEvent.getIf<sf::Event::MouseButtonPressed>()) { auto newB = *b;
             newB.position.x = static_cast<int>(newB.position.x / sX);
             newB.position.y = static_cast<int>(newB.position.y / sY);
             processedEvent = newB;
        }
        else if (auto* b =
    processedEvent.getIf<sf::Event::MouseButtonReleased>()) { auto newB = *b;
             newB.position.x = static_cast<int>(newB.position.x / sX);
             newB.position.y = static_cast<int>(newB.position.y / sY);
             processedEvent = newB;
        }
        // MouseWheelScrolled positions are also important!
        else if (auto* w =
    processedEvent.getIf<sf::Event::MouseWheelScrolled>()) { auto newW = *w; //
    Copy newW.position.x = static_cast<int>(newW.position.x / sX);
             newW.position.y = static_cast<int>(newW.position.y / sY);
             processedEvent = newW; // Assign back
        }
    }
    */

    mInputManager.processEvent(processedEvent);

    // --- CORRECCIÓN: PASAR EVENTO AL ESTADO ---
    if (!mStates.empty()) {
      mStates.back()->handleEvent(*this, processedEvent); // Pass Mapped Event
    }
    // ------------------------------------------

    // Resize must use REAL window events, but logic might depend on mapped
    // ones? Actually, onResize usually updates the View. If we use virtual
    // resolution, the View is fixed (handled in onResize fix previously). But
    // we might need to know the new Window Size for aspect ratio calcs. Let's
    // pass the ORIGINAL event for resize to be safe, or just rely on window
    // getter.
    if (const auto *r = ev->getIf<sf::Event::Resized>()) {
      if (mUseVirtualResolution && cfg::Window::INTERNAL_HEIGHT > 0) {
        int currentScale =
            std::max(1, (int)r->size.y / cfg::Window::INTERNAL_HEIGHT);
        float scale = static_cast<float>(currentScale);
        mRenderSprite.setScale({scale, scale});
      }

      if (!mStates.empty())
        mStates.back()->onResize(*this, r->size.x, r->size.y);
    }
  }
}

// [OPTIMIZATION] Input moved to Game::run loop
void Game::update(sf::Time dt) {
  ZoneScoped;
  if (!mStates.empty()) {
    mStates.back()->update(*this, dt); // Only Physics/Logic Update
  }
  // mInputManager.clearState(); // MOVED to run() to prevent clearing inside
  // the while-loop
}

// --- RENDERIZADO PRINCIPAL ---
void Game::render() {
  ZoneScoped;
  sf::Clock drawClock;
  if (mUseVirtualResolution) {
    sf::Vector2u windowSize = mWindow.getSize();
    // Auto-resize RenderTexture to prevent black bars/borders on native high resolution
    if (cfg::Window::INTERNAL_HEIGHT == 0 && mRenderTexture.getSize() != windowSize) {
      (void)mRenderTexture.resize(windowSize);
      mRenderTexture.setSmooth(false);
      mRenderSprite.setTexture(mRenderTexture.getTexture(), false);
      mRenderSprite.setTextureRect(sf::IntRect({0, 0}, {static_cast<int>(windowSize.x), static_cast<int>(windowSize.y)}));
      if (!mStates.empty()) {
        mStates.back()->onResize(*this, windowSize.x, windowSize.y);
      }
    }

    // 1. Dibujar MUNDO en la textura pequeña (Pixel Art Look)
    mRenderTexture.clear();
    if (!mStates.empty()) {
      mStates.back()->drawWorld(*this, mRenderTexture);
    }
    mRenderTexture.display();

    // --- [SUB-PIXEL SNAP] ---
    // Desplazamos el lienzo final para suavizar el movimiento de cámara
    sf::Vector2f scale = mRenderSprite.getScale();

    // Centrar por si hay un residuo matemático
    windowSize = mWindow.getSize();
    sf::Vector2u virtSize = mRenderTexture.getSize();
    float offsetX = (windowSize.x - (virtSize.x * scale.x)) / 2.0f;
    float offsetY = (windowSize.y - (virtSize.y * scale.y)) / 2.0f;

    mRenderSprite.setPosition({offsetX + (mVirtualOffset.x * scale.x),
                               offsetY + (mVirtualOffset.y * scale.y)});

    bool desaturateActive = (mPostFX.getDesaturate() > 0.001f);

    if (desaturateActive) {
      if (mCombinedTexture.getSize() != windowSize) {
        (void)mCombinedTexture.resize(windowSize);
        mCombinedTexture.setSmooth(false);
      }
      
      mCombinedTexture.clear();
      mCombinedTexture.setView(sf::View(sf::FloatRect({0.f, 0.f}, {static_cast<float>(windowSize.x), static_cast<float>(windowSize.y)})));
      
      // Draw post-processed low-res world into combined texture
      if (mPostFX.isActive()) {
        mPostFX.apply(mRenderSprite, mCombinedTexture, scale.x);
      } else {
        mCombinedTexture.draw(mRenderSprite);
      }
      
      // Draw high-res UI into combined texture
      if (!mStates.empty()) {
        mStates.back()->drawUI(*this, mCombinedTexture);
      }
      
      mCombinedTexture.display();
      
      // Draw combined texture to the screen (mWindow) using the grayscale shader
      mWindow.clear();
      mWindow.setView(sf::View(sf::FloatRect({0.f, 0.f}, {static_cast<float>(windowSize.x), static_cast<float>(windowSize.y)})));
      
      sf::Sprite combinedSprite(mCombinedTexture.getTexture());
      mPostFX.applyGrayscale(combinedSprite, mWindow, mPostFX.getDesaturate());
    } else {
      // 2. Dibujar la textura estirada en la pantalla real
      mWindow.clear();
      mWindow.setView(sf::View(sf::FloatRect({0.f, 0.f}, {static_cast<float>(windowSize.x), static_cast<float>(windowSize.y)})));

      if (mPostFX.isActive()) {
        mPostFX.apply(mRenderSprite, mWindow, scale.x);
      } else {
        mWindow.draw(mRenderSprite);
      }

      // 3. Dibujar UI en Alta Resolución (Vectorial/Textos nítidos)
      if (!mStates.empty()) {
        mStates.back()->drawUI(*this, mWindow);
      }
    }

    g_lastDrawTime = drawClock.getElapsedTime();
    mWindow.display();
  } else {
    // Render normal directo a ventana
    mWindow.clear();
    if (!mStates.empty()) {
      mStates.back()->draw(*this, mWindow);
    }
    g_lastDrawTime = drawClock.getElapsedTime();
    mWindow.display();
  }
}