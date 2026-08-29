#include "states/MenuState.h"
#include "Config.h"
#include "core/engine/Game.h"
#include "core/systems/SoundSystem.h" // [NEW]
#include "states/playing/PlayingState.h"
#include "animator/AnimatorState.h" // [ANIMATOR STUDIO]
#include <cmath>
#include <iostream>

MenuState::MenuState(Game &game) {
  mFontTexture = &game.getResources().getTexture("assets/fonts/font.png");
  mView.setSize({cfg::UI::LOGICAL_WIDTH, cfg::UI::LOGICAL_HEIGHT});
  mView.setCenter(
      {cfg::UI::LOGICAL_WIDTH * 0.5f, cfg::UI::LOGICAL_HEIGHT * 0.5f});

  // Título Principal "RPG ARENA"
  mTitleText.setTexture(mFontTexture);
  mTitleText.setString("RPG ARENA");
  mTitleText.setScale({cfg::UI::FONT_SCALE, cfg::UI::FONT_SCALE});
  mTitleText.setColor(
      sf::Color(249, 194, 43)); // COLOR ORO (Palette Gold #F9C22B)

  sf::FloatRect titleBounds = mTitleText.getLocalBounds();
  mTitleText.setOrigin({std::round(titleBounds.size.x * 0.5f),
                        std::round(titleBounds.size.y * 0.5f)});
  mTitleText.setPosition({std::round(cfg::UI::LOGICAL_WIDTH * 0.5f), 180.f});

  // Caja del Botón "PLAY"
  mPlayButton.setSize({260.f, 50.f});
  mPlayButton.setFillColor(
      sf::Color(50, 51, 83, 255)); // Slate 800 (Palette Slate Blue #323353)
  mPlayButton.setOutlineThickness(2.f);
  mPlayButton.setOutlineColor(
      sf::Color(249, 194, 43)); // Borde dorado (Palette Gold #F9C22B)
  mPlayButton.setOrigin({130.f, 25.f});
  mPlayButton.setPosition({std::round(cfg::UI::LOGICAL_WIDTH * 0.5f), 320.f});

  // Texto dentro del Botón "PLAY"
  mPlayText.setTexture(mFontTexture);
  mPlayText.setString("PLAY");
  mPlayText.setScale({cfg::UI::FONT_SCALE, cfg::UI::FONT_SCALE});
  mPlayText.setColor(sf::Color::White);

  sf::FloatRect playBounds = mPlayText.getLocalBounds();
  mPlayText.setOrigin({std::round(playBounds.size.x * 0.5f),
                       std::round(playBounds.size.y * 0.5f)});
  mPlayText.setPosition({std::round(cfg::UI::LOGICAL_WIDTH * 0.5f), 320.f});

  // Caja del Botón "ANIMADOR"
  mAnimatorButton.setSize({260.f, 50.f});
  mAnimatorButton.setFillColor(sf::Color(50, 51, 83, 255));
  mAnimatorButton.setOutlineThickness(2.f);
  mAnimatorButton.setOutlineColor(sf::Color(249, 194, 43));
  mAnimatorButton.setOrigin({130.f, 25.f});
  mAnimatorButton.setPosition({std::round(cfg::UI::LOGICAL_WIDTH * 0.5f), 390.f});

  // Texto dentro del Botón "ANIMADOR"
  mAnimatorText.setTexture(mFontTexture);
  mAnimatorText.setString("ANIMADOR");
  mAnimatorText.setScale({cfg::UI::FONT_SCALE, cfg::UI::FONT_SCALE});
  mAnimatorText.setColor(sf::Color::White);

  sf::FloatRect animBounds = mAnimatorText.getLocalBounds();
  mAnimatorText.setOrigin({std::round(animBounds.size.x * 0.5f),
                           std::round(animBounds.size.y * 0.5f)});
  mAnimatorText.setPosition({std::round(cfg::UI::LOGICAL_WIDTH * 0.5f), 390.f});

  // Caja del Botón "SONIDO: ACTIVADO/DESACTIVADO"
  mMuteButton.setSize({260.f, 50.f});
  mMuteButton.setFillColor(sf::Color(50, 51, 83, 255));
  mMuteButton.setOutlineThickness(2.f);
  mMuteButton.setOutlineColor(sf::Color(249, 194, 43));
  mMuteButton.setOrigin({130.f, 25.f});
  mMuteButton.setPosition({std::round(cfg::UI::LOGICAL_WIDTH * 0.5f), 460.f});

  // Texto dentro del Botón "SONIDO"
  mMuteText.setTexture(mFontTexture);
  mMuteText.setString(SoundSystem::isMuted() ? "SONIDO: DESACTIVADO"
                                             : "SONIDO: ACTIVADO");
  mMuteText.setScale({cfg::UI::FONT_SCALE * 1.0f, cfg::UI::FONT_SCALE * 1.0f});
  mMuteText.setColor(sf::Color::White);

  sf::FloatRect muteBounds = mMuteText.getLocalBounds();
  mMuteText.setOrigin({std::round(muteBounds.size.x * 0.5f),
                       std::round(muteBounds.size.y * 0.5f)});
  mMuteText.setPosition({std::round(cfg::UI::LOGICAL_WIDTH * 0.5f), 460.f});

  // Cargar cursor personalizado
  try {
    sf::Texture &tex = game.getResources().getTexture("assets/ui/cursor.png");
    mCursorSprite.emplace(tex);
    mCursorSprite->setOrigin(
        {cfg::UI::CURSOR_HOTSPOT_X, cfg::UI::CURSOR_HOTSPOT_Y});
    mHasCursor = true;
  } catch (...) {
    mHasCursor = false;
  }
}

