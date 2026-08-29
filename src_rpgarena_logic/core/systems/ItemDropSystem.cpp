#include "ItemDropSystem.h"
#include "entities/player/Player.h"
#include "../engine/InputManager.h"
#include "Config.h"      // [NEW]
#include "SoundSystem.h" // [AUDIO]
#include <cmath>
#include <iostream>


ItemDropSystem::ItemDropSystem() {}

void ItemDropSystem::dropItem(std::shared_ptr<Item> item, sf::Vector2f worldPos,
                              ResourceManager &res,
                              sf::Vector2f initialVelocity, float groundY,
                              float initialRotation, sf::Vector2f initialScale,
                              bool isArmor, const sf::Texture *customTexture,
                              const std::array<sf::Vertex, 6> *customVertices,
                              bool isVisualOnly) {
  if (!item)
    return;

  DroppedItem drop;
  drop.position = worldPos;
  drop.startY = worldPos.y + cfg::ItemDrop::FLOAT_OFFSET_Y;
  drop.item = item;
  drop.bobTimer = 0.f; // Randomize slightly?

  // Physics parameters
  drop.velocity = initialVelocity;
  drop.groundY = groundY;
  drop.rotation = initialRotation;
  drop.scale = initialScale;
  drop.onGround = (initialVelocity == sf::Vector2f(0.f, 0.f));

  drop.isArmor = isArmor;
  drop.customTexture = customTexture;
  if (customVertices) {
    drop.customVertices = *customVertices;
  }
  drop.isVisualOnly = isVisualOnly;
  drop.visualLifetime = 10.f;

  // Configurar el Sprite basándose en el Item
  if (!isArmor) {
    try {
      sf::Texture &tex = res.getTexture(item->texturePath);
      drop.sprite.emplace(tex);
      if (item->textureRect.size.x > 0 && item->textureRect.size.y > 0) {
        drop.sprite->setTextureRect(item->textureRect);
      }

      // Centrar origen en la base (bottom-center) para que el Y-Sorting sea
      // realista
      sf::FloatRect bounds = drop.sprite->getLocalBounds();
      sf::IntRect rect = item->textureRect;
      if (rect.size.x <= 0 || rect.size.y <= 0) {
        rect = sf::IntRect({0, 0}, sf::Vector2i(tex.getSize()));
      }

      const auto *mask = res.getBitmask(&tex);
      int maxY = rect.position.y + rect.size.y - 1;
      if (mask) {
        sf::Vector2u texSize = tex.getSize();
        bool found = false;
        for (int py = rect.position.y + rect.size.y - 1; py >= rect.position.y;
             --py) {
          for (int px = rect.position.x; px < rect.position.x + rect.size.x;
               ++px) {
            int index = py * texSize.x + px;
            if (index >= 0 && index < static_cast<int>(mask->size())) {
              if ((*mask)[index]) {
                maxY = py;
                found = true;
                break;
              }
            }
          }
          if (found)
            break;
        }
      }

      float originY = (float)(maxY - rect.position.y + 1);
      drop.sprite->setOrigin({bounds.size.x * 0.5f, originY});

      // Efecto visual: escala normal o la escala inicial
      drop.sprite->setScale(drop.scale);
      drop.sprite->setPosition(worldPos);
      drop.sprite->setRotation(sf::degrees(drop.rotation));

    } catch (...) {
      // std::cerr << "[ItemDropSystem] Warning: Could not load texture for
      // dropped item.\n";
    }
  }

  mDroppedItems.push_back(drop);
  std::cout << "[DEBUG_DROP] Created world drop for item " << item->name
            << " [ID: " << item->id << "] at position " << worldPos.x << ", "
            << worldPos.y << " (isArmor: " << isArmor << ")" << std::endl;
}

