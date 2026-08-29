#include "basic-orb.h"
#include "core/engine/ResourceManager.h"
#include "core/managers/EntityManager.h"
#include "core/systems/ParticleSystem.h"
#include "core/systems/SoundSystem.h"
#include "core/systems/WorldManager.h"
#include "core/systems/combat/CombatFeedback.h"
#include "core/systems/combat/CombatSystem.h"
#include "entities/Entity.h"
#include "entities/player/Player.h"
#include "utils/CombatCalculator.h"
#include "utils/Random.h"
#include <cmath>
#include <iostream>

std::vector<OrbProjectile> BasicOrb::sActiveOrbs;
const sf::Texture *BasicOrb::sOrbTexture = nullptr;

void OrbProjectile::draw(sf::RenderTarget &target,
                         sf::RenderStates states) const {
  if (!alive || !texture)
    return;
  sf::Sprite orbSprite(*texture);
  sf::Vector2u texSize = texture->getSize();
  orbSprite.setOrigin({texSize.x * 0.5f, texSize.y * 0.5f});
  orbSprite.setPosition(position);
  orbSprite.setScale({1.f, 1.f});
  orbSprite.setRotation(sf::degrees(animTimer * 180.f));
  target.draw(orbSprite, states);
}

static sf::Texture sOrbParticleTexture;
static bool sOrbParticleLoaded = false;
static std::vector<sf::IntRect> sOrbParticleRects;

static void ensureOrbParticleLoaded() {
  if (sOrbParticleLoaded)
    return;

  if (sOrbParticleTexture.loadFromFile("src/core/skills/mage/particles_orb.png")) {
    sOrbParticleTexture.setSmooth(false);
    sOrbParticleLoaded = true;
    sf::Vector2u size = sOrbParticleTexture.getSize();
    if (size.x >= 3) {
      sOrbParticleRects = {sf::IntRect({0, 0}, {1, 1}), sf::IntRect({1, 0}, {1, 1}),
                           sf::IntRect({2, 0}, {1, 1})};
    } else if (size.y >= 3) {
      sOrbParticleRects = {sf::IntRect({0, 0}, {1, 1}), sf::IntRect({0, 1}, {1, 1}),
                           sf::IntRect({0, 2}, {1, 1})};
    } else {
      sOrbParticleRects = {sf::IntRect({0, 0}, {1, 1})};
    }
  }
}

BasicOrb::BasicOrb() {
  id = 7;
  name = "Orbe Básico";
  description =
      "Canaliza y lanza un orbe mágico en línea recta que inflige daño al chocar.";
  cooldown = 1.5f;
  manaCost = 25;
  damageFlat = 60;
  range = 600;
  castTime = 1.0f;
  type = SkillType::Active;
  targetType = "DIRECTION";
  defaultSlot = 0;
}

void BasicOrb::onCastStart(Entity *caster, Entity *target,
                           ParticleSystem *particles) {
  if (!caster)
    return;

  // Face towards cast direction if target pos was set
  if (mCastTargetPos != sf::Vector2f(0.f, 0.f)) {
    float dx = mCastTargetPos.x - caster->getPosition().x;
    if (std::abs(dx) > 1.f) {
      caster->setFacingDir(dx > 0.f ? 1 : -1);
    }
  }

  // Iniciar animación heal.json (brazos en alto / canalización mágica) en la capa de acción
  if (auto *player = dynamic_cast<Player *>(caster)) {
    player->getSkin().playHealAnimation();
  }
}

void BasicOrb::updateChargeVisuals(Entity *caster, ParticleSystem *particles) {
  if (!caster || !particles)
    return;

  ensureOrbParticleLoaded();
  if (sOrbParticleLoaded) {
    if (auto *anim = caster->getAnimation()) {
      sf::Vector2f handL = anim->getNodePosition("hand_l");
      sf::Vector2f handR = anim->getNodePosition("hand_r");

      if (handL != sf::Vector2f(0.f, 0.f)) {
        particles->emitOrbParticles(handL, 2, &sOrbParticleTexture, sOrbParticleRects,
                                    10.f, 30.f, 0.2f, 0.45f, 4.f);
      }
      if (handR != sf::Vector2f(0.f, 0.f)) {
        particles->emitOrbParticles(handR, 2, &sOrbParticleTexture, sOrbParticleRects,
                                    10.f, 30.f, 0.2f, 0.45f, 4.f);
      }
    }
  }
}

