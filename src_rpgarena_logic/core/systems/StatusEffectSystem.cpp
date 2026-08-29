#include "StatusEffectSystem.h"
#include "Config.h"
#include "core/graphics/BitmapText.h"
#include <cmath>
#include <iostream>

const StatusEffectInfo* StatusEffectSystem::drawStatusEffects(
    sf::RenderTarget& target,
    const std::vector<Entity::ActiveStatusEffect>& effects,
    sf::Vector2f startPosition,
    sf::Vector2f mousePos,
    ResourceManager& res,
    float& outDuration
) {
    if (effects.empty()) return nullptr;

    const float zoom = cfg::Map::ZOOM_FACTOR;
    const float iconSize = 9.f * zoom;
    const float spacing = 3.f * zoom; // Espaciado entre cuadritos

    // Cargar la textura del atlas
    sf::Texture& atlas = res.getTexture("assets/ui/status_effect/atlas_status_effects.png");
    sf::Sprite sprite(atlas);

    const StatusEffectInfo* hoveredInfo = nullptr;

    for (size_t i = 0; i < effects.size(); ++i) {
        const auto& effect = effects[i];
        const StatusEffectInfo* info = StatusEffectManager::getInstance().getEffectInfo(effect.id);
        if (!info) continue;

        sf::Vector2f currentPos(
            startPosition.x + i * (iconSize + spacing),
            startPosition.y
        );

        // 1. Dibujar el fondo del icono
        sf::RectangleShape border({iconSize, iconSize});
        border.setPosition(currentPos);
        border.setFillColor(sf::Color(0, 0, 0, 180)); // Fondo negro semi-transparente
        target.draw(border);

        // 2. Configurar el sprite del atlas status effect
        // El atlas tiene iconos de 9x9 con 1px de separación entre cuadradito (10px por ranura)
        sprite.setTextureRect(sf::IntRect({info->atlasX * 10, info->atlasY * 10}, {9, 9}));
        sprite.setPosition(currentPos);
        sprite.setScale({zoom, zoom});

        target.draw(sprite);

        // [STACKS OVERLAY] Dibujar número de stacks a partir de 2 y hasta 99
        if (effect.stacks >= 2) {
            int displayStacks = std::min(99, effect.stacks);
            try {
                sf::Texture& fontTex = res.getTexture("assets/fonts/font.png");
                BitmapText stackText;
                stackText.setTexture(&fontTex);
                stackText.setString(std::to_string(displayStacks));
                stackText.setColor(sf::Color::White);
                stackText.setScale({zoom, zoom});

                float tw = stackText.getWidth() * zoom;
                float th = 5.f * zoom;

                float posX = currentPos.x + iconSize - tw + 1.0f * zoom;
                float posY = currentPos.y + iconSize - th;

                stackText.setPosition({posX, posY});
                target.draw(stackText);
            } catch (...) {}
        }

        // 3. Revisar colisión con el cursor (hover)
        sf::FloatRect bounds(currentPos, {iconSize, iconSize});
        if (bounds.contains(mousePos)) {
            hoveredInfo = info;
            outDuration = effect.remainingDuration;
        }
    }

    return hoveredInfo;
}
