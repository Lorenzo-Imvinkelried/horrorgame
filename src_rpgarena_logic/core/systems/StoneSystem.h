#pragma once
#include "../items/Item.h"

class StoneSystem {
public:
    // Scale a procedural stone based on level and quality
    static void scaleStone(Item& item, int level, ItemQuality quality);
};
