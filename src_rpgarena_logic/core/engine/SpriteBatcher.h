#pragma once
#include <SFML/Graphics.hpp>
#include <vector>

class SpriteBatcher {
public:
    SpriteBatcher();

    void add(const sf::Vertex* vertices, std::size_t vertexCount, const sf::Texture* texture);
    void render(sf::RenderTarget& target);
    void render(sf::RenderTarget& target, sf::RenderStates states);
    void clear();

    std::size_t getBatchCount() const { return mBatches.size(); }

private:
    struct Batch {
        std::size_t startIndex;
        std::size_t vertexCount;
        const sf::Texture* texture;
    };

    std::vector<sf::Vertex> mVertices;
    std::vector<Batch> mBatches;
    const sf::Texture* mCurrentTexture;
};
