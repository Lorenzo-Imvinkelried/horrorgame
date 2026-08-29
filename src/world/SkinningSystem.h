#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <string>

class PassiveMob;
class InventorySystem;
class DamageNumberSystem;
class ParticleSystem;
class ScentSystem;
class Player;

class SkinningSystem {
public:
    SkinningSystem();
    ~SkinningSystem();

    std::string GetPrompt(glm::vec3 playerPos, const std::vector<std::unique_ptr<PassiveMob>>& passiveMobs);

    bool TrySkin(glm::vec3 playerPos, 
                 std::vector<std::unique_ptr<PassiveMob>>& passiveMobs,
                 InventorySystem& inventory,
                 Player& player,
                 DamageNumberSystem& damageNumbers,
                 ParticleSystem& particles,
                 ScentSystem& scentSystem);
};
