#include "Player.h"
#include "Config.h"
#include "core/engine/InputManager.h"
#include "core/engine/animation/Animation.h"
#include "core/systems/ShieldSystem.h"
#include "utils/PhysicsUtils.h"
#include <algorithm>
#include <cmath>

void Player::handleInput(const InputManager &input, sf::Time dt) {
  const float s = dt.asSeconds();
  sf::Vector2f move{0.f, 0.f};

  mMovingManual = false;

  // [STUN] Block Input
  if (isStunned()) {
    mIsMoving = false;
    mMovingManual = false;
    mVelocity = {0.f, 0.f};
    return;
  }

  // [CHARGE] Block manual movement input
  if (mIsCharging) {
    return;
  }

  // 1. Calcular dirección relativa del input
  sf::Vector2f dir{0.f, 0.f};
  if (input.isActionActive(Action::MoveRight))
    dir.x += 1.f;
  if (input.isActionActive(Action::MoveLeft))
    dir.x -= 1.f;
  if (input.isActionActive(Action::MoveUp))
    dir.y -= 1.f;
  if (input.isActionActive(Action::MoveDown))
    dir.y += 1.f;

  // 2. Normalizar vector para que la velocidad diagonal no sea mayor
  if (dir.x != 0.f || dir.y != 0.f) {
    mMovingManual = true;
    float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
    dir /= len;

    float effectiveSpeed = mSpeed * getSpeedMultiplier();
    mPos += dir * effectiveSpeed * s;
    mIsMoving = true;
    mVelocity = dir * effectiveSpeed;
  } else {
    mMovingManual = false;
    mIsMoving = false;
    mVelocity = {0.f, 0.f};
  }

  // 3. Orientación y límites
  if ((!isAttacking() || isCasting()) && mIsMoving) {
    int prevFacing = mFacingDir;
    if (dir.x > 0.01f)
      mFacingDir = 1;
    else if (dir.x < -0.01f)
      mFacingDir = -1;
    if (prevFacing != mFacingDir && mResourceManager && hasShieldEquipped()) {
      updateWeaponVisuals(*mResourceManager);
    }
  }

  if (mMovingManual) {
    mFollowTarget = nullptr;
    mFollowRangeOverride = -1.f;
  }

  // Clamp / Límites del mundo
  float margin = cfg::World::BOUNDS_MARGIN;
  float minX = mWorldBounds.position.x + margin;
  float maxX = mWorldBounds.position.x + mWorldBounds.size.x - margin;
  mPos.x = std::clamp(mPos.x, minX, maxX);

  float minY = mWorldBounds.position.y + margin;
  float maxY = mWorldBounds.position.y + mWorldBounds.size.y - margin;
  mPos.y = std::clamp(mPos.y, minY, maxY);
}

void Player::update(sf::Time dt) {
  // Lógica de seguimiento automático (si no nos movemos manualmente)
  if (!mMovingManual && mFollowTarget && mFollowTarget->isAlive()) {
    updateFollowTarget(dt);
  } else if (!mMovingManual) {
    mVelocity = {0.f, 0.f};
  }

  // Actualizar Estado de Guardia (Escudo activo)
  auto isShield = [](const std::shared_ptr<Item>& item) {
    return item && item->isShield();
  };
  bool shieldRight = isShield(mEquipment[static_cast<int>(EquipmentSlot::MainHand)]);
  bool shieldLeft = isShield(mEquipment[static_cast<int>(EquipmentSlot::OffHand)]);
  if (!shieldRight && !shieldLeft && isGuardActive()) {
    if (auto* ss = ShieldSystem::getInstance()) {
      ss->setGuardActive(this, false);
    } else {
      setGuardActive(false);
    }
  }
  mSkin.setGuardState(isGuardActive(), shieldLeft, shieldRight);

  bool wasShieldAttacking = mSkin.isShieldAttacking();
  // Actualizar Animación (Skin)
  mSkin.update(dt, mIsMoving, mPos, mFacingDir, mSpeed * getSpeedMultiplier(), mTerrainDeform);
  if (wasShieldAttacking && !mSkin.isShieldAttacking() && mResourceManager) {
    updateWeaponVisuals(*mResourceManager);
  }

  // [HP REGEN LOGIC]
  // Solo regenerar si NO estamos en combate
  if (!isInCombat() && mHpRegenPercent > 0.f && mCurrentHp < mMaxHp &&
      isAlive()) {
    float regenPerSec = (float)mMaxHp * (mHpRegenPercent / 100.f);
    mHpRegenAccumulator += regenPerSec * dt.asSeconds();

    if (mHpRegenAccumulator >= 1.f) {
      int healAmount = static_cast<int>(mHpRegenAccumulator);
      heal(healAmount);
      mHpRegenAccumulator -= healAmount;
    }
  }

  // [MP REGEN LOGIC]
  if (!isInCombat() && mMpRegenPercent > 0.f && mCurrentMp < mMaxMp &&
      isAlive()) {
    float regenPerSec = (float)mMaxMp * (mMpRegenPercent / 100.f);
    mMpRegenAccumulator += regenPerSec * dt.asSeconds();

    if (mMpRegenAccumulator >= 1.f) {
      int restoreAmount = static_cast<int>(mMpRegenAccumulator);
      restoreMana(restoreAmount);
      mMpRegenAccumulator -= restoreAmount;
    }
  }

  // [SKILLS] Update Cooldowns
  updateCooldowns(dt);

  // [BUFF SYSTEM]
  updateBuffs(dt);

  // [MORPH SYSTEM]
  if (mMorphActive) {
    mMorphTimer -= dt.asSeconds();
    if (mMorphTimer <= 0.f) {
      revertMorph();
    }
  }

  // [AGGRO FIX]
  updateAggro(dt);
}

