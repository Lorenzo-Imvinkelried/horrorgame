#pragma once
#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>
#include "Config.h"

class WindSystem {
public:
    static WindSystem& get() {
        static WindSystem instance;
        return instance;
    }

    WindSystem() = default;

    void update(sf::Time dt);

    // Calculates the horizontal pixel offset for a tree/foliage vertex
    // worldX, worldY: World position of the tree base or vertex
    // heightFraction: 0.0 at trunk base/roots (no motion), 1.0 at highest point of canopy
    float getWindOffset(float worldX, float worldY, float heightFraction = 1.0f) const;

    sf::Vector2f getWindDisplacement(sf::Vector2f worldPos, float heightFraction = 1.0f) const {
        return { getWindOffset(worldPos.x, worldPos.y, heightFraction), 0.0f };
    }

    float getTime() const { return mTime; }
    void setTime(float time) { mTime = time; }

private:
    float mTime = 0.0f;
};