void ItemDropSystem::update(sf::Time dt, sf::Vector2f playerPos,
                            sf::Vector2f mouseWorldPos, ResourceManager &res) {
  float s = dt.asSeconds();
  mHoveredItem = nullptr;

  // Actualizamos lógica visual de todos los items tirados
  for (auto &drop : mDroppedItems) {
    if (!drop.onGround) {
      // Apply physics
      drop.velocity.y += cfg::Gore::GRAVITY * s;
      drop.position += drop.velocity * s;

      // Check collision with ground Y (origin is bottom-centered, so we check
      // position.y)
      if (drop.velocity.y >= 0.f && drop.position.y >= drop.groundY) {
        drop.position.y = drop.groundY;

        // Real Physics Bounce
        if (drop.velocity.y > cfg::Gore::MIN_BOUNCE_VELOCITY) {
          drop.velocity.y = -drop.velocity.y * cfg::Gore::RESTITUTION;
          drop.velocity.x = drop.velocity.x * cfg::Gore::FRICTION;
        } else {
          drop.onGround = true;
          drop.velocity = {0.f, 0.f};
          drop.startY = drop.position.y;
          drop.bobTimer = 0.f;
        }
      }

      if (!drop.isArmor && drop.sprite) {
        drop.sprite->setPosition(drop.position);
        drop.sprite->setRotation(
            sf::degrees(drop.rotation)); // Keep constant rotation while flying
        drop.sprite->setScale(drop.scale); // Keep original scale flip
      }
    } else {
      if (!drop.isArmor) {
        if (drop.sprite) {
          drop.sprite->setPosition(drop.position);
          drop.sprite->setRotation(
              sf::degrees(drop.rotation));   // Retain landing rotation
          drop.sprite->setScale(drop.scale); // Keep original scale flip
        }
      }
    }

    // Pixel-Perfect Mouse hover checks
    drop.isHovered = false; // Default

    if (drop.isArmor) {
      std::array<sf::Vertex, 6> tempVerts = drop.customVertices;

      // 1. Get bounding box of tempVerts to do a quick broad-phase check
      float minX = 999999.f, maxX = -999999.f;
      float minY = 999999.f, maxY = -999999.f;
      for (int j = 0; j < 6; ++j) {
        float x = tempVerts[j].position.x;
        float y = tempVerts[j].position.y;
        if (x < minX)
          minX = x;
        if (x > maxX)
          maxX = x;
        if (y < minY)
          minY = y;
        if (y > maxY)
          maxY = y;
      }
      sf::FloatRect bounds({minX, minY}, {maxX - minX, maxY - minY});

      // Damos un pequeño margen para que sea más fácil hacer hover en el broad
      // phase
      sf::FloatRect hoverBounds(
          {bounds.position.x - 5.f, bounds.position.y - 5.f},
          {bounds.size.x + 10.f, bounds.size.y + 10.f});

      if (hoverBounds.contains(mouseWorldPos) && drop.customTexture) {
        // 2. Narrow-phase: barycentric coordinate texture lookup
        auto getBarycentricTexCoords =
            [](sf::Vector2f P, sf::Vector2f A, sf::Vector2f B, sf::Vector2f C,
               sf::Vector2f A_tex, sf::Vector2f B_tex, sf::Vector2f C_tex,
               sf::Vector2f &outTexCoords) -> bool {
          sf::Vector2f v0 = B - A;
          sf::Vector2f v1 = C - A;
          sf::Vector2f v2 = P - A;
          float den = v0.x * v1.y - v1.x * v0.y;
          if (std::abs(den) < 0.0001f)
            return false;
          float v = (v2.x * v1.y - v1.x * v2.y) / den;
          float w = (v0.x * v2.y - v2.x * v0.y) / den;
          float u = 1.0f - v - w;

          if (u >= -0.01f && v >= -0.01f && w >= -0.01f && u <= 1.01f &&
              v <= 1.01f && w <= 1.01f) {
            outTexCoords = u * A_tex + v * B_tex + w * C_tex;
            return true;
          }
          return false;
        };

        sf::Vector2f outTexCoords;
        bool hit = false;

        // Triangle 1: tempVerts[0], [1], [2]
        if (getBarycentricTexCoords(
                mouseWorldPos, tempVerts[0].position, tempVerts[1].position,
                tempVerts[2].position, tempVerts[0].texCoords,
                tempVerts[1].texCoords, tempVerts[2].texCoords, outTexCoords)) {
          hit = true;
        }
        // Triangle 2: tempVerts[3], [4], [5]
        else if (getBarycentricTexCoords(
                     mouseWorldPos, tempVerts[3].position,
                     tempVerts[4].position, tempVerts[5].position,
                     tempVerts[3].texCoords, tempVerts[4].texCoords,
                     tempVerts[5].texCoords, outTexCoords)) {
          hit = true;
        }

        if (hit) {
          sf::Vector2u texSize = drop.customTexture->getSize();
          int tx = static_cast<int>(std::round(outTexCoords.x));
          int ty = static_cast<int>(std::round(outTexCoords.y));
          if (tx >= 0 && tx < static_cast<int>(texSize.x) && ty >= 0 &&
              ty < static_cast<int>(texSize.y)) {
            const auto *mask = res.getBitmask(drop.customTexture);
            if (mask) {
              int index = ty * texSize.x + tx;
              if (index >= 0 && index < static_cast<int>(mask->size())) {
                if ((*mask)[index]) {
                  drop.isHovered = true;
                  mHoveredItem = &drop;
                }
              }
            }
          }
        }
      }
    } else if (drop.sprite) {
      sf::FloatRect bounds = drop.sprite->getGlobalBounds();
      sf::FloatRect hoverBounds(
          {bounds.position.x - 5.f, bounds.position.y - 5.f},
          {bounds.size.x + 10.f, bounds.size.y + 10.f});

      if (hoverBounds.contains(mouseWorldPos)) {
        // Narrow phase: local pixel perfect bitmask check
        sf::Vector2f localPos =
            drop.sprite->getInverseTransform().transformPoint(mouseWorldPos);
        sf::IntRect rect = drop.sprite->getTextureRect();
        int tx = rect.position.x + static_cast<int>(std::round(localPos.x));
        int ty = rect.position.y + static_cast<int>(std::round(localPos.y));

        sf::Vector2u texSize = drop.sprite->getTexture().getSize();
        if (localPos.x >= 0.f && localPos.x < rect.size.x &&
            localPos.y >= 0.f && localPos.y < rect.size.y && tx >= 0 &&
            tx < static_cast<int>(texSize.x) && ty >= 0 &&
            ty < static_cast<int>(texSize.y)) {

          const auto *mask = res.getBitmask(&drop.sprite->getTexture());
          if (mask) {
            int index = ty * texSize.x + tx;
            if (index >= 0 && index < static_cast<int>(mask->size())) {
              if ((*mask)[index]) {
                drop.isHovered = true;
                mHoveredItem = &drop;
              }
            }
          }
        }
      }
    }
  }

  // Ensure only the final hovered item is marked as hovered
  for (auto &drop : mDroppedItems) {
    if (&drop != mHoveredItem) {
      drop.isHovered = false;
    }
  }
}

