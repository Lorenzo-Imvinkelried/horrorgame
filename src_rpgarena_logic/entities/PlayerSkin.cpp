#include "PlayerSkin.h"
#include "Config.h"
#include "core/systems/gore/GoreSystem.h"
#include "core/systems/terrain/TerrainDeformSystem.h"
#include "utils/FastMath.h"
#include <algorithm>
#include <cmath>
#include <iostream>

bool PlayerSkin::loadParts(ResourceManager &res) {
  FastMath::init();

  try {
    if (!mAnim.loadDynamicParts(
            res, "player",
            {"foot_l", "hand_l", "body", "head", "foot_r", "hand_r"})) {
      std::cerr
          << "[PlayerSkin] Error cargando partes dinamicas de Animation.\n";
      return false;
    }

    mIdleClip = res.getAnimationClip("assets/textures/player/idle.json");
    mAnim.setBaseIdleClip(mIdleClip);
    mWalkClip = res.getAnimationClip("assets/textures/player/walk.json");
    mIdleTwoHandedClip =
        res.getAnimationClip("assets/textures/player/idle_2h.json"); // [NEW]
    mWalkTwoHandedClip =
        res.getAnimationClip("assets/textures/player/walk_2h.json"); // [NEW]
    mAttackRClip = res.getAnimationClip("assets/textures/player/attack_r.json");
    mAttackLClip = res.getAnimationClip("assets/textures/player/attack_l.json");
    mAttackDualClip =
        res.getAnimationClip("assets/textures/player/attack_dual.json");
    mAttackTwoHandedClip = res.getAnimationClip(
        "assets/textures/player/attack_2h_v3_2.json"); // [NEW]
    mShieldActiveLeftClip =
        res.getAnimationClip("assets/textures/player/escudo_activo_d_i.json");
    mShieldActiveRightClip =
        res.getAnimationClip("assets/textures/player/escudo_activo_D_d.json");
    mHealClip = res.getAnimationClip("assets/textures/player/heal.json");

    if (mAttackRClip)
      const_cast<AnimationClip *>(mAttackRClip)->isLoop = false;
    if (mAttackLClip)
      const_cast<AnimationClip *>(mAttackLClip)->isLoop = false;
    if (mAttackDualClip)
      const_cast<AnimationClip *>(mAttackDualClip)->isLoop = false;
    if (mAttackTwoHandedClip)
      const_cast<AnimationClip *>(mAttackTwoHandedClip)->isLoop =
          false; // [NEW]
    if (mShieldActiveLeftClip)
      const_cast<AnimationClip *>(mShieldActiveLeftClip)->isLoop = false;
    if (mShieldActiveRightClip)
      const_cast<AnimationClip *>(mShieldActiveRightClip)->isLoop = false;

    // --- [FOOTPRINT] Load custom footprint texture and image ---
    try {
      mFootprintTexture =
          &res.getTexture("assets/textures/player/parts/partes/huella.png");
      mFootprintImageStorage = mFootprintTexture->copyToImage();
      mFootprintImage = &mFootprintImageStorage;
    } catch (...) {
      mFootprintTexture = nullptr;
      mFootprintImage = nullptr;
    }

    return true;
  } catch (...) {
    std::cerr << "[PlayerSkin] Error cargando partes o clips.\n";
    return false;
  }
}

void PlayerSkin::attack(float durationSeconds, bool useLeft, bool useRight,
                        bool isTwoHanded) {
  mIsAttacking = true;
  mAttackDuration = durationSeconds;
  mAttackTimer = 0.f;
  mAttackLeft = useLeft;
  mAttackRight = useRight;
  mAttackTwoHanded = isTwoHanded;

  const AnimationClip *clipToPlay = nullptr;
  if (isTwoHanded) {
    clipToPlay = mAttackTwoHandedClip;
  } else if (useLeft && useRight) {
    clipToPlay = mAttackDualClip;
  } else if (useLeft) {
    clipToPlay = mAttackLClip;
  } else {
    clipToPlay = mAttackRClip;
  }

  if (clipToPlay) {
    float speedMult = (clipToPlay->duration > 0.f && durationSeconds > 0.f)
                          ? (clipToPlay->duration / durationSeconds)
                          : 1.0f;
    mAnim.playAction(clipToPlay, 0.f, speedMult);
  }
}

