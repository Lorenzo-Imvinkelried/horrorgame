#pragma once
#include "../engine/ResourceManager.h"
#include "../items/Item.h"
#include <SFML/Graphics.hpp>
#include <array>
#include <functional>
#include <memory>
#include <vector>


class Player;
class InputManager;

#include "../engine/IRenderable.h"

struct DroppedItem : public IRenderable, public sf::Drawable {
  sf::Vector2f position;
  std::shared_ptr<Item> item;
  std::optional<sf::Sprite> sprite;

  // Animation & State
  float bobTimer = 0.f;
  float startY = 0.f;
  bool isHovered = false;

  // Physics
  sf::Vector2f velocity = {0.f, 0.f};
  float groundY = 0.f;
  bool onGround = true;
  float rotation = 0.f;
  sf::Vector2f scale = {1.f, 1.f};

  virtual RenderType getRenderType() const override {
    return RenderType::Generic;
  }
  virtual bool castsShadow() const override { return true; }

  virtual const sf::Drawable *getDrawable() const override { return this; }

  virtual void draw(sf::RenderTarget &target,
                    sf::RenderStates states) const override;

  virtual void getRenderData(std::vector<sf::Vertex> &vertices,
                             const sf::Texture *&texture) const override {
    if (isArmor && customTexture) {
      texture = customTexture;
      vertices.resize(6);
      std::array<sf::Vertex, 6> verts = customVertices;
      sf::Color col = isHovered ? sf::Color(255, 255, 200, 255) : sf::Color::White;
      for (int j = 0; j < 6; ++j) {
        verts[j].color = col;
        vertices[j] = verts[j];
      }
    } else if (sprite.has_value()) {
      texture = &sprite->getTexture();
      sf::IntRect rect = sprite->getTextureRect();
      if (rect.size.x <= 0 || rect.size.y <= 0) {
        rect = sf::IntRect({0, 0}, sf::Vector2i(texture->getSize()));
      }
      sf::Vector2u dtSize = texture->getSize();
      float u1 = static_cast<float>(rect.position.x) / static_cast<float>(dtSize.x);
      float v1 = static_cast<float>(rect.position.y) / static_cast<float>(dtSize.y);
      float u2 = static_cast<float>(rect.position.x + rect.size.x) / static_cast<float>(dtSize.x);
      float v2 = static_cast<float>(rect.position.y + rect.size.y) / static_cast<float>(dtSize.y);

      vertices.resize(6);
      sf::Color col = isHovered ? sf::Color(255, 255, 200, 255) : sf::Color::White;
      vertices[0] = sf::Vertex{{0.f, 0.f}, col, {u1, v1}};
      vertices[1] = sf::Vertex{{1.f, 0.f}, col, {u2, v1}};
      vertices[2] = sf::Vertex{{0.f, 1.f}, col, {u1, v2}};
      vertices[3] = sf::Vertex{{1.f, 0.f}, col, {u2, v1}};
      vertices[4] = sf::Vertex{{1.f, 1.f}, col, {u2, v2}};
      vertices[5] = sf::Vertex{{0.f, 1.f}, col, {u1, v2}};
    } else {
      texture = nullptr;
    }
  }

  virtual void getShadowRenderData(std::vector<sf::Vertex> &vertices,
                                   const sf::Texture *&texture) const override;

  // Custom Vertices for Armor items
  bool isArmor = false;
  const sf::Texture *customTexture = nullptr;
  std::array<sf::Vertex, 6> customVertices;
  bool isVisualOnly = false;
  float visualLifetime = 10.f;
};

class ItemDropSystem {
public:
  ItemDropSystem();

  // Updates animation states for all dropped items
  void update(sf::Time dt, sf::Vector2f playerPos, sf::Vector2f mouseWorldPos,
              ResourceManager &res);

  // Drops an item at a specific world location
  void dropItem(std::shared_ptr<Item> item, sf::Vector2f worldPos,
                ResourceManager &res, sf::Vector2f initialVelocity = {0.f, 0.f},
                float groundY = 0.f, float initialRotation = 0.f,
                sf::Vector2f initialScale = {1.f, 1.f}, bool isArmor = false,
                const sf::Texture *customTexture = nullptr,
                const std::array<sf::Vertex, 6> *customVertices = nullptr,
                bool isVisualOnly = false);

  // Attempts to pick up an item if Action::PickupLoot is pressed
  bool tryPickup(Player *player, sf::Vector2f playerPos,
                 sf::Vector2f mouseWorldPos, const InputManager &input,
                 std::function<bool(std::shared_ptr<Item>)> inventoryAddFn);

  // Get the currently hovered item for Tooltip rendering
  const DroppedItem *getHoveredItem() const;

  const std::vector<DroppedItem> &getDroppedItems() const {
    return mDroppedItems;
  }

  void clear() {
    mDroppedItems.clear();
    mHoveredItem = nullptr;
  }

private:
  std::vector<DroppedItem> mDroppedItems;
  const DroppedItem *mHoveredItem = nullptr;

  // Physics constants
  const float PICKUP_RANGE = 80.0f; // Maximum distance to pick up
  const float HOVER_RADIUS = 20.0f; // Mouse radius for hovering
};
