#pragma once
#include <vector>
#include <string>
#include <SFML/System/Vector2.hpp>
#include <SFML/System/Time.hpp>

struct RespawnTicket {
    std::string blueprintName;
    sf::Vector2f spawnPos;
    float timeRemaining;
    int level = -1;
    bool isBoss = false;
};

class RespawnSystem {
public:
    void scheduleRespawn(const std::string& blueprintName, sf::Vector2f pos, int level = -1, bool isBoss = false);
    void update(sf::Time dt);
    void clear(); // [LEVEL TRANSITION]
    
    // Devuelve los mobs que deben nacer este frame (y los elimina de la cola interna)
    std::vector<RespawnTicket> getReadyMobs();

private:
    std::vector<RespawnTicket> mMainQueue;
    std::vector<RespawnTicket> mReadyQueue; // Cola intermedia para throttling
};