void BasicOrb::onExecute(Entity *caster, Entity *target,
                         ParticleSystem *particles) {
  if (!caster)
    return;

  // Limpiar capa de acción de casteo al completarse
  if (auto *anim = caster->getAnimation()) {
    anim->clearAction();
  }

  sf::Vector2f startPos = caster->getPosition();
  startPos.y -= 15.f; // Height offset for hands/chest

  sf::Vector2f targetPos = mCastTargetPos;
  if (targetPos == sf::Vector2f(0.f, 0.f)) {
    if (target && target != caster) {
      targetPos = target->getPosition();
    } else {
      targetPos =
          startPos +
          sf::Vector2f(static_cast<float>(caster->getFacingDir()) * 200.f, 0.f);
    }
  }

  sf::Vector2f dir = targetPos - startPos;
  float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
  if (len > 0.001f) {
    dir /= len;
  } else {
    dir = sf::Vector2f(static_cast<float>(caster->getFacingDir()), 0.f);
  }

  float speed = 480.f;
  int dmg = caster->getAttack() + getEffectiveDamageFlat();

  OrbProjectile orb;
  orb.position = startPos;
  orb.velocity = dir * speed;
  orb.distanceTraveled = 0.f;
  orb.maxDistance = (range > 0) ? static_cast<float>(range) : 600.f;
  orb.hitRadius = 18.f;
  orb.damage = dmg;
  orb.caster = caster;
  orb.alive = true;
  orb.animTimer = 0.f;
  orb.texture = sOrbTexture;

  sActiveOrbs.push_back(orb);

  // Play Sound Effect if available
  if (auto *ss = SoundSystem::getInstance()) {
    ss->playSound("assets/sounds/player/power_strike.wav", 60.f);
  }

  std::cout << "[SKILL] BasicOrb (Orbe Básico) launched by "
            << caster->getName() << "\n";
}

