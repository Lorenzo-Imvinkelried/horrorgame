#include "Frustum.h"
#include <cmath>

void Frustum::Plane::Normalize() {
    float mag = std::sqrt(a * a + b * b + c * c);
    a /= mag;
    b /= mag;
    c /= mag;
    d /= mag;
}

void Frustum::Update(const glm::mat4& viewProj) {
    // Gribb-Hartmann Plane Extraction
    // Left
    m_planes[0].a = viewProj[0][3] + viewProj[0][0];
    m_planes[0].b = viewProj[1][3] + viewProj[1][0];
    m_planes[0].c = viewProj[2][3] + viewProj[2][0];
    m_planes[0].d = viewProj[3][3] + viewProj[3][0];

    // Right
    m_planes[1].a = viewProj[0][3] - viewProj[0][0];
    m_planes[1].b = viewProj[1][3] - viewProj[1][0];
    m_planes[1].c = viewProj[2][3] - viewProj[2][0];
    m_planes[1].d = viewProj[3][3] - viewProj[3][0];

    // Bottom
    m_planes[2].a = viewProj[0][3] + viewProj[0][1];
    m_planes[2].b = viewProj[1][3] + viewProj[1][1];
    m_planes[2].c = viewProj[2][3] + viewProj[2][1];
    m_planes[2].d = viewProj[3][3] + viewProj[3][1];

    // Top
    m_planes[3].a = viewProj[0][3] - viewProj[0][1];
    m_planes[3].b = viewProj[1][3] - viewProj[1][1];
    m_planes[3].c = viewProj[2][3] - viewProj[2][1];
    m_planes[3].d = viewProj[3][3] - viewProj[3][1];

    // Near
    m_planes[4].a = viewProj[0][3] + viewProj[0][2];
    m_planes[4].b = viewProj[1][3] + viewProj[1][2];
    m_planes[4].c = viewProj[2][3] + viewProj[2][2];
    m_planes[4].d = viewProj[3][3] + viewProj[3][2];

    // Far
    m_planes[5].a = viewProj[0][3] - viewProj[0][2];
    m_planes[5].b = viewProj[1][3] - viewProj[1][2];
    m_planes[5].c = viewProj[2][3] - viewProj[2][2];
    m_planes[5].d = viewProj[3][3] - viewProj[3][2];

    for (auto& plane : m_planes) {
        plane.Normalize();
    }
}

bool Frustum::IsBoxVisible(const glm::vec3& min, const glm::vec3& max) const {
    // Check if box is outside any of the 6 planes
    for (const auto& plane : m_planes) {
        // Find the point on the box fully in the direction of the normal
        // This is the "positive vertex" (p)
        // If p is behind the plane, the whole box is behind.
        
        // Actually, optimization: check "positive vertex"
        // If the positive vertex is behind the plane (dist < 0), then outside.
        
        // Let's use the simple logic:
        // Identify the corner of the AABB closest to the normal direction.
        glm::vec3 p;
        p.x = (plane.a > 0) ? max.x : min.x;
        p.y = (plane.b > 0) ? max.y : min.y;
        p.z = (plane.c > 0) ? max.z : min.z;

        if (plane.Distance(p) < 0) {
            return false; // Outside this plane
        }
    }
    return true;
}