bool ItemDropSystem::tryPickup(
    Player *player, sf::Vector2f playerPos, sf::Vector2f mouseWorldPos,
    const InputManager &input,
    std::function<bool(std::shared_ptr<Item>)> inventoryAddFn) {
  if (!player || !player->isAlive())
    return false;
  if (!input.isActionJustPressed(Action::PickupLoot))
    return false;

  // Buscamos el ítem hovered actual
  if (!mHoveredItem)
    return false;

  // Calculamos distancia al jugador
  float dx = mHoveredItem->position.x - playerPos.x;
  float dy = mHoveredItem->position.y - playerPos.y;
  float distSq = dx * dx + dy * dy;

  if (distSq > PICKUP_RANGE * PICKUP_RANGE) {
    std::cout << "[ItemDropSystem] Item too far to pick up.\n";
    return false;
  }

  // Intentamos agregarlo usando la función delegada (ej. Inventario)
  if (inventoryAddFn(mHoveredItem->item)) {
    std::cout << "[ItemDropSystem] Picked up " << mHoveredItem->item->name
              << "!\n";

    if (auto *ss = SoundSystem::getInstance()) {
      ss->playSound("assets/sounds/pick_up.wav", 100.f);
    }

    // Borrarlo de la lista en el mundo
    for (auto it = mDroppedItems.begin(); it != mDroppedItems.end(); ++it) {
      if (&(*it) == mHoveredItem) {
        mDroppedItems.erase(it);
        mHoveredItem = nullptr;
        return true;
      }
    }
  } else {
    std::cout << "[ItemDropSystem] Pickup failed. Inventory full!\n";
  }

  return false;
}

