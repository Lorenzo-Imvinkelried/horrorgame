// WeightSystem.h
// [WEIGHT] Converts entity weight (kg) into footprint depth (px).
//
// Formula:
//   weight <= threshold → depth = 0 (too light to leave marks)
//   weight >  threshold → depth = weight / divisor
//
// Default: threshold=100kg, divisor=1000
//   100kg → 0.1px, 200kg → 0.2px, 500kg → 0.5px, 1000kg → 1.0px
//
// Both threshold and divisor are exposed in config.json under "weight".
#pragma once

namespace cfg {
    namespace Weight {
        // Minimum weight (kg) required to leave footprints.
        // Anything at or below this produces depth = 0.
        inline float THRESHOLD_KG = 100.f;

        // Divisor that converts kg → pixels of depth.
        // depth_px = weight_kg / DIVISOR
        inline float DIVISOR = 1000.f;

        // Default player weight (kg). Loaded from config.json → player.weight_kg
        inline float PLAYER_DEFAULT_KG = 100.f;
    }
}

class WeightSystem {
public:
    // Pure function: converts weight in kg to footprint depth in pixels.
    // Returns 0 if weight <= threshold.
    static float calcFootprintDepth(float weightKg);
};
