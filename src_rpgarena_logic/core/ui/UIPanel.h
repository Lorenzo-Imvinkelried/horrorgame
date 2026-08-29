#pragma once
#include <SFML/Graphics.hpp>
#include "../engine/ResourceManager.h"

// Interface para cualquier panel de UI interactivo
class UIPanel {
public:
    virtual ~UIPanel() = default;

    virtual void draw(sf::RenderTarget& target, ResourceManager& res) = 0;
    virtual sf::FloatRect getBounds() const = 0;
    virtual void setPosition(sf::Vector2f pos) = 0;

    // Input hooks
    virtual void onMouseMove(sf::Vector2f mousePos) {}
    virtual bool onMousePress(sf::Vector2f mousePos) { return false; }
    virtual void onMouseRelease() {}

    // Visibility
    virtual bool isVisible() const { return mVisible; }
    virtual void setVisible(bool visible) { mVisible = visible; }

    // Drag state query
    virtual bool isBeingDragged() const { return false; }

protected:
    bool mVisible = false; // Default hidden? Or true? Hud manages it. Let's say false usually start closed.
};
