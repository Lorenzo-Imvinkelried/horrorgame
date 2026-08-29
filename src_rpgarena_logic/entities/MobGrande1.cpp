#include "MobGrande1.h"
#include <iostream>

void MobGrande1::updateAI(sf::Time dt) {
    // Ejemplo de personalización futura:
    // Por ejemplo, los gigantes podrían ignorar el retroceso o hacer temblar el suelo
    
    // Por defecto, ejecuta la lógica estándar de persecución y combate de Mob
    Mob::updateAI(dt);
}
