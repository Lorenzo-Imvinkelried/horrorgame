#include "Goblin.h"
#include <iostream>

void Goblin::updateAI(sf::Time dt) {
    // Ejemplo de personalización futura:
    // if (mCurrentHp < getMaxHp() * 0.3f) {
    //     // Lógica especial de pánico/huida
    // }

    // Por defecto, ejecuta la lógica estándar de persecución y combate de Mob
    Mob::updateAI(dt);
}