void Player::setFollowTarget(Entity *target, float customRange) {
  mFollowTarget = target;
  mFollowRangeOverride = customRange;
}

void Player::updateFollowTarget(sf::Time dt) {
  if (!mFollowTarget || !mFollowTarget->isAlive()) {
    mFollowTarget = nullptr;
    mFollowRangeOverride = -1.f;
    mIsMoving = false;
    return;
  }

  // Si el jugador está realizando un ataque (y no embistiendo ni casteando), bloqueamos su movimiento automático de seguimiento
  if (isAttacking() && !isCharging() && !isCasting()) {
    mIsMoving = false;
    mVelocity = {0.f, 0.f};
    return;
  }

  float targetRange =
      (mFollowRangeOverride >= 0.f) ? mFollowRangeOverride : getAttackRange();
  
  // Nos acercamos 8 píxeles más adentro del rango del objetivo para evitar jitter / stutter-step en los límites
  float stopRange = std::max(10.f, targetRange - 8.f);
  sf::Vector2f diff = mFollowTarget->getPosition() - mPos;
  float distSq = diff.x * diff.x + diff.y * diff.y;
  float rangeSq = stopRange * stopRange;

  if (distSq > rangeSq) {
    float dist = std::sqrt(distSq);
    sf::Vector2f moveDir = diff / dist;
    float effectiveSpeed = mSpeed * getSpeedMultiplier();
    mPos += moveDir * effectiveSpeed * dt.asSeconds();

    int prevFacing = mFacingDir;
    mFacingDir = (moveDir.x > 0.f) ? 1 : -1;
    if (prevFacing != mFacingDir && mResourceManager && hasShieldEquipped()) {
      updateWeaponVisuals(*mResourceManager);
    }
    mIsMoving = true;
    mVelocity = moveDir * effectiveSpeed;
  } else {
    mIsMoving = false;
    mVelocity = {0.f, 0.f};
  }
}

void Player::handleEnvironmentCollisions(
    const std::vector<sf::FloatRect> &obstacles) {
  float feetWidth = cfg::Player::FEET_WIDTH;
  float feetHeight = cfg::Player::FEET_HEIGHT;
  float feetYOffset = cfg::YSorting::PLAYER;

  PhysicsUtils::resolveEnvironmentCollisions(mPos, obstacles, feetWidth,
                                             feetHeight, feetYOffset);
}

