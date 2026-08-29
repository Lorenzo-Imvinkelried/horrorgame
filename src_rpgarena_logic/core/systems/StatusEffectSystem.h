#pragma once
#include <SFML/Graphics.hpp>
#include "entities/Entity.h"
#include "core/managers/StatusEffectManager.h"
#include "core/engine/ResourceManager.h"
#include <vector>

class StatusEffectSystem {
public:
    StatusEffectSystem() = default;
    ~StatusEffectSystem() = default;

    // Dibuja los efectos en fila a partir de startPosition.
    // Retorna el puntero al StatusEffectInfo y su duración restante si el mouse está sobre algún icono.
    const StatusEffectInfo* drawStatusEffects(
        sf::RenderTarget& target,
        const std::vector<Entity::ActiveStatusEffect>& effects,
        sf::Vector2f startPosition,
        sf::Vector2f mousePos,
        ResourceManager& res,
        float& outDuration
    );
};