void PlayerSkin::shieldAttack(float durationSeconds, bool useLeft) {
  mIsAttacking = true;
  mIsShieldAttacking = true;
  mAttackDuration = durationSeconds;
  mAttackTimer = 0.f;
  mAttackLeft = useLeft;
  mAttackRight = !useLeft;
  mAttackTwoHanded = false;

  const AnimationClip *clipToPlay = useLeft ? mAttackLClip : mAttackRClip;

  if (clipToPlay) {
    float speedMult = (clipToPlay->duration > 0.f && durationSeconds > 0.f)
                          ? (clipToPlay->duration / durationSeconds)
                          : 1.0f;
    mAnim.playAction(clipToPlay, 0.f, speedMult);
  }
}

void PlayerSkin::playHealAnimation() {
  if (mHealClip) {
    mAnim.playAction(mHealClip, 0.f, 1.0f);
  }
}

void PlayerSkin::update(sf::Time dt, bool isMoving, sf::Vector2f position,
                        int facingDir, float currentSpeed,
                        const TerrainDeformSystem *terrain) {
  // Estado de ataque sincronizado con el ciclo de vida de la capa de acción
  mIsAttacking = mAnim.hasAction();
  if (!mIsAttacking) {
    mIsShieldAttacking = false;
  }
  if (mIsAttacking) {
    mAttackTimer += dt.asSeconds();
  }

  const AnimationClip *baseClipToPlay = nullptr;
  float speedMult = 1.0f;

  // Resolve active shield guard clip and bone
  const AnimationClip* activeGuardClip = nullptr;
  std::string activeGuardBone = "";
  if (mIsGuardActive && (mHasShieldLeft || mHasShieldRight)) {
    if (mHasShieldLeft && mShieldActiveLeftClip) {
      activeGuardClip = mShieldActiveLeftClip;
      activeGuardBone = "hand_l";
    } else if (mHasShieldRight && mShieldActiveRightClip) {
      activeGuardClip = mShieldActiveRightClip;
      activeGuardBone = "hand_r";
    }
  }
  mAnim.setShieldGuardClip(activeGuardClip, activeGuardBone);

  // La capa Base siempre representa la postura o locomoción (Walk / Idle)
  if (isMoving) {
    baseClipToPlay = (mHasTwoHandedWeapon && mWalkTwoHandedClip)
                         ? mWalkTwoHandedClip
                         : mWalkClip;
    if (baseClipToPlay) {
      float worldStride = mAnim.getWorldStride();
      if (worldStride > 0.1f) {
        // Sincronización perfecta del paso: speedMult = (currentSpeed * duration) / (4 * worldStride)
        speedMult = (currentSpeed * baseClipToPlay->duration) / (4.0f * worldStride);
      } else {
        speedMult = currentSpeed / 150.f;
      }
    }
  } else {
    baseClipToPlay = (mHasTwoHandedWeapon && mIdleTwoHandedClip)
                         ? mIdleTwoHandedClip
                         : mIdleClip;
  }

  // --- GAIT PHASE TRACKING (ALTERNANCIA DE PIES EN PASOS CORTOS / TAPS) ---
  bool stoppedMoving = !isMoving && mWasMoving;

  if (stoppedMoving) {
    // Al soltar el movimiento, calculamos qué pie quedó en el aire
    float currentWalkTime = mAnim.getBaseTimer();
    const AnimationClip *activeBase = mAnim.getBaseClip();
    float dur = (activeBase && activeBase->duration > 0.f) ? activeBase->duration : 1.0f;
    float norm = std::fmod(currentWalkTime, dur) / dur;
    if (norm < 0.f) norm += 1.f;
    
    // [0.0, 0.5): Pie derecho en el aire -> el siguiente paso arranca con el pie izquierdo (0.5 * dur)
    // [0.5, 1.0): Pie izquierdo en el aire -> el siguiente paso arranca con el pie derecho (0.0)
    if (norm < 0.5f) {
      mNextWalkPhase = 0.5f * dur;
    } else {
      mNextWalkPhase = 0.0f;
    }
  } else if (isMoving && (mAnim.getBaseClip() == mWalkClip || mAnim.getBaseClip() == mWalkTwoHandedClip)) {
    // Mientras camina continuamente, actualizamos el próximo pie según la fase actual
    float currentWalkTime = mAnim.getBaseTimer();
    const AnimationClip *activeBase = mAnim.getBaseClip();
    float dur = (activeBase && activeBase->duration > 0.f) ? activeBase->duration : 1.0f;
    float norm = std::fmod(currentWalkTime, dur) / dur;
    if (norm < 0.f) norm += 1.f;
    if (norm < 0.5f) {
      mNextWalkPhase = 0.5f * dur;
    } else {
      mNextWalkPhase = 0.0f;
    }
  }

  mWasMoving = isMoving;

  if (baseClipToPlay && mAnim.getBaseClip() != baseClipToPlay) {
    if (isMoving && (baseClipToPlay == mWalkClip || baseClipToPlay == mWalkTwoHandedClip)) {
      // Iniciar caminata desde la fase del pie que corresponde con blend rápido de 0.08s
      float dur = (baseClipToPlay->duration > 0.f) ? baseClipToPlay->duration : 1.0f;
      float startT = std::fmod(mNextWalkPhase, dur);
      if (startT < 0.f) startT += dur;
      mAnim.playBase(baseClipToPlay, startT, 0.08f);

      // Alternar para el siguiente paso
      if (startT < 0.5f * dur) {
        mNextWalkPhase = 0.5f * dur;
      } else {
        mNextWalkPhase = 0.0f;
      }
    } else if (!isMoving && (baseClipToPlay == mIdleClip || baseClipToPlay == mIdleTwoHandedClip || baseClipToPlay == mShieldActiveLeftClip || baseClipToPlay == mShieldActiveRightClip)) {
      // Al pasar a Idle o Guard: blend rápido de 0.08s
      mAnim.playBase(baseClipToPlay, 0.f, 0.08f);
    } else {
      mAnim.playBase(baseClipToPlay);
    }
  }

  float S = cfg::Player::SCALE_Y;
  mAnim.setScale({S, S});

  // Actualizar Animation (que internamente calcula el IK y aplica física de
  // terreno en cascada)
  mAnim.update(dt, isMoving, position, facingDir, speedMult, terrain);

  // [TERRAIN DEFORM] Edge-detection de pisada basado en la altura vertical
  // efectiva (lift)
  {
    float kLiftThreshold = cfg::Terrain::FOOTPRINT_LIFT_THRESHOLD;

    float restYL = (facingDir == -1) ? mAnim.getNodeCustomRestY("foot_r") : mAnim.getNodeCustomRestY("foot_l");
    float restYR = (facingDir == -1) ? mAnim.getNodeCustomRestY("foot_l") : mAnim.getNodeCustomRestY("foot_r");

    float liftL = restYL - mAnim.getNodeCurrentY("foot_l");
    float liftR = restYR - mAnim.getNodeCurrentY("foot_r");

    float effectiveLiftL = liftL * S;
    float effectiveLiftR = liftR * S;

    bool leftAirborne = (effectiveLiftL > kLiftThreshold);
    bool rightAirborne = (effectiveLiftR > kLiftThreshold);

    mLeftFootDown = mPrevLeftAirborne && !leftAirborne;
    mRightFootDown = mPrevRightAirborne && !rightAirborne;

    mPrevLeftAirborne = leftAirborne;
    mPrevRightAirborne = rightAirborne;
  }

  // Capturar transform exacto en el frame de aterrizaje
  if (mLeftFootDown) {
    mLandedLeftPos = mAnim.getNodePosition("foot_l");
    mLandedLeftRot = mAnim.getNodeRotation("foot_l") * -facingDir;
    mLandedLeftScale = {mAnim.getNodeScale("foot_l").x * S * -facingDir,
                        mAnim.getNodeScale("foot_l").y * S};
    auto bounds = mAnim.getNodeLocalBounds("foot_l");
    mLandedLeftOrigin = {bounds.size.x * 0.5f, bounds.size.y * 0.5f};
  }
  if (mRightFootDown) {
    mLandedRightPos = mAnim.getNodePosition("foot_r");
    mLandedRightRot = mAnim.getNodeRotation("foot_r") * -facingDir;
    mLandedRightScale = {mAnim.getNodeScale("foot_r").x * S * -facingDir,
                         mAnim.getNodeScale("foot_r").y * S};
    auto bounds = mAnim.getNodeLocalBounds("foot_r");
    mLandedRightOrigin = {bounds.size.x * 0.5f, bounds.size.y * 0.5f};
  }
}

