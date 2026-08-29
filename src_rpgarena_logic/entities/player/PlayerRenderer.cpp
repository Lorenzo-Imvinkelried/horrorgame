#include "Player.h"
#include "Config.h"
#include "core/engine/ResourceManager.h"
#include "core/engine/animation/Animation.h"

bool Player::loadSkin(ResourceManager &res) {
  mResourceManager = &res;
  bool success = mSkin.loadParts(res);
  if (!mSkin.loadSkeleton(res, "assets/textures/player/esqueleto.json")) {
    mSkin.setCustomRestOffsets(
        cfg::Player::HEAD_OFFSET, cfg::Player::HAND_L_OFFSET,
        cfg::Player::HAND_R_OFFSET, cfg::Player::FOOT_L_OFFSET,
        cfg::Player::FOOT_R_OFFSET);
  }
  return success;
}

void Player::morphInto(const std::string &mobType, float duration,
                       const MobBlueprint &bp) {
  mMorphActive = true;
  mMorphType = mobType;
  mMorphTimer = duration;
  mMorphBlueprint = bp;

  if (mResourceManager) {
    mSkin.morph(mobType, *mResourceManager);
    updateWeaponVisuals(*mResourceManager);
    updateArmorVisuals(*mResourceManager);
  }

  recalculateStats();
}

void Player::revertMorph() {
  if (!mMorphActive)
    return;
  mMorphActive = false;
  mMorphType = "";
  mMorphTimer = 0.f;
  mMorphBlueprint = MobBlueprint();

  if (mResourceManager) {
    mSkin.revert(*mResourceManager);
    updateWeaponVisuals(*mResourceManager);
    updateArmorVisuals(*mResourceManager);
  }

  recalculateStats();
}

void Player::draw(sf::RenderTarget &target, sf::RenderStates states) {
  if (!mIsVisible)
    return;
  mSkin.draw(target, states);
}

void Player::drawLayer(sf::RenderTarget &target, int layer, sf::RenderStates states) {
  if (!mIsVisible)
    return;
  if (layer == 0) {
    mSkin.draw(target, states);
  } else {
    mSkin.drawLayer(target, layer, states);
  }
}

float Player::getLayerSortingY(int layer) const {
  if (mSkin.getAnimation()) {
    return mSkin.getAnimation()->getLayerSortingY(layer, getSortingY());
  }
  return Entity::getLayerSortingY(layer);
}

sf::Vector2f Player::getVisualPoint(const std::string &pointName) const {
  if (pointName == "head") {
    sf::FloatRect h = mSkin.getAnim().getNodeGlobalBounds("head");
    if (h.size.x > 0.f && h.size.y > 0.f) {
      return sf::Vector2f(h.position.x + h.size.x * 0.5f, h.position.y);
    }
    sf::Vector2f p = getPosition();
    p.y -= getVisualHeight();
    return p;
  }
  if (pointName == "hand_right") {
    sf::Vector2f pos = mSkin.getRightHandPosition();
    if (pos != sf::Vector2f(0, 0))
      return pos;
    sf::Vector2f p = getPosition();
    p.y -= 30.f;
    return p;
  }
  if (pointName == "hand_left") {
    sf::Vector2f pos = mSkin.getLeftHandPosition();
    if (pos != sf::Vector2f(0, 0))
      return pos;
    sf::Vector2f p = getPosition();
    p.y -= 30.f;
    return p;
  }
  return Entity::getVisualPoint(pointName);
}

void Player::getShadowRenderData(std::vector<sf::Vertex> &vertices,
                                 const sf::Texture *&texture) const {
  if (!isVisible() || !isAlive()) {
    texture = nullptr;
    return;
  }
  mSkin.getShadowRenderData(vertices, texture);
}

void Player::getWeaponShadowRenderData(std::vector<sf::Vertex> &vertices,
                                       const sf::Texture *&texture,
                                       int slotIndex) const {
  if (!isVisible() || !isAlive()) {
    texture = nullptr;
    return;
  }
  mSkin.getWeaponShadowRenderData(vertices, texture, slotIndex);
}

void Player::getArmorShadowRenderData(std::vector<sf::Vertex> &vertices,
                                      const sf::Texture *&texture,
                                      int slotIndex) const {
  if (!isVisible() || !isAlive()) {
    texture = nullptr;
    return;
  }
  mSkin.getArmorShadowRenderData(vertices, texture, slotIndex);
}
