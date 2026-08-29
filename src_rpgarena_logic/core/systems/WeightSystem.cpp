#include "WeightSystem.h"
#include <algorithm>

float WeightSystem::calcFootprintDepth(float weightKg) {
    if (weightKg <= cfg::Weight::THRESHOLD_KG) {
        return 0.f;
    }
    float calculated = weightKg / cfg::Weight::DIVISOR;
    return calculated;
}
