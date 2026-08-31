#include "ItemActionSystem.h"
#include "ItemRegistry.h"
#include "Player.h"
#include "combat/DamageNumberSystem.h"
#include "ParticleSystem.h"
#include <algorithm>

ItemActionResult ItemActionSystem::ExecuteUse(const ItemInstance& instance, ItemActionContext& context) {
    ItemActionResult res;
    if (!instance.IsValid() || context.player == nullptr) {
        res.success = false;
        return res;
    }

    const ItemDefinition& def = ItemRegistry::Get().Get(instance.id);
    if (!def.isUsable || def.effects.empty()) {
        res.success = false;
        res.feedbackMessage = "Este objeto no se puede consumir.";
        return res;
    }

    bool effectApplied = false;
    Player& player = *context.player;

    for (const auto& effect : def.effects) {
        switch (effect.type) {
            case ItemEffectType::RESTORE_HP: {
                if (player.Stats.CurrentHP < player.Stats.MaxHP) {
                    int healAmount = static_cast<int>(effect.magnitude);
                    int oldHP = player.Stats.CurrentHP;
                    player.Stats.CurrentHP = std::min(player.Stats.MaxHP, player.Stats.CurrentHP + healAmount);
                    int actualHealed = player.Stats.CurrentHP - oldHP;

                    if (context.damageNumbers) {
                        context.damageNumbers->SpawnDamage(player.Position + glm::vec3(0, 1.8f, 0), actualHealed, true);
                    }
                    if (context.particles) {
                        for (int i = 0; i < 16; ++i) {
                            glm::vec3 pVel((rand()%100/50.0f - 1.0f)*1.5f, (rand()%100/50.0f + 0.3f)*2.0f, (rand()%100/50.0f - 1.0f)*1.5f);
                            context.particles->SpawnParticle(player.Position + glm::vec3(0, 1.0f, 0), pVel, glm::vec4(0.15f, 0.95f, 0.25f, 1.0f), 0.14f, 0.8f, -4.0f);
                        }
                    }
                    effectApplied = true;
                }
                break;
            }

            case ItemEffectType::RESTORE_MP: {
                if (player.Stats.CurrentMP < player.Stats.MaxMP) {
                    int manaAmount = static_cast<int>(effect.magnitude);
                    player.Stats.CurrentMP = std::min(player.Stats.MaxMP, player.Stats.CurrentMP + manaAmount);

                    if (context.damageNumbers) {
                        context.damageNumbers->SpawnDamage(player.Position + glm::vec3(0, 1.8f, 0), manaAmount, false);
                    }
                    if (context.particles) {
                        for (int i = 0; i < 16; ++i) {
                            glm::vec3 pVel((rand()%100/50.0f - 1.0f)*1.5f, (rand()%100/50.0f + 0.3f)*2.0f, (rand()%100/50.0f - 1.0f)*1.5f);
                            context.particles->SpawnParticle(player.Position + glm::vec3(0, 1.0f, 0), pVel, glm::vec4(0.25f, 0.45f, 1.0f, 1.0f), 0.14f, 0.8f, -4.0f);
                        }
                    }
                    effectApplied = true;
                }
                break;
            }

            case ItemEffectType::GRANT_EXP: {
                int expAmount = static_cast<int>(effect.magnitude);
                bool lvlUp = false;
                player.Stats.AddExp(expAmount, lvlUp);

                if (context.damageNumbers) {
                    context.damageNumbers->SpawnExp(player.Position + glm::vec3(0, 1.8f, 0), expAmount);
                    if (lvlUp) {
                        context.damageNumbers->SpawnLevelUp(player.Position);
                    }
                }
                effectApplied = true;
                break;
            }

            case ItemEffectType::BUFF_ATTACK: {
                // Efecto de furia
                if (context.particles) {
                    for (int i = 0; i < 20; ++i) {
                        glm::vec3 pVel((rand()%100/50.0f - 1.0f)*2.0f, (rand()%100/50.0f + 0.5f)*2.5f, (rand()%100/50.0f - 1.0f)*2.0f);
                        context.particles->SpawnParticle(player.Position + glm::vec3(0, 1.0f, 0), pVel, glm::vec4(0.95f, 0.15f, 0.15f, 1.0f), 0.15f, 0.9f, -6.0f);
                    }
                }
                effectApplied = true;
                break;
            }

            default:
                break;
        }
    }

    if (effectApplied) {
        res.success = true;
        res.consumeItem = true;
        res.feedbackMessage = "Usado: " + def.name;
    } else {
        res.success = false;
        res.consumeItem = false;
        res.feedbackMessage = "Tu salud/recurso ya se encuentra al maximo.";
    }

    return res;
}
