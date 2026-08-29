#include "TotemHeal_1.h"
#include "Config.h"
#include "core/engine/ResourceManager.h"
#include "core/systems/ParticleSystem.h"
#include "core/systems/SoundSystem.h"
#include "core/systems/combat/CombatFeedback.h"
#include "entities/Entity.h"
#include "entities/player/Player.h"
#include "utils/Random.h"
#include <cmath>
#include <iostream>
#include <algorithm>

std::vector<TotemInstance> TotemHeal_1::sActiveTotems;
const sf::Texture* TotemHeal_1::sTotemTexture = nullptr;
const sf::Texture* TotemHeal_1::sParticleTexture = nullptr;
std::vector<sf::IntRect> TotemHeal_1::sParticleRects;

void TotemInstance::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    if (!alive || !texture) return;
    sf::Sprite sprite(*texture);
    sprite.setOrigin({9.5f, 46.0f});
    sprite.setPosition(position);
    target.draw(sprite, states);
}

void TotemInstance::getRenderData(std::vector<sf::Vertex>& vertices, const sf::Texture*& outTex) const {
    if (!alive || !texture) {
        outTex = nullptr;
        return;
    }
    outTex = texture;

    float left = position.x - 9.5f;
    float right = position.x + 9.5f;
    float top = position.y - 46.0f;
    float bottom = position.y;

    float u1 = 0.f;
    float u2 = 19.f;
    float v1 = 0.f;
    float v2 = 46.f;

    sf::Color col = sf::Color::White;

    sf::Vector2f tL(left, top);
    sf::Vector2f tR(right, top);
    sf::Vector2f bL(left, bottom);
    sf::Vector2f bR(right, bottom);

    // Tri 1: tL, tR, bL
    vertices.emplace_back(sf::Vertex{tL, col, sf::Vector2f(u1, v1)});
    vertices.emplace_back(sf::Vertex{tR, col, sf::Vector2f(u2, v1)});
    vertices.emplace_back(sf::Vertex{bL, col, sf::Vector2f(u1, v2)});

    // Tri 2: tR, bR, bL
    vertices.emplace_back(sf::Vertex{tR, col, sf::Vector2f(u2, v1)});
    vertices.emplace_back(sf::Vertex{bR, col, sf::Vector2f(u2, v2)});
    vertices.emplace_back(sf::Vertex{bL, col, sf::Vector2f(u1, v2)});
}

void TotemInstance::getShadowRenderData(std::vector<sf::Vertex>& vertices, const sf::Texture*& outTex) const {
    if (!alive || !texture) {
        outTex = nullptr;
        return;
    }
    outTex = texture;

    float left = position.x - 9.5f;
    float right = position.x + 9.5f;
    float top = position.y - 46.0f;
    float bottom = position.y;

    float u1 = 0.f;
    float u2 = 19.f;
    float v1 = 0.f;
    float v2 = 46.f;

    float baseX = position.x;
    float groundY = position.y;

    float shadowScaleY = cfg::Shadow::SCALE_Y;
    float shadowScaleX = cfg::Shadow::SCALE_X;
    float shadowSkewX = cfg::Shadow::SKEW_X;
    float shOffsetX = cfg::Shadow::OFFSET_X;
    float shOffsetY = cfg::Shadow::OFFSET_Y;

    auto projectShadow = [&](sf::Vector2f p) -> sf::Vector2f {
        float relX = p.x - baseX;
        float height = groundY - p.y;
        return {
            baseX + relX * shadowScaleX + shOffsetX + height * shadowSkewX,
            groundY + (p.y - groundY) * shadowScaleY + shOffsetY
        };
    };

    sf::Vector2f s_tL = projectShadow({left, top});
    sf::Vector2f s_tR = projectShadow({right, top});
    sf::Vector2f s_bL = projectShadow({left, bottom});
    sf::Vector2f s_bR = projectShadow({right, bottom});

    sf::Color shadowCol(46, 34, 47, 255);

    // Tri 1: s_tL, s_tR, s_bL
    vertices.emplace_back(sf::Vertex{s_tL, shadowCol, sf::Vector2f(u1, v1)});
    vertices.emplace_back(sf::Vertex{s_tR, shadowCol, sf::Vector2f(u2, v1)});
    vertices.emplace_back(sf::Vertex{s_bL, shadowCol, sf::Vector2f(u1, v2)});

    // Tri 2: s_tR, s_bR, s_bL
    vertices.emplace_back(sf::Vertex{s_tR, shadowCol, sf::Vector2f(u2, v1)});
    vertices.emplace_back(sf::Vertex{s_bR, shadowCol, sf::Vector2f(u2, v2)});
    vertices.emplace_back(sf::Vertex{s_bL, shadowCol, sf::Vector2f(u1, v2)});
}

TotemHeal_1::TotemHeal_1() {
    id = 8;
    name = "Totem Curativo";
    description = "Coloca un tótem en el suelo que emite un aura verde y cura periódicamente a los aliados dentro del área.";
    iconPath = "assets/ui/skills/atlas_skills_18x18x10.png";
    atlasX = 0;
    atlasY = 1;
    cooldown = 10.0f;
    manaCost = 35;
    damageFlat = 30; // Cantidad de curación por tick
    range = 140;     // Radio del aura
    buffDuration = 10.0f; // Duración en el suelo
    type = SkillType::Active;
    targetType = "GROUND";
    defaultSlot = -1;
}

