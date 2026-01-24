#pragma once

#include <glm/glm.hpp>
#include <array>

class Frustum {
public:
    // Update frustum from View-Projection matrix
    void Update(const glm::mat4& viewProj);

    // Check if Axis-Aligned Bounding Box is visible
    // min: Bottom-Left-Back corner
    // max: Top-Right-Front corner
    bool IsBoxVisible(const glm::vec3& min, const glm::vec3& max) const;

private:
    struct Plane {
        float a, b, c, d;
        float Distance(const glm::vec3& p) const {
            return a * p.x + b * p.y + c * p.z + d;
        }

        void Normalize();
    };

    std::array<Plane, 6> m_planes;
};
