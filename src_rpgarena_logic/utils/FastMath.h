#pragma once
#include <cmath>

struct FastMath {
    static inline float sinTable[1024];
    static inline bool initialized = false;

    static void init() {
        if (initialized) return;
        for (int i = 0; i < 1024; i++) {
            sinTable[i] = std::sin(i * 6.28318530718f / 1024.0f);
        }
        initialized = true;
    }

    static float fastSin(float angle) {
        // Enforce positive wrapping to avoid negative index issues
        // angle modulo 2PI normalized to 0-1024
        float normalized = std::fmod(angle, 6.28318530718f);
        if (normalized < 0.0f) normalized += 6.28318530718f;

        int index = static_cast<int>(normalized * 1024.0f / 6.28318530718f) & 1023;
        return sinTable[index];
    }
};
