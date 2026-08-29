#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
#include <array>
#include <string>
#include <functional>
#include "core/engine/IRenderable.h"

class GoreSystem {
public:
    using BaseYCallback = std::function<void(float)>;

    struct Gib : public IRenderable, public sf::Drawable {
        std::array<sf::Vertex, 6> vertices;
        const sf::Texture* texture = nullptr;

        const sf::Texture* armorTexture = nullptr;
        std::array<sf::Vertex, 6> armorVertices;

        const sf::Texture* boneTexture = nullptr;
        std::array<sf::Vertex, 6> boneVertices;
        float decayTimer = 0.f;

        std::shared_ptr<class Item> item = nullptr;
        bool hasSpawnedItem = false;

        sf::Vector2f velocity;
        float angularVelocity = 0.f;
        float lifetime = 0.f;
        float maxLifetime = 0.f;
        float groundY = 0.f;
        bool onGround = false; 
        bool active = false;

        // [RAGDOLL CONSTRAINTS]
        int parentIndex = -1;
        sf::Vector2f restOffset = {0.f, 0.f};
        float rotation = 0.f;
        float restitution = 0.5f;
        float friction = 0.8f;
        bool allowRotation = true;
        bool rotationLocked = false;
        std::uint64_t id = 0;
        int layerPriority = 0;
        float mobBaseX = 0.f;
        float facingDir = 1.f;
        float deathSortY = 0.f;

        Gib() = default;

        sf::Vector2f getCenter() const {
            sf::Vector2f center(0.f, 0.f);
            for (int j = 0; j < 6; ++j) {
                center += vertices[j].position;
            }
            return center / 6.f;
        }

        // IRenderable & sf::Drawable implementation
        RenderType getRenderType() const override { return RenderType::Generic; }
        const sf::Drawable* getDrawable() const override { return this; }
        void draw(sf::RenderTarget& target, sf::RenderStates states = sf::RenderStates::Default) const override;
        void getRenderData(std::vector<sf::Vertex>& outVertices, const sf::Texture*& outTexture) const override;
    };

    struct LandedDrop {
        std::shared_ptr<class Item> item;
        sf::Vector2f position;
        bool isArmor = false;
        const sf::Texture* customTexture = nullptr;
        std::array<sf::Vertex, 6> customVertices;
    };

public:
    GoreSystem();

    void update(sf::Time dt);
    void draw(sf::RenderTarget& target, sf::RenderStates states = sf::RenderStates::Default, BaseYCallback onSetBaseY = nullptr);

    void emitGibs(const sf::Sprite& mobSprite, sf::Vector2f sourcePos = {0.f, 0.f}, float forceMultiplier = 1.0f, sf::Vector2f initialVelocity = {0.f, 0.f}, float deathSortY = 0.f);
    void emitGibs(const sf::Sprite& partSprite, float floorY, sf::Vector2f sourcePos = {0.f, 0.f}, float forceMultiplier = 1.0f, sf::Vector2f initialVelocity = {0.f, 0.f}, float deathSortY = 0.f);
    void emitGibs(const std::vector<sf::Vertex>& vertices, const sf::Texture* texture, float floorY, sf::Vector2f sourcePos = {0.f, 0.f}, float forceMultiplier = 1.0f, const std::vector<std::string>& nodeNames = {}, sf::Vector2f initialVelocity = {0.f, 0.f}, const class Animation* anim = nullptr, const std::vector<std::shared_ptr<class Item>>& armorItems = {}, const std::string& mobType = "", float deathSortY = 0.f);

    const std::vector<LandedDrop>& getLandedDrops() const { return mLandedDrops; }
    void clearLandedDrops() { mLandedDrops.clear(); }
    const std::vector<Gib>& getGibs() const { return mGibs; }

    void clear();
    int getActiveCount() const { return mActiveCount; }

    static const sf::Texture* getBoneTexture(const std::string& mobType, const std::string& nodeName);

private:
    Gib* spawnGibSlot();
    void spawnGib(const sf::Texture& texture, const sf::IntRect& rect, sf::Vector2f pos, float rotation, sf::Vector2f scale, sf::Color color, float floorY, sf::Vector2f sourcePos = {0.f, 0.f}, float forceMultiplier = 1.0f, sf::Vector2f initialVelocity = {0.f, 0.f}, float deathSortY = 0.f);

private:
    std::vector<LandedDrop> mLandedDrops;
    std::vector<Gib> mGibs;
    int mActiveCount = 0;
    std::uint64_t mNextGibId = 0;
};
