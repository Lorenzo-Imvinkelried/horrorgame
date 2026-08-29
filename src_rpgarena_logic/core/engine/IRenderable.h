#pragma once
#include <SFML/Graphics.hpp>
#include <vector>

class IRenderable {
public:
    virtual ~IRenderable() = default;

    // [OPTIMIZATION] Enum to avoid dynamic_cast
    enum class RenderType { Generic, Entity, Decor, Particle };

    virtual RenderType getRenderType() const { return RenderType::Generic; }

    virtual bool castsShadow() const { return false; }

    // Obtiene los datos de renderizado necesarios para el batching.
    // vertices: Vector donde se deben agregar los vértices (normalmente 4 para un quads/2 triangulos).
    // texture: Puntero a la textura que utiliza este objeto.
    virtual void getRenderData(std::vector<sf::Vertex>& vertices, const sf::Texture*& texture) const = 0;

    // Obtiene los datos de renderizado para las sombras.
    virtual void getShadowRenderData(std::vector<sf::Vertex>& vertices, const sf::Texture*& texture) const {}

    // Optional: Return raw sf::Drawable to bypass batching
    virtual const sf::Drawable* getDrawable() const { return nullptr; }
};
