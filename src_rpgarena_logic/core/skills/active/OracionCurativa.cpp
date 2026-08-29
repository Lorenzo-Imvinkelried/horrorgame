#include "OracionCurativa.h"
#include "core/systems/ParticleSystem.h"
#include "core/systems/SoundSystem.h"
#include "core/systems/combat/CombatFeedback.h"
#include "core/systems/combat/CombatSystem.h"
#include "entities/Entity.h"
#include "entities/player/Player.h"
#include <iostream>

static sf::Texture sParticleTexture;
static bool sParticleLoaded = false;
static std::vector<sf::IntRect> sParticleRects;

static void ensureTextureLoaded() {
  if (sParticleLoaded)
    return;

  const std::string paths[] = {
      "src/core/skills/active/OracionCurativa/particulas_verdes.png",
      "src/core/skills/active/oracinCurativa/particulas_verdes.png",
      "src/core/skills/active/TotemHeal_1/particulas_verdes.png"
  };

  for (const auto &path : paths) {
    if (sParticleTexture.loadFromFile(path)) {
      sParticleTexture.setSmooth(false);
      sParticleLoaded = true;
      sParticleRects = {
          sf::IntRect({0, 0}, {2, 2}),
          sf::IntRect({2, 0}, {2, 2}),
          sf::IntRect({0, 2}, {2, 2}),
          sf::IntRect({2, 2}, {2, 2})
      };
      break;
    }
  }
}

OracionCurativa::OracionCurativa() {
  id = 9;
  name = "Oración Curativa";
  description = "Canaliza una plegaria sagrada durante 2 segundos y cura una "
                "gran cantidad de puntos de vida.";
  iconPath = "assets/ui/skills/atlas_skills_18x18x10.png";
  atlasX = 1 * 18 + 1;
  atlasY = 1 * 18 + 1;
  cooldown = 6.0f;
  manaCost = 30;
  damageFlat = 80;
  range = 0;
  castTime = 2.0f;
  type = SkillType::Active;
  targetType = "SELF";
  defaultSlot = 7;
}

void OracionCurativa::onCastStart(Entity *caster, Entity *target,
                                  ParticleSystem *particles) {
  if (!caster)
    return;

  // Iniciar animación heal.json en la capa de acción
  if (auto *player = dynamic_cast<Player *>(caster)) {
    player->getSkin().playHealAnimation();
  }
}

void OracionCurativa::updateChargeVisuals(Entity *caster, ParticleSystem *particles) {
  if (!caster || !particles)
    return;

  ensureTextureLoaded();
  if (sParticleLoaded) {
    if (auto *anim = caster->getAnimation()) {
      sf::Vector2f handL = anim->getNodePosition("hand_l");
      sf::Vector2f handR = anim->getNodePosition("hand_r");

      // Emitir partículas sutiles únicamente en las manos mientras castea
      if (handL != sf::Vector2f(0.f, 0.f)) {
        particles->emitRisingParticles(handL, 3.f, 1, &sParticleTexture,
                                       sParticleRects, 10.f, 25.f, 0.35f, 0.7f);
      }
      if (handR != sf::Vector2f(0.f, 0.f)) {
        particles->emitRisingParticles(handR, 3.f, 1, &sParticleTexture,
                                       sParticleRects, 10.f, 25.f, 0.35f, 0.7f);
      }
    }
  }
}

void OracionCurativa::onExecute(Entity *caster, Entity *target,
                                ParticleSystem *particles) {
  if (!caster)
    return;

  // Limpiar capa de acción al terminar el casteo
  if (auto *anim = caster->getAnimation()) {
    anim->clearAction();
  }

  // Calcular y aplicar curación
  int healAmount = this->getEffectiveDamageFlat();
  if (healAmount <= 0)
    healAmount = 80;

  caster->heal(healAmount);

  // Texto flotante de curación (verde)
  if (auto *cs = CombatSystem::getInstance()) {
    cs->getFeedback().onHeal(caster, healAmount, caster);
  }

  // Partículas de curación con particulas_verdes.png alrededor del personaje
  if (particles) {
    ensureTextureLoaded();
    if (sParticleLoaded) {
      sf::Vector2f groundCenter = caster->getGroundPosition();
      particles->emitRisingParticles(groundCenter, 26.f, 30, &sParticleTexture,
                                     sParticleRects, 30.f, 70.f, 0.8f, 1.5f);
    } else {
      sf::FloatRect bounds = caster->getGlobalBounds();
      sf::Vector2f center = {bounds.position.x + bounds.size.x * 0.5f,
                             bounds.position.y + bounds.size.y * 0.5f};
      particles->emit(center, 25, sf::Color(80, 255, 120, 220), 45.f, 0.8f);
    }
  }

  // Efecto de sonido
  if (auto *ss = SoundSystem::getInstance()) {
    ss->playSound("assets/sounds/player/level_up.wav", 20.f, 1.35f);
  }

  std::cout << "[SKILL] Oracion Curativa ejecutada exitosamente en "
            << caster->getName() << " (+" << healAmount << " HP)\n";
}