const DroppedItem *ItemDropSystem::getHoveredItem() const {
  return mHoveredItem;
}

void DroppedItem::getShadowRenderData(std::vector<sf::Vertex> &vertices,
                                      const sf::Texture *&texture) const {
  if (isArmor) {
    if (!customTexture) {
      texture = nullptr;
      return;
    }
    texture = customTexture;

    float shadowScaleY = cfg::Shadow::SCALE_Y;
    float shadowScaleX = cfg::Shadow::SCALE_X;
    float shadowSkewX = cfg::Shadow::SKEW_X;
    float shOffsetX = cfg::Shadow::OFFSET_X;
    float shOffsetY = cfg::Shadow::OFFSET_Y;

    float itemGroundY = onGround ? position.y : groundY;

    sf::Vector2f p1 = customVertices[0].position;
    sf::Vector2f p2 = customVertices[1].position;
    sf::Vector2f p3 = customVertices[4].position;
    sf::Vector2f p4 = customVertices[2].position;

    auto projectPoint = [&](sf::Vector2f p) -> sf::Vector2f {
      float relX = p.x - position.x;
      float height = itemGroundY - p.y;

      float projX =
          position.x + relX * shadowScaleX + shOffsetX + height * shadowSkewX;
      float projY =
          itemGroundY + (p.y - itemGroundY) * shadowScaleY + shOffsetY;
      return {projX, projY};
    };

    sf::Vector2f proj1 = projectPoint(p1);
    sf::Vector2f proj2 = projectPoint(p2);
    sf::Vector2f proj3 = projectPoint(p3);
    sf::Vector2f proj4 = projectPoint(p4);

    float u1 = customVertices[0].texCoords.x;
    float v1 = customVertices[0].texCoords.y;
    float u2 = customVertices[4].texCoords.x;
    float v2 = customVertices[4].texCoords.y;

    sf::Color shColor = sf::Color(46, 34, 47, 255);

    // T1: P1, P2, P4
    vertices.push_back(sf::Vertex{proj1, shColor, {u1, v1}});
    vertices.push_back(sf::Vertex{proj2, shColor, {u2, v1}});
    vertices.push_back(sf::Vertex{proj4, shColor, {u1, v2}});

    // T2: P2, P3, P4
    vertices.push_back(sf::Vertex{proj2, shColor, {u2, v1}});
    vertices.push_back(sf::Vertex{proj3, shColor, {u2, v2}});
    vertices.push_back(sf::Vertex{proj4, shColor, {u1, v2}});
    return;
  }
  if (!sprite.has_value()) {
    texture = nullptr;
    return;
  }

  texture = &sprite->getTexture();

  float shadowScaleY = cfg::Shadow::SCALE_Y;
  float shadowScaleX = cfg::Shadow::SCALE_X;
  float shadowSkewX = cfg::Shadow::SKEW_X;
  float shOffsetX = cfg::Shadow::OFFSET_X;
  float shOffsetY = cfg::Shadow::OFFSET_Y;

  float itemGroundY = onGround ? position.y : groundY;

  const auto &seedRect = sprite->getTextureRect();
  sf::FloatRect rect(sf::Vector2f(seedRect.position),
                     sf::Vector2f(seedRect.size));

  sf::Transform trans = sprite->getTransform();

  float left = 0.f;
  float top = 0.f;
  float right = rect.size.x;
  float bottom = rect.size.y;

  sf::Vector2f p1 = trans.transformPoint({left, top});
  sf::Vector2f p2 = trans.transformPoint({right, top});
  sf::Vector2f p3 = trans.transformPoint({right, bottom});
  sf::Vector2f p4 = trans.transformPoint({left, bottom});

  auto projectPoint = [&](sf::Vector2f p) -> sf::Vector2f {
    float relX = p.x - position.x;
    float height = itemGroundY - p.y;

    float projX =
        position.x + relX * shadowScaleX + shOffsetX + height * shadowSkewX;
    float projY = itemGroundY + (p.y - itemGroundY) * shadowScaleY + shOffsetY;
    return {projX, projY};
  };

  sf::Vector2f proj1 = projectPoint(p1);
  sf::Vector2f proj2 = projectPoint(p2);
  sf::Vector2f proj3 = projectPoint(p3);
  sf::Vector2f proj4 = projectPoint(p4);

  float u1 = rect.position.x;
  float v1 = rect.position.y;
  float u2 = rect.position.x + rect.size.x;
  float v2 = rect.position.y + rect.size.y;

  sf::Color shColor = sf::Color(46, 34, 47, 255);

  // T1: P1, P2, P4
  vertices.push_back(sf::Vertex{proj1, shColor, {u1, v1}});
  vertices.push_back(sf::Vertex{proj2, shColor, {u2, v1}});
  vertices.push_back(sf::Vertex{proj4, shColor, {u1, v2}});

  // T2: P2, P3, P4
  vertices.push_back(sf::Vertex{proj2, shColor, {u2, v1}});
  vertices.push_back(sf::Vertex{proj3, shColor, {u2, v2}});
  vertices.push_back(sf::Vertex{proj4, shColor, {u1, v2}});
}

