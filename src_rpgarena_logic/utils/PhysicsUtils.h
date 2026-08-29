#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath>

class PhysicsUtils {
public:
    // Resuelve colisiones AABB simples (Axis-Aligned Bounding Box) empujando la 'position' fuera de los 'obstacles'.
    // Requiere dimensiones de la caja de colisión (pies) y el offset vertical desde el centro de la entidad.
    static void resolveEnvironmentCollisions(sf::Vector2f& position, 
                                             const std::vector<sf::FloatRect>& obstacles,
                                             float width, 
                                             float height, 
                                             float yOffset) 
    {
        if (obstacles.empty()) return;

        // Definimos la caja inicial basada en la posición actual
        sf::FloatRect bounds(
            {position.x - width * 0.5f, position.y + yOffset - height * 0.5f},
            {width, height}
        );

        for (const auto& wallRect : obstacles) {
            auto intersection = bounds.findIntersection(wallRect);
            
            if (intersection) {
                sf::FloatRect overlap = *intersection;
                float buffer = 0.5f; // Un pequeño buffer para evitar que se quede pegado

                // Resolver en el eje de menor superposición
                if (overlap.size.x < overlap.size.y) {
                    if (bounds.position.x < wallRect.position.x) {
                        position.x -= (overlap.size.x + buffer);
                    } else {
                        position.x += (overlap.size.x + buffer);
                    }
                } else {
                    if (bounds.position.y < wallRect.position.y) {
                        position.y -= (overlap.size.y + buffer);
                    } else {
                        position.y += (overlap.size.y + buffer);
                    }
                }

                // Recalcular la caja de colisión con la nueva posición para la siguiente iteración
                bounds.position = {
                    position.x - width * 0.5f, 
                    position.y + yOffset - height * 0.5f
                };
            }
        }
    }

    static float getDistance(const sf::Vector2f& a, const sf::Vector2f& b) {
        sf::Vector2f diff = a - b;
        return std::sqrt(diff.x * diff.x + diff.y * diff.y);
    }
};