void PlayerSkin::draw(sf::RenderTarget &target, sf::RenderStates states) {
  mAnim.draw(target, states);
}

void PlayerSkin::drawLayer(sf::RenderTarget &target, int layer,
                           sf::RenderStates states) {
  if (layer == 0) {
    mAnim.draw(target, states);
  } else {
    mAnim.drawLayer(target, layer, states);
  }
}

void PlayerSkin::setWeaponVisuals(const sf::Texture *baseTex,
                                  const sf::Texture *layoutTex,
                                  const sf::IntRect &baseRect,
                                  const sf::IntRect &overlayRect,
                                  ItemQuality quality, float scale,
                                  sf::Vector2f offset, bool isTwoHanded,
                                  int fortificationLevel, bool isShield, bool shieldOverHand) {
  mHasTwoHandedWeapon = (baseTex != nullptr) && isTwoHanded;
  mAnim.setWeaponVisuals(baseTex, layoutTex, baseRect, overlayRect, quality,
                         offset, isTwoHanded, fortificationLevel, isShield, shieldOverHand);
}

void PlayerSkin::setSecondaryWeaponVisuals(
    const sf::Texture *baseTex, const sf::Texture *layoutTex,
    const sf::IntRect &baseRect, const sf::IntRect &overlayRect,
    ItemQuality quality, float scale, sf::Vector2f offset,
    int fortificationLevel, bool isShield, bool shieldOverHand) {
  mAnim.setSecondaryWeaponVisuals(baseTex, layoutTex, baseRect, overlayRect,
                                  quality, offset, fortificationLevel,
                                  isShield, shieldOverHand);
}