void DroppedItem::draw(sf::RenderTarget &target,
                       sf::RenderStates states) const {
  // 1. Outline shader compilation on-demand
  static sf::Shader outlineShader;
  static bool shaderLoaded = false;
  static bool shaderInitTried = false;

  if (!shaderInitTried) {
    shaderInitTried = true;
    if (sf::Shader::isAvailable()) {
      shaderLoaded = outlineShader.loadFromFile(
          "assets/shaders/item_outline.frag", sf::Shader::Type::Fragment);
      if (!shaderLoaded) {
        std::cerr << "[ItemDropSystem] ERROR: Failed to load "
                     "assets/shaders/item_outline.frag\n";
      }
    }
  }

  if (isArmor) {
    if (!customTexture)
      return;

    sf::RenderStates armorStates = states;
    armorStates.texture = customTexture;

    if (isHovered) {
      // Draw 1px white outline in 4 directions if shader is loaded
      if (shaderLoaded) {
        sf::RenderStates outlineStates = states;
        outlineStates.shader = &outlineShader;
        outlineStates.texture = customTexture;

        // 1 texture pixel offset in world coordinates
        sf::Vector2f offsets[4] = {
            {-1.f, 0.f}, {1.f, 0.f}, {0.f, -1.f}, {0.f, 1.f}};

        std::array<sf::Vertex, 6> outlineVerts = customVertices;
        for (int j = 0; j < 6; ++j) {
          outlineVerts[j].color = sf::Color::White;
        }

        for (int dir = 0; dir < 4; ++dir) {
          std::array<sf::Vertex, 6> tempVerts = outlineVerts;
          for (int j = 0; j < 6; ++j) {
            tempVerts[j].position += offsets[dir];
          }
          target.draw(tempVerts.data(), 6, sf::PrimitiveType::Triangles,
                      outlineStates);
        }
      }

      // Draw the main foreground rotated armor quad on top (with yellow hover
      // tint)
      std::array<sf::Vertex, 6> mainVerts = customVertices;
      for (int j = 0; j < 6; ++j) {
        mainVerts[j].color = sf::Color(255, 255, 200, 255);
      }
      target.draw(mainVerts.data(), 6, sf::PrimitiveType::Triangles,
                  armorStates);
    } else {
      // Unhovered: draw armor quad with normal vertex color (white)
      std::array<sf::Vertex, 6> mainVerts = customVertices;
      for (int j = 0; j < 6; ++j) {
        mainVerts[j].color = sf::Color::White;
      }
      target.draw(mainVerts.data(), 6, sf::PrimitiveType::Triangles,
                  armorStates);
    }
    return;
  }

  if (!sprite.has_value())
    return;

  int numSprites = 1;
  if (item && item->stackCount > 1) {
    numSprites = std::min(item->stackCount, 3);
  }

  sf::Sprite tempSprite = *sprite;
  sf::Vector2f originalPos = tempSprite.getPosition();

  // [SCREEN-PIXEL SNAP] Snaps render position to the nearest screen pixel grid
  // (1/zoom world units) in HD mode, and to the integer world grid in virtual
  // resolution mode, to prevent camera jitter.
  bool isVirtual =
      (cfg::Window::INTERNAL_WIDTH > 0 && cfg::Window::INTERNAL_HEIGHT > 0);
  float snapZoom = isVirtual ? 1.f : cfg::Window::CAMERA_ZOOM;
  if (snapZoom > 0.f) {
    originalPos.x = std::round(originalPos.x * snapZoom) / snapZoom;
    originalPos.y = std::round(originalPos.y * snapZoom) / snapZoom;
  }

  sf::Color originalColor = tempSprite.getColor();

  for (int i = numSprites - 1; i >= 0; --i) {
    sf::Vector2f offset(i * -2.f * std::abs(scale.x),
                        i * -2.f * std::abs(scale.y));
    sf::Vector2f currentItemPos = originalPos + offset;

    // Render aura if fortified >= 6
    if (i == 0 && item && item->fortificationLevel >= 6) {
      ItemAuraRenderer::drawAura(target, tempSprite, item->fortificationLevel,
                                 1.0f, states);
    }

    if (i == 0 && isHovered) {
      // Draw 1px outline in 4 directions if shader is loaded
      if (shaderLoaded) {
        sf::RenderStates outlineStates = states;
        outlineStates.shader = &outlineShader;
        outlineShader.setUniform("texture", sf::Shader::CurrentTexture);

        // Outline color matching the main sprite's alpha/opacity
        tempSprite.setColor(sf::Color(255, 255, 255, originalColor.a));

        // 1 texture pixel offset in world coordinates
        sf::Vector2f outlineOffset(std::abs(scale.x), std::abs(scale.y));

        // Left
        tempSprite.setPosition(currentItemPos +
                               sf::Vector2f(-outlineOffset.x, 0.f));
        target.draw(tempSprite, outlineStates);

        // Right
        tempSprite.setPosition(currentItemPos +
                               sf::Vector2f(outlineOffset.x, 0.f));
        target.draw(tempSprite, outlineStates);

        // Up
        tempSprite.setPosition(currentItemPos +
                               sf::Vector2f(0.f, -outlineOffset.y));
        target.draw(tempSprite, outlineStates);

        // Down
        tempSprite.setPosition(currentItemPos +
                               sf::Vector2f(0.f, outlineOffset.y));
        target.draw(tempSprite, outlineStates);
      }

      // Draw the main foreground item sprite on top (with yellow hover tint)
      tempSprite.setPosition(currentItemPos);
      tempSprite.setColor(sf::Color(255, 255, 200, 255));
      target.draw(tempSprite, states);
    } else {
      tempSprite.setPosition(currentItemPos);
      if (i == 0) {
        tempSprite.setColor(originalColor);
      } else {
        sf::Color darkColor = originalColor;
        darkColor.r = static_cast<unsigned char>(originalColor.r * 0.7f);
        darkColor.g = static_cast<unsigned char>(originalColor.g * 0.7f);
        darkColor.b = static_cast<unsigned char>(originalColor.b * 0.7f);
        tempSprite.setColor(darkColor);
      }
      target.draw(tempSprite, states);
    }
  }
}
