#include "WindSystem.h"
#include <cmath>
#include <algorithm>

void WindSystem::update(sf::Time dt) {
    if (!cfg::Wind::ENABLE) return;
    mTime += dt.asSeconds();
    if (mTime > 100000.f) {
        mTime = std::fmod(mTime, 100000.f);
    }
}

float WindSystem::getWindOffset(float worldX, float worldY, float heightFraction) const {
    if (!cfg::Wind::ENABLE || heightFraction <= 0.001f) {
        return 0.0f;
    }

    // 1. Height Attenuation (Trunk is 100% frozen, canopy curves smoothly)
    float trunkThresh = std::clamp(cfg::Wind::TRUNK_THRESHOLD, 0.0f, 0.95f);
    if (heightFraction <= trunkThresh) {
        return 0.0f;
    }

    float canopyFraction = (heightFraction - trunkThresh) / (1.0f - trunkThresh);

    // 2. Dual Height Curves:
    // - macroWeight: gentle curve for whole canopy/branch sway
    // - microWeight: steep curve (exponent * micro_curve_multiplier) so micro-turbulence flutter only affects the uppermost leaves
    float exponent = (cfg::Wind::CANOPY_CURVE_EXPONENT > 0.0f) ? cfg::Wind::CANOPY_CURVE_EXPONENT : 1.5f;
    float microMultiplier = (cfg::Wind::MICRO_CURVE_MULTIPLIER > 0.0f) ? cfg::Wind::MICRO_CURVE_MULTIPLIER : 2.0f;
    float macroWeight = std::pow(canopyFraction, exponent);
    float microWeight = std::pow(canopyFraction, exponent * microMultiplier);

    float t = mTime * cfg::Wind::SPEED;
    float freq = cfg::Wind::FREQUENCY;

    // 3. Spatial wave phase (directional wave progression across the map)
    float spatialPhase = (worldX * cfg::Wind::DIRECTION_X + worldY * cfg::Wind::DIRECTION_Y) * freq;

    // 4. Primary Macro Sway (Gentle oscillating roll around the wind pose)
    float macroSway = std::sin(t + spatialPhase) * macroWeight;

    // 5. Traveling Wind Gusts (Periodic directional push envelope)
    float gustPhase = t * cfg::Wind::GUST_FREQUENCY + (worldX * freq * cfg::Wind::GUST_SPATIAL_FACTOR);
    float rawGust = std::sin(gustPhase);
    float gustMagnitude = std::max(0.0f, rawGust) * cfg::Wind::GUST_STRENGTH;
    float gustPush = gustMagnitude * macroWeight;

    // 6. Micro Turbulence (Fast flutter on upper canopy edges, naturally amplified during gusts)
    float flutterSpeed = t * cfg::Wind::TURBULENCE_SPEED + (spatialPhase * cfg::Wind::TURBULENCE_SPATIAL_MULT);
    float microFlutter = std::sin(flutterSpeed) * cfg::Wind::TURBULENCE * (1.0f + gustMagnitude * cfg::Wind::GUST_TURBULENCE_BOOST) * microWeight;

    // 7. Total combined displacement
    return (macroSway + gustPush + microFlutter) * cfg::Wind::STRENGTH;
}