void BasicOrb::updateAll(float dt, EntityManager &entityManager,
                         CombatSystem &combatSystem,
                         ParticleSystem &particleSystem, ResourceManager &res,
                         WorldManager *worldManager) {
  if (!sOrbTexture) {
    try {
      sOrbTexture = &res.getTexture("src/core/skills/mage/orb.png");
    } catch (...) {
    }
  }

  if (sActiveOrbs.empty())
    return;

  // Load orb particles texture rects safely
  const sf::Texture *partTex = nullptr;
  std::vector<sf::IntRect> rects;
  try {
    sf::Texture &tex = res.getTexture("src/core/skills/mage/particles_orb.png");
    partTex = &tex;
    sf::Vector2u size = tex.getSize();
    if (size.x >= 3) {
      rects = {sf::IntRect({0, 0}, {1, 1}), sf::IntRect({1, 0}, {1, 1}),
               sf::IntRect({2, 0}, {1, 1})};
    } else if (size.y >= 3) {
      rects = {sf::IntRect({0, 0}, {1, 1}), sf::IntRect({0, 1}, {1, 1}),
               sf::IntRect({0, 2}, {1, 1})};
    } else {
      rects = {sf::IntRect({0, 0}, {1, 1})};
    }
  } catch (...) {
  }

  for (auto &orb : sActiveOrbs) {
    if (!orb.texture && sOrbTexture) {
      orb.texture = sOrbTexture;
    }
    if (!orb.alive)
      continue;

    // Emit launch particles burst on first frame ("a la salida")
    if (orb.distanceTraveled == 0.f && partTex) {
      particleSystem.emitOrbParticles(orb.position, 18, partTex, rects, 30.f,
                                      100.f, 0.2f, 0.5f, 12.f);
    }

    // Movement
    float speed = std::sqrt(orb.velocity.x * orb.velocity.x +
                            orb.velocity.y * orb.velocity.y);
    sf::Vector2f moveStep = orb.velocity * dt;
    orb.position += moveStep;
    orb.distanceTraveled += speed * dt;
    orb.animTimer += dt;

    // Continuous trail particles while moving ("a medida que se mueve")
    if (partTex) {
      particleSystem.emitOrbParticles(orb.position, 3, partTex, rects, 10.f,
                                      40.f, 0.15f, 0.35f, 8.f,
                                      -orb.velocity * 0.1f);
    }

    // 1. Collision detection against alive entities (excluding caster)
    auto candidates = entityManager.querySpatialGrid(orb.position, 64.f);
    for (auto *entity : candidates) {
      if (!entity || !entity->isAlive() || entity == orb.caster)
        continue;

      float dx = entity->getPosition().x - orb.position.x;
      float dy = entity->getPosition().y - orb.position.y;
      float distSq = dx * dx + dy * dy;
      float combinedRadius = orb.hitRadius + 14.f;

      if (distSq <= combinedRadius * combinedRadius) {
        // HIT! Apply damage via CombatCalculator
        float inputRaw = static_cast<float>(orb.damage);
        auto resDamage = CombatCalculator::calculateSkillDamage(
            orb.caster, entity, inputRaw, true);

        entity->takeDamage(resDamage.totalDamage, orb.caster, resDamage.isCrit);
        if (!resDamage.isBlocked && resDamage.totalDamage > 0 && orb.caster) {
          entity->triggerHitEffect(orb.caster->getPosition());
        }
        combatSystem.getFeedback().onHit(entity, resDamage.totalDamage,
                                         resDamage.isCrit, false,
                                         resDamage.isBlocked);

        // Impact particle explosion ("al chocar")
        if (partTex) {
          particleSystem.emitOrbParticles(orb.position, 28, partTex, rects,
                                          60.f, 180.f, 0.25f, 0.6f, 16.f);
        }

        orb.alive = false;
        break;
      }
    }

    // 2. Collision detection against environment obstacles (trees / decor
    // trunks)
    if (orb.alive && worldManager) {
      static std::vector<sf::FloatRect> sObstacleCache;
      worldManager->getDecorSystem().getObstaclesNearby(orb.position,
                                                        sObstacleCache);
      for (const auto &trunk : sObstacleCache) {
        // Trunk collision box covering the solid tree trunk column
        sf::FloatRect trunkBox(
            {trunk.position.x - 4.f, trunk.position.y - 45.f},
            {trunk.size.x + 8.f, trunk.size.y + 45.f});

        float closeX = std::clamp(orb.position.x, trunkBox.position.x,
                                  trunkBox.position.x + trunkBox.size.x);
        float closeY = std::clamp(orb.position.y, trunkBox.position.y,
                                  trunkBox.position.y + trunkBox.size.y);
        float dx = orb.position.x - closeX;
        float dy = orb.position.y - closeY;
        if ((dx * dx + dy * dy) <=
            (orb.hitRadius * orb.hitRadius * 0.7f * 0.7f)) {
          // Tree impact! Explode with particles and sound
          if (partTex) {
            particleSystem.emitOrbParticles(orb.position, 26, partTex, rects,
                                            50.f, 160.f, 0.25f, 0.55f, 14.f);
          }
          if (auto *ss = SoundSystem::getInstance()) {
            ss->playSound("assets/sounds/player/power_strike.wav", 40.f);
          }
          orb.alive = false;
          break;
        }
      }
    }

    // Max range check
    if (orb.alive && orb.distanceTraveled >= orb.maxDistance) {
      if (partTex) {
        particleSystem.emitOrbParticles(orb.position, 15, partTex, rects, 30.f,
                                        90.f, 0.2f, 0.4f, 10.f);
      }
      orb.alive = false;
    }
  }

  // Clean up dead orbs
  sActiveOrbs.erase(
      std::remove_if(sActiveOrbs.begin(), sActiveOrbs.end(),
                     [](const OrbProjectile &o) { return !o.alive; }),
      sActiveOrbs.end());
}

void BasicOrb::clearAll() { sActiveOrbs.clear(); }