void MenuState::handleInput(Game &game, sf::Time dt) {
  const auto &input = game.getInput();

  // Atajo teclado enter para jugar directamente
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Enter)) {
    game.changeState(std::make_unique<PlayingState>(game));
    return;
  }

  // Si se pulsa escape en el menú, se cierra el juego por completo
  if (input.isActionJustPressed(Action::Exit)) {
    game.getWindow().close();
  }
}

void MenuState::update(Game &game, sf::Time dt) {
  // Asegurar que el cursor del sistema esté oculto si usamos el personalizado
  if (mHasCursor) {
    game.getWindow().setMouseCursorVisible(false);
  }

  // Detectar si el puntero del mouse está SOBRE los botones
  sf::Vector2i mousePos = sf::Mouse::getPosition(game.getWindow());
  sf::Vector2f mouseWorld = game.getWindow().mapPixelToCoords(mousePos, mView);

  // Botón PLAY
  sf::FloatRect playBounds = mPlayButton.getGlobalBounds();
  if (playBounds.contains(mouseWorld)) {
    mHoveringPlay = true;
    mPlayButton.setFillColor(sf::Color(72, 74, 119, 255)); // Slate 700
    mPlayText.setColor(sf::Color(249, 194, 43));           // Oro
  } else {
    mHoveringPlay = false;
    mPlayButton.setFillColor(sf::Color(50, 51, 83, 255)); // Slate 800
    mPlayText.setColor(sf::Color::White);
  }

  // Botón ANIMADOR
  sf::FloatRect animBounds = mAnimatorButton.getGlobalBounds();
  if (animBounds.contains(mouseWorld)) {
    mHoveringAnimator = true;
    mAnimatorButton.setFillColor(sf::Color(72, 74, 119, 255)); // Slate 700
    mAnimatorText.setColor(sf::Color(249, 194, 43));           // Oro
  } else {
    mHoveringAnimator = false;
    mAnimatorButton.setFillColor(sf::Color(50, 51, 83, 255)); // Slate 800
    mAnimatorText.setColor(sf::Color::White);
  }

  // Botón MUTE
  sf::FloatRect muteBtnBounds = mMuteButton.getGlobalBounds();
  if (muteBtnBounds.contains(mouseWorld)) {
    mHoveringMute = true;
    mMuteButton.setFillColor(sf::Color(72, 74, 119, 255)); // Slate 700
    mMuteText.setColor(sf::Color(249, 194, 43));           // Oro
  } else {
    mHoveringMute = false;
    mMuteButton.setFillColor(sf::Color(50, 51, 83, 255)); // Slate 800
    mMuteText.setColor(sf::Color::White);
  }
}

void MenuState::drawWorld(Game &game, sf::RenderTarget &target) {
  target.clear(sf::Color(46, 34, 47)); // Slate 900
}

void MenuState::drawUI(Game &game, sf::RenderTarget &target) {
  sf::View oldView = target.getView();
  target.setView(mView); // Forzamos nuestra vista lógica de UI

  // Dibujar elementos del menú
  target.draw(mPlayButton);
  target.draw(mPlayText);
  target.draw(mAnimatorButton);
  target.draw(mAnimatorText);
  target.draw(mMuteButton);
  target.draw(mMuteText);
  target.draw(mTitleText);

  // Dibujar cursor personalizado
  if (mHasCursor && mCursorSprite) {
    sf::Vector2i mousePos = sf::Mouse::getPosition(game.getWindow());
    sf::Vector2f uiMousePos = target.mapPixelToCoords(mousePos, mView);

    float zoom = cfg::Map::ZOOM_FACTOR;
    mCursorSprite->setScale({zoom, zoom});
    mCursorSprite->setPosition(uiMousePos);
    target.draw(*mCursorSprite);
  }

  target.setView(oldView); // Volvemos a la vista anterior
}

void MenuState::draw(Game &game, sf::RenderTarget &target) {
  drawWorld(game, target);
  drawUI(game, target);
}

void MenuState::onResize(Game &game, int w, int h) {}

void MenuState::handleEvent(Game &game, const sf::Event &ev) {
  if (const auto *mb = ev.getIf<sf::Event::MouseButtonPressed>()) {
    if (mb->button == sf::Mouse::Button::Left) {
      if (mHoveringPlay) {
        game.changeState(std::make_unique<PlayingState>(game));
      } else if (mHoveringAnimator) {
        game.changeState(std::make_unique<AnimatorState>(game));
      } else if (mHoveringMute) {
        // Alternar estado de mute
        bool currentMuted = SoundSystem::isMuted();
        SoundSystem::setMuted(!currentMuted);

        // Actualizar texto y centrado
        mMuteText.setString(SoundSystem::isMuted() ? "SONIDO: DESACTIVADO"
                                                   : "SONIDO: ACTIVADO");
        sf::FloatRect muteBounds = mMuteText.getLocalBounds();
        mMuteText.setOrigin({std::round(muteBounds.size.x * 0.5f),
                             std::round(muteBounds.size.y * 0.5f)});
        mMuteText.setPosition(
            {std::round(cfg::UI::LOGICAL_WIDTH * 0.5f), 460.f});
      }
    }
  }
}
