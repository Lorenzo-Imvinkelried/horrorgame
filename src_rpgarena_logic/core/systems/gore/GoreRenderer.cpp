#include "GoreSystem.h"
#include "Config.h"
#include <algorithm>
#include <vector>
#include <cmath>

namespace {
    // 2D Sutherland-Hodgman edge clipping against horizontal ground plane y <= groundY
    static void clipTriangleToGround(sf::Vertex v0, sf::Vertex v1, sf::Vertex v2, float groundY, std::vector<sf::Vertex>& out) {
        auto intersect = [](const sf::Vertex& p1, const sf::Vertex& p2, float gY) -> sf::Vertex {
            float dy = p2.position.y - p1.position.y;
            float t = (std::abs(dy) > 0.00001f) ? (gY - p1.position.y) / dy : 0.f;
            t = std::clamp(t, 0.f, 1.f);

            sf::Vertex res;
            res.position.x = p1.position.x + t * (p2.position.x - p1.position.x);
            res.position.y = gY;
            res.texCoords = p1.texCoords + t * (p2.texCoords - p1.texCoords);

            sf::Color c1 = p1.color;
            sf::Color c2 = p2.color;
            res.color.r = static_cast<std::uint8_t>(c1.r + t * (c2.r - c1.r));
            res.color.g = static_cast<std::uint8_t>(c1.g + t * (c2.g - c1.g));
            res.color.b = static_cast<std::uint8_t>(c1.b + t * (c2.b - c1.b));
            res.color.a = static_cast<std::uint8_t>(c1.a + t * (c2.a - c1.a));
            return res;
        };

        bool in0 = (v0.position.y <= groundY);
        bool in1 = (v1.position.y <= groundY);
        bool in2 = (v2.position.y <= groundY);

        int count = (in0 ? 1 : 0) + (in1 ? 1 : 0) + (in2 ? 1 : 0);

        if (count == 0) return;

        if (count == 3) {
            out.push_back(v0);
            out.push_back(v1);
            out.push_back(v2);
            return;
        }

        if (count == 1) {
            sf::Vertex A = in0 ? v0 : (in1 ? v1 : v2);
            sf::Vertex B = in0 ? v1 : (in1 ? v2 : v0);
            sf::Vertex C = in0 ? v2 : (in1 ? v0 : v1);

            sf::Vertex AB = intersect(A, B, groundY);
            sf::Vertex AC = intersect(A, C, groundY);

            out.push_back(A);
            out.push_back(AB);
            out.push_back(AC);
            return;
        }

        if (count == 2) {
            sf::Vertex A = !in0 ? v1 : (!in1 ? v2 : v0);
            sf::Vertex B = !in0 ? v2 : (!in1 ? v0 : v1);
            sf::Vertex C = !in0 ? v0 : (!in1 ? v1 : v2);

            sf::Vertex AC = intersect(A, C, groundY);
            sf::Vertex BC = intersect(B, C, groundY);

            out.push_back(A);
            out.push_back(B);
            out.push_back(AC);

            out.push_back(B);
            out.push_back(BC);
            out.push_back(AC);
            return;
        }
    }
}

