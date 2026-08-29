#include "ConsumablesSystem.h"
#include "entities/player/Player.h"
#include "core/systems/SoundSystem.h"
#include "core/systems/FXSystem.h"
#include <iostream>

bool ConsumablesSystem::use(Player* player, const std::shared_ptr<Item>& item) {
    if (!player || !player->isAlive() || !item) return false;
    
    // Solo permitir usar pociones (consumibles)
    if (item->type != ItemType::Potion) return false;
    
    bool used = false;
    
    // Curar vida si tiene hpBonus
    if (item->stats.hpBonus > 0) {
        player->heal(item->stats.hpBonus);
        std::cout << "[ConsumablesSystem] Usada pocion de vida: " << item->name 
                  << " (Curado: " << item->stats.hpBonus << " HP)" << std::endl;
                  
        // Generar FX de curacion flotante
        if (auto* fx = FXSystem::getInstance()) {
            fx->addHealNumber(item->stats.hpBonus, player->getHitImpactBounds(), cfg::UI::FloatingText::OFFSET_Y_HEAL, player);
        }
        
        used = true;
    }
    
    // Restaurar mana si tiene mpBonus
    if (item->stats.mpBonus > 0) {
        player->restoreMana(item->stats.mpBonus);
        std::cout << "[ConsumablesSystem] Usada pocion de mana: " << item->name 
                  << " (Restaurado: " << item->stats.mpBonus << " MP)" << std::endl;
                  
        // Generar FX de mana flotante
        if (auto* fx = FXSystem::getInstance()) {
            fx->addManaNumber(item->stats.mpBonus, player->getHitImpactBounds(), cfg::UI::FloatingText::OFFSET_Y_HEAL, player);
        }
        
        used = true;
    }
    
    // Si no tiene hpBonus ni mpBonus pero es de tipo Potion, aun asi la consumimos por defecto
    if (!used) {
        std::cout << "[ConsumablesSystem] Usada pocion sin efectos directos: " << item->name << std::endl;
        used = true;
    }
    
    return used;
}