void PlayerSkin::setArmorVisuals(EquipmentSlot slot, const sf::Texture *tex,
                                 const sf::IntRect &rect, sf::Vector2f offset,
                                 float scale, int fortificationLevel) {
  mAnim.setArmorVisuals(slot, tex, rect, offset, scale, fortificationLevel);
}

void PlayerSkin::getRenderData(std::vector<sf::Vertex> &vertices,
                               const sf::Texture *&texture) const {
  mAnim.getRenderData(vertices, texture);
}

void PlayerSkin::getShadowRenderData(std::vector<sf::Vertex> &vertices,
                                     const sf::Texture *&texture) const {
  mAnim.getShadowRenderData(vertices, texture);
}

void PlayerSkin::getWeaponShadowRenderData(std::vector<sf::Vertex> &vertices,
                                           const sf::Texture *&texture,
                                           int slotIndex) const {
  mAnim.getWeaponShadowRenderData(vertices, texture, slotIndex);
}

void PlayerSkin::getArmorShadowRenderData(std::vector<sf::Vertex> &vertices,
                                          const sf::Texture *&texture,
                                          int slotIndex) const {
  mAnim.getArmorShadowRenderData(vertices, texture, slotIndex);
}

void PlayerSkin::emitGibs(
    GoreSystem &gore, float floorY, sf::Vector2f sourcePos,
    float forceMultiplier, sf::Vector2f initialVelocity,
    const std::vector<std::shared_ptr<Item>> &armorItems,
    float deathSortY) const {
  const sf::Texture *tex = mAnim.getAtlasTexture();
  if (!tex)
    return;
  std::vector<sf::Vertex> allVerts;
  std::vector<std::string> nodeNames;
  for (const auto &node : mAnim.getNodes()) {
    auto vertices = mAnim.getNodeVertices(node.name);
    if (!vertices.empty()) {
      allVerts.insert(allVerts.end(), vertices.begin(), vertices.end());
      nodeNames.push_back(node.name);
    }
  }
  if (!allVerts.empty()) {
    gore.emitGibs(allVerts, tex, floorY, sourcePos, forceMultiplier, nodeNames,
                  initialVelocity, &mAnim, armorItems, "", deathSortY);
  }
}

