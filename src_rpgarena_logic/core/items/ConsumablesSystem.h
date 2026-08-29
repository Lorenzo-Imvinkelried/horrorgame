#pragma once
#include <memory>
#include "Item.h"

class Player;

class ConsumablesSystem {
public:
    static bool use(Player* player, const std::shared_ptr<Item>& item);
};
