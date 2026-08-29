#include "PlayingState.h"
#include "Config.h"
#include "utils/PixelSpriteRenderer.h"
#include "core/skills/mage/basic-orb.h"
#include <cmath>

void PlayingState::draw(Game &game, sf::RenderTarget &target) {
  if (!mInitialized) {
    target.clear(sf::Color::Black);
    sf::View oldView = target.getView();
    target.setView(target.getDefaultView());
    mUIManager.drawLoadingScreen(target, static_cast<float>(mInitStep) / 10.f,
                                 mTargetWorldID);
    target.setView(oldView);
    return;
  }
  drawWorld(game, target);
  drawUI(game, target);
}

void PlayingState::drawWorld(Game &game, sf::RenderTarget &target) {
  if (!mInitialized) {
    target.clear(sf::Color::Black);
    sf::View oldView = target.getView();
    target.setView(target.getDefaultView());
    mUIManager.drawLoadingScreen(target, static_cast<float>(mInitStep) / 10.f,
                                 mTargetWorldID);
    target.setView(oldView);
    return;
  }

  mRenderSystem.render(target, mView, mWorldManager, mEntityManager,
                       mParticleSystem, mGoreSystem, mTerrainDeform,
                       mItemDrops, mPlayerController.getTargetedEntity());

  if (cfg::Window::ENABLE_ROTATION_DEBUG && mPlayerPtr) {
      static sf::Clock protoClock;
      float dt = protoClock.restart().asSeconds();

      static float protoAngle = 0.f;
      if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num9)) {
          protoAngle -= 60.f * dt;
      }
      if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num0)) {
          protoAngle += 60.f * dt;
      }
      while (protoAngle < 0.f) protoAngle += 360.f;
      while (protoAngle >= 360.f) protoAngle -= 360.f;

      const sf::Texture& armorTex = game.getResources().getTexture("assets/items/weapons/armor_32x32.png");
      sf::IntRect armorRect(sf::Vector2i(0, 0), sf::Vector2i(32, 32));

      sf::Vector2f basePos = mPlayerPtr->getPosition();
      sf::Vector2f leftPos = basePos + sf::Vector2f(-45.f, -50.f);
      sf::Vector2f centerPos = basePos + sf::Vector2f(0.f, -50.f);
      sf::Vector2f rightPos = basePos + sf::Vector2f(45.f, -50.f);

      sf::Vector2f origin(16.f, 16.f);
      sf::Vector2f scale(1.f, 1.f);

      sf::Sprite refSprite(armorTex);
      refSprite.setTextureRect(armorRect);
      refSprite.setOrigin(origin);
      refSprite.setScale(scale);
      refSprite.setPosition(centerPos);
      refSprite.setRotation(sf::degrees(0.f));
      target.draw(refSprite);

      sf::Sprite leftSprite(armorTex);
      leftSprite.setTextureRect(armorRect);
      leftSprite.setOrigin(origin);
      leftSprite.setScale(scale);
      leftSprite.setPosition(leftPos);
      leftSprite.setRotation(sf::degrees(protoAngle));
      target.draw(leftSprite);

      PixelSpriteRenderer::draw(target, armorTex, armorRect, rightPos, origin, scale, protoAngle, sf::Color::White);
  }

  // Target Indicator
  Entity *targetEnt = mPlayerController.getTargetedEntity();
  if (!targetEnt || !targetEnt->isAlive()) {
    targetEnt = mCombatSystem.getCurrentTarget();
  }

  if (targetEnt && targetEnt->isAlive() && targetEnt != mPlayerPtr) {
    float cx = targetEnt->getPosition().x;
    float top = targetEnt->getPosition().y - targetEnt->getVisualHeight();
    float bobOffset = std::sin(mArrowTimer * 8.f) * 3.f;

    float arrowX = std::floor(cx);
    float arrowY = std::floor(top - 30.f + bobOffset);

    mArrowSprite.setPosition({arrowX, arrowY});
    target.draw(mArrowSprite);
  }

  // [GROUND SKILL TARGETING PREVIEW]
  mPlayerController.drawGroundTargeting(target, mSkillManager, game.getResources());

  if (cfg::Debug::ENABLE_DEBUG_OVERLAY) {
    target.setView(mView);
    mDebugOverlay.draw(target);
  }
}

void PlayingState::drawUI(Game &game, sf::RenderTarget &target) {
  if (!mInitialized) return;

  mUIManager.draw(game, target, mView, mEntityManager, mWorldManager,
                  mPlayerPtr, mPlayerController.getTargetedEntity(),
                  mSkillManager, &mAggroSystem);
}
