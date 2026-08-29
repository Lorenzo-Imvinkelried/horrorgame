#include "Mob.h"

void Mob::draw(sf::RenderTarget& target, sf::RenderStates states) {
    mSkin.draw(target, states);
}

void Mob::drawLayer(sf::RenderTarget& target, int layer, sf::RenderStates states) {
    if (layer == 0) {
        draw(target, states);
    } else {
        mSkin.drawLayer(target, layer, states);
    }
}

float Mob::getLayerSortingY(int layer) const {
    return mSkin.getLayerSortingY(layer, getSortingY());
}

bool Mob::castsShadow() const {
    if (mCurrentState == State::Dead ||
        mCurrentState == State::Fading || mCurrentState == State::Removable) {
        return false;
    }
    return Entity::castsShadow();
}

void Mob::getShadowRenderData(std::vector<sf::Vertex> &vertices,
                             const sf::Texture *&texture) const {
    if (mCurrentState == State::Dead ||
        mCurrentState == State::Fading || mCurrentState == State::Removable) {
        texture = nullptr;
        return;
    }
    mSkin.getShadowRenderData(vertices, texture);
}

void Mob::getWeaponShadowRenderData(std::vector<sf::Vertex> &vertices,
                                   const sf::Texture *&texture,
                                   int slotIndex) const {
    if (mCurrentState == State::Dead ||
        mCurrentState == State::Fading || mCurrentState == State::Removable) {
        texture = nullptr;
        return;
    }
    mSkin.getWeaponShadowRenderData(vertices, texture, slotIndex);
}

void Mob::getArmorShadowRenderData(std::vector<sf::Vertex> &vertices,
                                  const sf::Texture *&texture,
                                  int slotIndex) const {
    if (mCurrentState == State::Dead ||
        mCurrentState == State::Fading || mCurrentState == State::Removable) {
        texture = nullptr;
        return;
    }
    mSkin.getArmorShadowRenderData(vertices, texture, slotIndex);
}

float Mob::getVisualHeight() const {
    sf::FloatRect b = mSkin.getBodyBounds();
    if (b.size.x > 0) return b.size.y;
    return 50.f;
}

sf::Vector2f Mob::getVisualPoint(const std::string& pointName) const {
    if (pointName == "head") {
        sf::FloatRect h = mSkin.getNodeGlobalBounds("head");
        if (h.size.x > 0.f && h.size.y > 0.f) {
            return sf::Vector2f(h.position.x + h.size.x * 0.5f, h.position.y);
        }
        sf::Vector2f p = getPosition();
        p.y -= getVisualHeight();
        return p;
    }
    return Entity::getVisualPoint(pointName);
}