void Player::startAttackAnimation(Entity *target, float speedMultiplier) {
  if (speedMultiplier <= 0.f) speedMultiplier = 1.0f;

  // 1. AUTO-AIM (Mirar al objetivo)
  if (target) {
    float dx = target->getPosition().x - mPos.x;
    if (std::abs(dx) > 1.0f) {
      int prevFacing = mFacingDir;
      mFacingDir = (dx > 0) ? 1 : -1;
      if (prevFacing != mFacingDir && mResourceManager && hasShieldEquipped()) {
        updateWeaponVisuals(*mResourceManager);
      }
    }
  }

  // 2. INICIAR ANIMACIÓN
  float duration = (1.f / getAtkSpeed()) / speedMultiplier;

  // Lógica Dual Wield / GripType
  auto mainHandWeapon = mEquipment[static_cast<int>(EquipmentSlot::MainHand)];
  auto offHandWeapon = mEquipment[static_cast<int>(EquipmentSlot::OffHand)];

  auto isShield = [](const std::shared_ptr<Item>& item) {
    return item && item->isShield();
  };

  bool hasRightWeapon = (mainHandWeapon != nullptr) && !isShield(mainHandWeapon);
  bool hasLeftWeapon = (offHandWeapon != nullptr) && !isShield(offHandWeapon);
  bool isTwoHanded =
      (mainHandWeapon && mainHandWeapon->gripType == GripType::TwoHanded) ||
      (offHandWeapon && offHandWeapon->gripType == GripType::TwoHanded);

  bool attackRight = true;
  bool attackLeft = false;

  if (isTwoHanded) {
    attackRight = true;
    attackLeft = false;
  } else if (hasRightWeapon && !hasLeftWeapon) {
    attackRight = true;
    attackLeft = false;
  } else if (!hasRightWeapon && hasLeftWeapon) {
    attackRight = false;
    attackLeft = true;
  } else if (hasRightWeapon && hasLeftWeapon) {
    attackRight = true;
    attackLeft = true;
  } else {
    if (isShield(offHandWeapon)) {
      attackRight = true;
      attackLeft = false;
    } else if (isShield(mainHandWeapon)) {
      attackRight = false;
      attackLeft = true;
    } else {
      if (mFacingDir == 1) {
        attackRight = true;
        attackLeft = false;
      } else {
        attackRight = false;
        attackLeft = true;
      }
    }
  }

  mSkin.attack(duration * 0.9f, attackLeft, attackRight, isTwoHanded);

  // 3. APLICAR IMPULSO PROCEDIMENTAL DE ATAQUE (ATK LUNGE & TORQUE)
  sf::Vector2f atkDir = { static_cast<float>(mFacingDir), 0.f };
  if (target) {
      sf::Vector2f diff = target->getPosition() - mPos;
      float len = std::sqrt(diff.x * diff.x + diff.y * diff.y);
      if (len > 0.001f) {
          atkDir = diff / len;
      }
  }
  Animation* anim = getAnimation();
  if (anim) {
      float weightFactor = anim->getEquippedWeightFactor();
      anim->applyAttackImpulse(atkDir, weightFactor);
  }
}

void Player::startShieldAttackAnimation(Entity *target, float speedMultiplier) {
  if (speedMultiplier <= 0.f) speedMultiplier = 1.0f;

  // 1. AUTO-AIM (Mirar al objetivo)
  if (target) {
    float dx = target->getPosition().x - mPos.x;
    if (std::abs(dx) > 1.0f) {
      int prevFacing = mFacingDir;
      mFacingDir = (dx > 0) ? 1 : -1;
    }
  }

  // 2. INICIAR ANIMACION CON LA MANO DEL ESCUDO (Escudo Activo E)
  float duration = (1.f / getAtkSpeed()) / speedMultiplier;

  auto mainHandWeapon = mEquipment[static_cast<int>(EquipmentSlot::MainHand)];
  auto offHandWeapon  = mEquipment[static_cast<int>(EquipmentSlot::OffHand)];

  auto isShield = [](const std::shared_ptr<Item>& item) {
    return item && item->isShield();
  };

  bool attackLeft = true;
  if (isShield(mainHandWeapon) && !isShield(offHandWeapon)) {
    attackLeft = false;
  } else {
    attackLeft = true;
  }

  mSkin.shieldAttack(duration * 0.9f, attackLeft);
  if (mResourceManager) {
    updateWeaponVisuals(*mResourceManager);
  }

  // 3. APLICAR IMPULSO PROCEDIMENTAL DE ATAQUE CON ESCUDO (LUNGE PESADO)
  sf::Vector2f atkDir = { static_cast<float>(mFacingDir), 0.f };
  if (target) {
    sf::Vector2f diff = target->getPosition() - mPos;
    float len = std::sqrt(diff.x * diff.x + diff.y * diff.y);
    if (len > 0.001f) {
      atkDir = diff / len;
    }
  }
  Animation* anim = getAnimation();
  if (anim) {
    float weightFactor = anim->getEquippedWeightFactor();
    anim->applyAttackImpulse(atkDir, weightFactor * 1.5f);
  }
}