void GoreSystem::Gib::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    if (!active) return;

    float fleshAlpha = 255.f;
    float armorAlpha = 255.f;
    float boneAlpha = 0.f;

    float fleshSinkY = 0.f;
    float armorSinkY = 0.f;
    float boneSinkY = 0.f;

    if (cfg::Gore::ENABLE_BONE_DECAY && boneTexture && onGround) {
        float t = decayTimer;
        float delay = cfg::Gore::DECAY_DELAY_SEC;
        float fadeDur = cfg::Gore::DECAY_FADE_DURATION;
        float boneLife = cfg::Gore::BONE_LIFETIME_SEC;

        if (t < delay) {
            fleshAlpha = 255.f;
            armorAlpha = 255.f;
            boneAlpha = 0.f;
        } else if (t < delay + fadeDur) {
            float p = (t - delay) / fadeDur;
            fleshAlpha = (1.0f - p) * 255.f;
            armorAlpha = (1.0f - p) * 255.f;
            boneAlpha = p * 255.f;

            float sink = p * cfg::Gore::SINK_DISTANCE;
            fleshSinkY = sink;
            armorSinkY = sink;
        } else if (t < delay + fadeDur + boneLife) {
            fleshAlpha = 0.f;
            armorAlpha = 0.f;
            boneAlpha = 255.f;
        } else {
            float p = (t - (delay + fadeDur + boneLife)) / cfg::Gore::BONE_FADE_DURATION;
            fleshAlpha = 0.f;
            armorAlpha = 0.f;
            boneAlpha = std::max(0.f, (1.0f - p) * 255.f);

            boneSinkY = p * cfg::Gore::SINK_DISTANCE;
        }
    } else {
        if (lifetime <= cfg::Gore::FADE_DURATION) {
            float p = std::max(0.f, lifetime / cfg::Gore::FADE_DURATION);
            float fadeProgress = 1.0f - p;
            fleshAlpha = p * 255.f;
            armorAlpha = p * 255.f;

            float sink = fadeProgress * cfg::Gore::SINK_DISTANCE;
            fleshSinkY = sink;
            armorSinkY = sink;
        }
    }

    auto drawLayerClipped = [&](const std::array<sf::Vertex, 6>& inputVerts, float sinkY, float alpha, const sf::Texture* layerTexture, bool useShader = true) {
        if (!layerTexture || alpha <= 0.f) return;

        std::uint8_t a = static_cast<std::uint8_t>(std::clamp(alpha, 0.f, 255.f));
        std::array<sf::Vertex, 6> shifted = inputVerts;
        for (int j = 0; j < 6; ++j) {
            shifted[j].position.y += sinkY;
            shifted[j].color.a = a;
        }

        std::vector<sf::Vertex> clippedVerts;
        clippedVerts.reserve(12);

        float clipY = groundY - cfg::Gore::CLIP_OFFSET_Y;

        clipTriangleToGround(shifted[0], shifted[1], shifted[2], clipY, clippedVerts);
        clipTriangleToGround(shifted[3], shifted[4], shifted[5], clipY, clippedVerts);

        if (!clippedVerts.empty()) {
            sf::RenderStates layerStates = states;
            if (!useShader) {
                layerStates.shader = nullptr;
            }
            layerStates.texture = layerTexture;
            target.draw(clippedVerts.data(), clippedVerts.size(), sf::PrimitiveType::Triangles, layerStates);
        }
    };

    // 1. Draw bone layer (without shader)
    if (boneTexture && boneAlpha > 0.f) {
        std::array<sf::Vertex, 6> boneVerts = vertices;
        sf::Vector2u bSize = boneTexture->getSize();
        float bw = static_cast<float>(bSize.x);
        float bh = static_cast<float>(bSize.y);

        boneVerts[0].texCoords = {0.f, 0.f};
        boneVerts[1].texCoords = {bw, 0.f};
        boneVerts[2].texCoords = {0.f, bh};
        boneVerts[3].texCoords = {bw, 0.f};
        boneVerts[4].texCoords = {bw, bh};
        boneVerts[5].texCoords = {0.f, bh};

        drawLayerClipped(boneVerts, boneSinkY, boneAlpha, boneTexture, false);
    }

    // 2. Draw flesh layer (with occlusion/shadow shader)
    if (fleshAlpha > 0.f) {
        drawLayerClipped(vertices, fleshSinkY, fleshAlpha, texture, true);
    }

    // 3. Draw armor layer (with occlusion/shadow shader)
    if (armorTexture && armorAlpha > 0.f) {
        drawLayerClipped(armorVertices, armorSinkY, armorAlpha, armorTexture, true);
    }
}

void GoreSystem::Gib::getRenderData(std::vector<sf::Vertex>& outVertices, const sf::Texture*& outTexture) const {
    outTexture = texture ? texture : (armorTexture ? armorTexture : boneTexture);
    outVertices.assign(vertices.begin(), vertices.end());
}

void GoreSystem::draw(sf::RenderTarget& target, sf::RenderStates states, BaseYCallback onSetBaseY) {
    if (mActiveCount <= 0) return;

    std::vector<int> sortedIndices(mActiveCount);
    for (int i = 0; i < mActiveCount; ++i) {
        sortedIndices[i] = i;
    }

    std::stable_sort(sortedIndices.begin(), sortedIndices.end(), [this](int a, int b) {
        const Gib& gA = mGibs[a];
        const Gib& gB = mGibs[b];

        float yA = gA.onGround ? gA.groundY : gA.deathSortY;
        float yB = gB.onGround ? gB.groundY : gB.deathSortY;

        if (std::abs(yA - yB) > 0.5f) {
            return yA < yB;
        }
        if (gA.layerPriority != gB.layerPriority) {
            return gA.layerPriority < gB.layerPriority;
        }
        return gA.id < gB.id;
    });

    for (int idx : sortedIndices) {
        const Gib& g = mGibs[idx];

        if (onSetBaseY) {
            float baseY = g.groundY;
            onSetBaseY(baseY);
        }

        g.draw(target, states);
    }
}
