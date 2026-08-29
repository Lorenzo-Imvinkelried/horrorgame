#include "PlayingState.h"
#include "states/MenuState.h"
#include "core/systems/InteractionSystem.h"
#include "Config.h"
#include <algorithm>

void PlayingState::handleInput(Game &game, sf::Time dt) {
  if (!mInitialized)
    return;

  const auto &input = game.getInput();

  if (input.isActionJustPressed(Action::Exit)) {
    if (InteractionSystem::getInstance().isActive()) {
      InteractionSystem::getInstance().cancel();
    } else {
      game.changeState(std::make_unique<MenuState>(game));
    }
    return;
  }

  if (mPlayerPtr && mPlayerPtr->getCurrentHp() <= 0) {
    return;
  }

  if (mUIManager.isChatFocused())
    return;

  mPlayerController.handleInput(game, dt, mView);
}

void PlayingState::handleEvent(Game &game, const sf::Event &ev) {
  mUIManager.handleEvent(game, ev, mPlayerPtr);
}

void PlayingState::onResize(Game &game, int w, int h) {
  if (game.isUsingVirtualResolution()) {
    sf::Vector2u texSize = game.getRenderTexture().getSize();
    mView.setSize({(float)texSize.x, (float)texSize.y});
  } else {
    float zoom = std::max(1.0f, cfg::Window::CAMERA_ZOOM);
    mView.setSize({(float)w / zoom, (float)h / zoom});
  }

  int uiW = w;
  int uiH = h;

  mUIManager.onResize(uiW, uiH);
  mEntityManager.updateActivationRanges(mView.getSize());
}

void PlayingState::clearState() {
}
