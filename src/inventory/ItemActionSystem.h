#pragma once

#include "ItemInstance.h"
#include <string>

class Player;
class ParticleSystem;
class DamageNumberSystem;

struct ItemActionContext {
    Player* player = nullptr;
    ParticleSystem* particles = nullptr;
    DamageNumberSystem* damageNumbers = nullptr;
};

struct ItemActionResult {
    bool success = false;
    bool consumeItem = false;
    std::string feedbackMessage = "";
};

/**
 * @brief ItemActionSystem: Ejecutor desacoplado de efectos de gameplay para objetos consumibles.
 * El inventario delega el uso aquí sin necesitar conocer detalles de Player ni de Combate.
 */
class ItemActionSystem {
public:
    static ItemActionResult ExecuteUse(const ItemInstance& instance, ItemActionContext& context);
};
