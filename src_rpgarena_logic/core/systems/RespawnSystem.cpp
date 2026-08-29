#include "RespawnSystem.h"
#include "Config.h"
#include "utils/Random.h" // Asumiendo que existe, usado en Mob.cpp. Si no, usaré rand() standard.
// Revisé Mob.cpp y usa Random::Float, así que debemos usar eso para consistencia.
// Si fallara al compilar, cambiaré a rand().

#include <algorithm>
#include <iostream>

void RespawnSystem::scheduleRespawn(const std::string& blueprintName, sf::Vector2f pos, int level, bool isBoss) {
    float baseTime = cfg::Mob::RESPAWN_TIME;
    float jitter   = cfg::Mob::RESPAWN_JITTER;
    
    // Generar tiempo aleatorio: TIME + (-JITTER, +JITTER)
    float timeToSpawn = baseTime + Random::Float(-jitter, jitter);
    if (timeToSpawn < 1.0f) timeToSpawn = 1.0f; // Seguridad

    // std::cout << "[RespawnSystem] Scheduled '" << blueprintName << "' in " << timeToSpawn << "s\n";
    mMainQueue.push_back({ blueprintName, pos, timeToSpawn, level, isBoss });
}

void RespawnSystem::update(sf::Time dt) {
    float s = dt.asSeconds();

    // 1. Actualizar timers
    for (auto& ticket : mMainQueue) {
        ticket.timeRemaining -= s;
    }

    // 2. Mover los listos a la cola de "Listos"
    // Usamos std::partition para separar los terminados
    auto it = std::partition(mMainQueue.begin(), mMainQueue.end(), 
        [](const RespawnTicket& t) {
            return t.timeRemaining > 0.f;
        });

    // Los elementos desde 'it' hasta el final son los que tienen timeRemaining <= 0
    if (it != mMainQueue.end()) {
        // Copiamos a mReadyQueue
        for (auto i = it; i != mMainQueue.end(); ++i) {
    // std::cout << "[RespawnSystem] Mob '" << i->blueprintName << "' is READY to spawn!\n";
            mReadyQueue.push_back(*i);
        }
        // Boramos del vector principal
        mMainQueue.erase(it, mMainQueue.end());
    }
}

void RespawnSystem::clear() {
    mMainQueue.clear();
    mReadyQueue.clear();
}

std::vector<RespawnTicket> RespawnSystem::getReadyMobs() {
    std::vector<RespawnTicket> readyBatch;
    
    if (mReadyQueue.empty()) return readyBatch;

    // THROTTLING: Saca solo hasta MAX_RESPAWNS_PER_FRAME
    int limit = cfg::Optimization::MAX_RESPAWNS_PER_FRAME;
    int count = 0;

    // Iteramos al revés para pop_back eficiente o simplemente extraemos del final
    while (!mReadyQueue.empty() && count < limit) {
        readyBatch.push_back(mReadyQueue.back());
        mReadyQueue.pop_back();
        count++;
    }

    return readyBatch;
}