void PlayerSkin::emitWeaponGibs(GoreSystem &gore, float floorY,
                                sf::Vector2f sourcePos,
                                float forceMultiplier) const {
  mAnim.emitWeaponGibs(gore, floorY, sourcePos, forceMultiplier);
}

bool PlayerSkin::morph(const std::string &mobType, ResourceManager &res) {
  try {
    const SkeletonData *sk =
        res.getSkeleton("assets/textures/mobs/" + mobType + "/esqueleto.json");
    std::vector<std::string> parts;
    if (sk && !sk->parts.empty()) {
      parts = sk->parts;
    } else {
      parts = {"foot_l", "hand_l", "body", "head", "foot_r", "hand_r"};
    }

    if (!mAnim.loadDynamicParts(res, mobType, parts)) {
      std::cerr << "[PlayerSkin] Error morphing: loadDynamicParts failed.\n";
      return false;
    }

    if (sk) {
      mAnim.loadSkeleton(res,
                         "assets/textures/mobs/" + mobType + "/esqueleto.json");
    } else {
      // Fallback default offsets
      mAnim.setCustomRestOffsets({-10.f, -40.f}, {-12.f, -5.f}, {12.f, 5.f},
                                 {-6.f, 25.f}, {6.f, 35.f});
    }

    mIdleClip =
        res.getAnimationClip("assets/textures/mobs/" + mobType + "/idle.json");
    mWalkClip =
        res.getAnimationClip("assets/textures/mobs/" + mobType + "/walk.json");
    mIdleTwoHandedClip = res.getAnimationClip("assets/textures/mobs/" +
                                              mobType + "/idle_2h.json");
    mWalkTwoHandedClip = res.getAnimationClip("assets/textures/mobs/" +
                                              mobType + "/walk_2h.json");
    mAttackRClip = res.getAnimationClip("assets/textures/mobs/" + mobType +
                                        "/attack.json");
    mAttackLClip = mAttackRClip;
    mAttackDualClip = mAttackRClip;
    mAttackTwoHandedClip = res.getAnimationClip("assets/textures/mobs/" +
                                                mobType + "/attack_2h.json");
    if (!mAttackTwoHandedClip)
      mAttackTwoHandedClip = mAttackRClip;

    if (mAttackRClip)
      const_cast<AnimationClip *>(mAttackRClip)->isLoop = false;
    if (mAttackTwoHandedClip)
      const_cast<AnimationClip *>(mAttackTwoHandedClip)->isLoop = false;

    return true;
  } catch (...) {
    std::cerr << "[PlayerSkin] Morph failed.\n";
    return false;
  }
}

bool PlayerSkin::revert(ResourceManager &res) {
  bool success = loadParts(res);
  if (!mAnim.loadSkeleton(res, "assets/textures/player/esqueleto.json")) {
    mAnim.setCustomRestOffsets(
        cfg::Player::HEAD_OFFSET, cfg::Player::HAND_L_OFFSET,
        cfg::Player::HAND_R_OFFSET, cfg::Player::FOOT_L_OFFSET,
        cfg::Player::FOOT_R_OFFSET);
  }
  return success;
}