void TotemHeal_1::onCastStart(Entity* caster, Entity* target, ParticleSystem* particles) {
    if (!caster) return;
    caster->startAttackAnimation(nullptr);
}

void TotemHeal_1::onExecute(Entity* caster, Entity* target, ParticleSystem* particles) {
    if (!caster) return;
    sf::Vector2f placePos = mCastTargetPos;
    if (placePos == sf::Vector2f(0.f, 0.f)) {
        placePos = caster->getPosition();
    }

    spawnTotem(placePos, caster, static_cast<float>(range), getEffectiveDamageFlat(), buffDuration, sTotemTexture);

    if (auto* ss = SoundSystem::getInstance()) {
        ss->playSound("assets/sounds/player/skill_buff.wav", 75.f);
    }

    std::cout << "[SKILL] TotemHeal_1 colocado en (" << placePos.x << ", " << placePos.y << ") por " << caster->getName() << "\n";
}

void TotemHeal_1::spawnTotem(sf::Vector2f position, Entity* caster, float radius, int healAmount, float duration, const sf::Texture* tex) {
    TotemInstance totem;
    totem.position = position;
    totem.radius = (radius > 0.f) ? radius : 140.f;
    totem.healAmount = (healAmount > 0) ? healAmount : 30;
    totem.lifetime = (duration > 0.f) ? duration : 10.0f;
    totem.maxLifetime = totem.lifetime;
    totem.tickTimer = 1.0f;
    totem.particleTimer = 0.f;
    totem.alive = true;
    totem.caster = caster;
    totem.texture = tex ? tex : sTotemTexture;

    sActiveTotems.push_back(totem);
}

void TotemHeal_1::updateAll(float dt, Entity* player, CombatFeedback& feedback, ParticleSystem& particleSystem, ResourceManager& res, WorldManager* worldManager) {
    if (!sTotemTexture) {
        try {
            sTotemTexture = &res.getTexture("src/core/skills/active/TotemHeal_1/totem.png");
        } catch (...) {}
    }

    if (!sParticleTexture) {
        try {
            sParticleTexture = &res.getTexture("src/core/skills/active/TotemHeal_1/particulas_verdes.png");
            sParticleRects = {
                sf::IntRect({0, 0}, {2, 2}),
                sf::IntRect({2, 0}, {2, 2}),
                sf::IntRect({0, 2}, {2, 2}),
                sf::IntRect({2, 2}, {2, 2})
            };
        } catch (...) {}
    }

    if (sActiveTotems.empty()) return;

    for (auto& totem : sActiveTotems) {
        if (!totem.texture && sTotemTexture) {
            totem.texture = sTotemTexture;
        }
        if (!totem.alive) continue;

        totem.lifetime -= dt;
        totem.particleTimer += dt;
        totem.tickTimer -= dt;

        // 1. Emisión continua de partículas verdes que flotan hacia arriba desde el suelo
        if (totem.particleTimer >= 0.05f) {
            totem.particleTimer = 0.f;
            if (sParticleTexture && !sParticleRects.empty()) {
                particleSystem.emitRisingParticles(totem.position, totem.radius, 2, sParticleTexture, sParticleRects, 18.f, 38.f, 0.9f, 1.8f);
            }
        }

        // 2. Tick de Curación periódico
        if (totem.tickTimer <= 0.f) {
            totem.tickTimer = 1.0f; // Cada 1 segundo

            if (player && player->isAlive()) {
                sf::Vector2f pPos = player->getPosition();
                float dx = pPos.x - totem.position.x;
                float dy = (pPos.y - totem.position.y) / 0.5f; // Proyección 2.5D (elipse 2:1)
                float distSq = dx * dx + dy * dy;

                if (distSq <= (totem.radius * totem.radius)) {
                    // Sanar al jugador
                    player->heal(totem.healAmount);
                    feedback.onHeal(player, totem.healAmount, totem.caster);

                    // Pequeña explosión de partículas verdes curativas sobre el jugador
                    if (sParticleTexture && !sParticleRects.empty()) {
                        particleSystem.emitRisingParticles(pPos, 25.f, 8, sParticleTexture, sParticleRects, 30.f, 60.f, 0.6f, 1.2f);
                    }

                    if (auto* ss = SoundSystem::getInstance()) {
                        ss->playSound("assets/sounds/player/skill_buff.wav", 50.f);
                    }
                }
            }
        }

        if (totem.lifetime <= 0.f) {
            totem.alive = false;
        }
    }

    // Limpiar tótems expirados
    sActiveTotems.erase(
        std::remove_if(sActiveTotems.begin(), sActiveTotems.end(),
                       [](const TotemInstance& t) { return !t.alive; }),
        sActiveTotems.end()
    );
}

void TotemHeal_1::clearAll() {
    sActiveTotems.clear();
}
