#pragma once
#include <random>

class Random {
public:
    static void init();
    static int Int(int min, int max);
    static float Float(float min, float max);
    static bool Roll(float percentage);

private:
    static std::mt19937 s_Engine;
};
