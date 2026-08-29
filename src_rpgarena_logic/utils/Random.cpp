#include "Random.h"
#include <ctime>
#include <utility> // for std::swap

std::mt19937 Random::s_Engine;

void Random::init() {
    s_Engine.seed(static_cast<unsigned>(std::time(nullptr)));
}

int Random::Int(int min, int max) {
    // Protección simple contra rangos invertidos
    if (min > max) std::swap(min, max);
    
    std::uniform_int_distribution<int> dist(min, max);
    return dist(s_Engine);
}

float Random::Float(float min, float max) {
    if (min > max) std::swap(min, max);
    
    std::uniform_real_distribution<float> dist(min, max);
    return dist(s_Engine);
}

bool Random::Roll(float percentage) {
    // Si tenemos 15.5% chance, tiramos 0..100. Si sale < 15.5, es éxito.
    if (percentage <= 0.0f) return false;
    if (percentage >= 100.0f) return true;
    return Float(0.0f, 100.0f) < percentage;
}
