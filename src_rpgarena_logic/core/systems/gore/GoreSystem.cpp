#include "GoreSystem.h"
#include "Config.h"
#include <iostream>
#include <unordered_map>

namespace {
    static std::unordered_map<std::string, std::unique_ptr<sf::Texture>> sBoneTextureCache;
}

const sf::Texture* GoreSystem::getBoneTexture(const std::string& mobType, const std::string& nodeName) {
    if (mobType.empty() || nodeName.empty()) return nullptr;
    std::string boneStem = nodeName;
    if (nodeName.find("hand") != std::string::npos || nodeName.find("arm") != std::string::npos) boneStem = "hand";
    else if (nodeName.find("foot") != std::string::npos || nodeName.find("leg") != std::string::npos) boneStem = "foot";
    else if (nodeName.find("head") != std::string::npos) boneStem = "head";
    else if (nodeName.find("body") != std::string::npos || nodeName.find("torso") != std::string::npos) boneStem = "body";

    std::string path = "assets/textures/mobs/" + mobType + "/parts/bones/" + boneStem + "_bone.png";
    if (mobType == "player") {
        path = "assets/textures/player/parts/partes/bones/" + boneStem + "_bone.png";
    }

    auto it = sBoneTextureCache.find(path);
    if (it != sBoneTextureCache.end()) {
        return it->second.get();
    }

    auto tex = std::make_unique<sf::Texture>();
    if (tex->loadFromFile(path)) {
        tex->setSmooth(false);
        const sf::Texture* ptr = tex.get();
        sBoneTextureCache[path] = std::move(tex);
        std::cout << "[GoreSystem] Loaded bone texture: " << path << std::endl;
        return ptr;
    } else {
        sBoneTextureCache[path] = nullptr;
        return nullptr;
    }
}

GoreSystem::GoreSystem() {
    mGibs.reserve(cfg::Player::GORE_MAX_GIBS);
}

void GoreSystem::clear() {
    mActiveCount = 0;
}

GoreSystem::Gib* GoreSystem::spawnGibSlot() {
    Gib* g = nullptr;

    if (mActiveCount < mGibs.size()) {
        g = &mGibs[mActiveCount];
    } else if (mGibs.size() < cfg::Player::GORE_MAX_GIBS) {
        mGibs.emplace_back();
        g = &mGibs.back();
    } else {
        return nullptr;
    }

    g->vertices.fill(sf::Vertex());
    g->texture = nullptr;
    g->armorTexture = nullptr;
    g->armorVertices.fill(sf::Vertex());
    g->item = nullptr;
    g->hasSpawnedItem = false;
    g->velocity = {0.f, 0.f};
    g->angularVelocity = 0.f;
    g->lifetime = 0.f;
    g->maxLifetime = 0.f;
    g->groundY = 0.f;
    g->onGround = false;
    g->parentIndex = -1;
    g->restOffset = {0.f, 0.f};
    g->rotation = 0.f;
    g->restitution = 0.5f;
    g->friction = 0.8f;
    g->allowRotation = true;
    g->rotationLocked = false;
    g->id = ++mNextGibId;
    g->layerPriority = 0;
    g->mobBaseX = 0.f;
    g->facingDir = 1.f;
    g->deathSortY = 0.f;

    mActiveCount++;
    g->active = true;
    return g;
}
