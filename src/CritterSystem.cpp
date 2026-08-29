#include "CritterSystem.h"
#include <cmath>
#include <cstdlib>
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>
#include <iostream>

CritterSystem::CritterSystem() {
    glGenVertexArrays(1, &m_critterVAO);
    glGenBuffers(1, &m_critterVBO);

    glBindVertexArray(m_critterVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_critterVBO);

    // Layout: Pos (0), Color (1), UV (2), Normal (3)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(3);

    glBindVertexArray(0);
}

CritterSystem::~CritterSystem() {
    if (m_critterVAO) glDeleteVertexArrays(1, &m_critterVAO);
    if (m_critterVBO) glDeleteBuffers(1, &m_critterVBO);
}

void CritterSystem::Init(glm::vec3 playerPos) {
    m_butterflies.clear();
    m_fireflies.clear();
    m_frogs.clear();

    // 1. Spawn Butterflies
    glm::vec3 butterflyColors[] = {
        glm::vec3(0.95f, 0.55f, 0.10f), // Monarch Orange
        glm::vec3(0.20f, 0.65f, 0.98f), // Blue Morpho
        glm::vec3(0.35f, 0.90f, 0.45f), // Emerald Swallowtail
        glm::vec3(0.85f, 0.40f, 0.95f), // Violet Moth
        glm::vec3(0.98f, 0.92f, 0.20f)  // Golden Brimstone
    };

    for (int i = 0; i < 10; ++i) {
        float angle = (i / 10.0f) * 6.28318f;
        float dist = 8.0f + (rand() % 40);
        float x = playerPos.x + cos(angle) * dist;
        float z = playerPos.z + sin(angle) * dist;
        float y = WorldGenerator::GetHeight(x, z) + 0.8f + (rand() % 100) * 0.015f;

        Butterfly b;
        b.origin = glm::vec3(x, y, z);
        b.pos = b.origin;
        b.flightAngle = (float)(rand() % 360);
        b.flightRadius = 1.2f + (rand() % 100) * 0.02f;
        b.wingTimer = (float)(rand() % 100) * 0.1f;
        b.wingSpeed = 16.0f + (rand() % 10);
        b.color = butterflyColors[i % 5];
        m_butterflies.push_back(b);
    }

    // 2. Spawn Fireflies
    glm::vec3 fireflyColors[] = {
        glm::vec3(0.75f, 1.00f, 0.25f), // Bright Chartreuse
        glm::vec3(0.30f, 0.95f, 0.85f), // Cyan Spark
        glm::vec3(0.95f, 0.90f, 0.35f), // Golden Glow
        glm::vec3(0.40f, 1.00f, 0.50f)  // Emerald Glow
    };

    for (int i = 0; i < 16; ++i) {
        float angle = (float)(rand() % 360) * 0.01745f;
        float dist = 6.0f + (rand() % 45);
        float x = playerPos.x + cos(angle) * dist;
        float z = playerPos.z + sin(angle) * dist;
        float y = WorldGenerator::GetHeight(x, z) + 0.6f + (rand() % 100) * 0.025f;

        Firefly f;
        f.origin = glm::vec3(x, y, z);
        f.pos = f.origin;
        f.phaseOffset = (float)(rand() % 100) * 0.1f;
        f.glowTimer = (float)(rand() % 100) * 0.1f;
        f.color = fireflyColors[i % 4];
        f.driftSpeed = 0.4f + (rand() % 100) * 0.005f;
        m_fireflies.push_back(f);
    }

    // 3. Spawn Frogs / Sapos near grass and water
    for (int i = 0; i < 6; ++i) {
        float angle = (float)(rand() % 360) * 0.01745f;
        float dist = 8.0f + (rand() % 35);
        float x = playerPos.x + cos(angle) * dist;
        float z = playerPos.z + sin(angle) * dist;
        float y = WorldGenerator::GetHeight(x, z);

        Frog frog;
        frog.pos = glm::vec3(x, y, z);
        frog.startJumpPos = frog.pos;
        frog.targetJumpPos = frog.pos;
        frog.yaw = (float)(rand() % 360);
        frog.jumpTimer = 0.0f;
        frog.jumpDuration = 0.45f;
        frog.nextJumpTimer = 1.5f + (rand() % 100) * 0.035f;
        frog.isJumping = false;
        frog.croakTimer = (float)(rand() % 100) * 0.1f;
        m_frogs.push_back(frog);
    }
}

void CritterSystem::Update(float deltaTime, glm::vec3 playerPos) {
    if (m_butterflies.empty()) {
        Init(playerPos);
    }

    // 1. Update Butterflies
    for (auto& b : m_butterflies) {
        b.flightAngle += deltaTime * 1.8f;
        b.wingTimer += deltaTime * b.wingSpeed;

        float ox = cos(b.flightAngle) * b.flightRadius;
        float oz = sin(b.flightAngle * 1.3f) * b.flightRadius;
        float oy = sin(b.flightAngle * 2.5f) * 0.35f;

        b.pos = b.origin + glm::vec3(ox, oy, oz);

        // Respawn if too far from player
        if (glm::distance(b.pos, playerPos) > 65.0f) {
            float a = (float)(rand() % 360) * 0.01745f;
            float d = 10.0f + (rand() % 35);
            b.origin.x = playerPos.x + cos(a) * d;
            b.origin.z = playerPos.z + sin(a) * d;
            b.origin.y = WorldGenerator::GetHeight(b.origin.x, b.origin.z) + 0.8f + (rand() % 100) * 0.015f;
            b.pos = b.origin;
        }
    }

    // 2. Update Fireflies
    for (auto& f : m_fireflies) {
        f.glowTimer += deltaTime * 2.8f;
        f.phaseOffset += deltaTime * f.driftSpeed;

        float dx = sin(f.phaseOffset) * 0.8f;
        float dz = cos(f.phaseOffset * 0.8f) * 0.8f;
        float dy = sin(f.phaseOffset * 1.5f) * 0.45f;

        f.pos = f.origin + glm::vec3(dx, dy, dz);

        // Respawn if too far
        if (glm::distance(f.pos, playerPos) > 70.0f) {
            float a = (float)(rand() % 360) * 0.01745f;
            float d = 8.0f + (rand() % 45);
            f.origin.x = playerPos.x + cos(a) * d;
            f.origin.z = playerPos.z + sin(a) * d;
            f.origin.y = WorldGenerator::GetHeight(f.origin.x, f.origin.z) + 0.6f + (rand() % 100) * 0.025f;
            f.pos = f.origin;
        }
    }

    // 3. Update Frogs
    for (auto& frog : m_frogs) {
        frog.croakTimer += deltaTime * 3.0f;

        float distToPlayer = glm::distance(frog.pos, playerPos);

        if (frog.isJumping) {
            frog.jumpTimer += deltaTime;
            float t = glm::clamp(frog.jumpTimer / frog.jumpDuration, 0.0f, 1.0f);
            
            // Linear horizontal movement + Parabolic vertical arc
            glm::vec3 horiz = glm::mix(frog.startJumpPos, frog.targetJumpPos, t);
            float arc = sin(t * 3.14159f) * 0.65f;
            float groundY = WorldGenerator::GetHeight(horiz.x, horiz.z);
            horiz.y = groundY + arc;
            frog.pos = horiz;

            if (t >= 1.0f) {
                frog.isJumping = false;
                frog.pos.y = WorldGenerator::GetHeight(frog.pos.x, frog.pos.z);
                frog.nextJumpTimer = 2.0f + (rand() % 100) * 0.03f;
            }
        } else {
            frog.nextJumpTimer -= deltaTime;
            
            // Hop spontaneously or flee if player gets too close
            if (frog.nextJumpTimer <= 0.0f || distToPlayer < 3.5f) {
                frog.isJumping = true;
                frog.jumpTimer = 0.0f;
                frog.startJumpPos = frog.pos;

                // Jump direction: away from player if close, else random wander
                float jumpAngle = 0.0f;
                if (distToPlayer < 4.0f) {
                    glm::vec3 fleeDir = glm::normalize(frog.pos - playerPos);
                    jumpAngle = atan2(fleeDir.x, fleeDir.z) + ((rand() % 50 - 25) * 0.01745f);
                } else {
                    jumpAngle = (float)(rand() % 360) * 0.01745f;
                }

                frog.yaw = glm::degrees(jumpAngle);
                float jumpDist = 1.2f + (rand() % 100) * 0.015f;
                frog.targetJumpPos = frog.startJumpPos + glm::vec3(sin(jumpAngle) * jumpDist, 0.0f, cos(jumpAngle) * jumpDist);
                frog.targetJumpPos.y = WorldGenerator::GetHeight(frog.targetJumpPos.x, frog.targetJumpPos.z);
            }
        }

        // Respawn if too far
        if (distToPlayer > 60.0f) {
            float a = (float)(rand() % 360) * 0.01745f;
            float d = 10.0f + (rand() % 35);
            frog.pos.x = playerPos.x + cos(a) * d;
            frog.pos.z = playerPos.z + sin(a) * d;
            frog.pos.y = WorldGenerator::GetHeight(frog.pos.x, frog.pos.z);
            frog.startJumpPos = frog.pos;
            frog.targetJumpPos = frog.pos;
            frog.isJumping = false;
            frog.nextJumpTimer = 1.0f + (rand() % 100) * 0.02f;
        }
    }

    updateCritterMesh();
}

void CritterSystem::buildButterflyMesh(const Butterfly& b, std::vector<Vertex>& vertices) {
    float wingAngle = sin(b.wingTimer) * 0.95f;
    float cosW = cos(wingAngle), sinW = sin(wingAngle);
    float size = 0.14f;

    glm::vec3 center = b.pos;
    glm::vec3 col = b.color;
    glm::vec3 norm(0.0f, 1.0f, 0.0f);

    // Left Wing
    glm::vec3 lwTip = center + glm::vec3(-cosW * size * 1.5f, sinW * size * 0.8f, size * 0.5f);
    glm::vec3 lwFront = center + glm::vec3(-cosW * size * 0.8f, sinW * size * 0.4f, size * 1.2f);
    glm::vec3 lwBack = center + glm::vec3(-cosW * size * 0.8f, sinW * size * 0.4f, -size * 0.8f);

    vertices.push_back(Vertex{ center, col * 0.8f, glm::vec2(0.5f, 0.5f), norm });
    vertices.push_back(Vertex{ lwFront, col, glm::vec2(0.0f, 1.0f), norm });
    vertices.push_back(Vertex{ lwTip, col * 1.2f, glm::vec2(0.0f, 0.5f), norm });

    vertices.push_back(Vertex{ center, col * 0.8f, glm::vec2(0.5f, 0.5f), norm });
    vertices.push_back(Vertex{ lwTip, col * 1.2f, glm::vec2(0.0f, 0.5f), norm });
    vertices.push_back(Vertex{ lwBack, col, glm::vec2(0.0f, 0.0f), norm });

    // Right Wing
    glm::vec3 rwTip = center + glm::vec3(cosW * size * 1.5f, sinW * size * 0.8f, size * 0.5f);
    glm::vec3 rwFront = center + glm::vec3(cosW * size * 0.8f, sinW * size * 0.4f, size * 1.2f);
    glm::vec3 rwBack = center + glm::vec3(cosW * size * 0.8f, sinW * size * 0.4f, -size * 0.8f);

    vertices.push_back(Vertex{ center, col * 0.8f, glm::vec2(0.5f, 0.5f), norm });
    vertices.push_back(Vertex{ rwTip, col * 1.2f, glm::vec2(1.0f, 0.5f), norm });
    vertices.push_back(Vertex{ rwFront, col, glm::vec2(1.0f, 1.0f), norm });

    vertices.push_back(Vertex{ center, col * 0.8f, glm::vec2(0.5f, 0.5f), norm });
    vertices.push_back(Vertex{ rwBack, col, glm::vec2(1.0f, 0.0f), norm });
    vertices.push_back(Vertex{ rwTip, col * 1.2f, glm::vec2(1.0f, 0.5f), norm });
}

void CritterSystem::buildFireflyMesh(const Firefly& f, std::vector<Vertex>& vertices) {
    float pulse = 0.65f + sin(f.glowTimer) * 0.35f;
    float s = 0.06f * pulse;
    glm::vec3 col = f.color * (1.2f * pulse);
    glm::vec3 norm(0.0f, 1.0f, 0.0f);

    // Glowing Diamond / Cross Quad
    glm::vec3 p1 = f.pos + glm::vec3(-s, -s, 0.0f);
    glm::vec3 p2 = f.pos + glm::vec3( s, -s, 0.0f);
    glm::vec3 p3 = f.pos + glm::vec3( s,  s, 0.0f);
    glm::vec3 p4 = f.pos + glm::vec3(-s,  s, 0.0f);

    vertices.push_back(Vertex{ p1, col, glm::vec2(0,0), norm });
    vertices.push_back(Vertex{ p2, col, glm::vec2(1,0), norm });
    vertices.push_back(Vertex{ p3, col, glm::vec2(1,1), norm });
    vertices.push_back(Vertex{ p1, col, glm::vec2(0,0), norm });
    vertices.push_back(Vertex{ p3, col, glm::vec2(1,1), norm });
    vertices.push_back(Vertex{ p4, col, glm::vec2(0,1), norm });

    glm::vec3 q1 = f.pos + glm::vec3(0.0f, -s, -s);
    glm::vec3 q2 = f.pos + glm::vec3(0.0f, -s,  s);
    glm::vec3 q3 = f.pos + glm::vec3(0.0f,  s,  s);
    glm::vec3 q4 = f.pos + glm::vec3(0.0f,  s, -s);

    vertices.push_back(Vertex{ q1, col, glm::vec2(0,0), norm });
    vertices.push_back(Vertex{ q2, col, glm::vec2(1,0), norm });
    vertices.push_back(Vertex{ q3, col, glm::vec2(1,1), norm });
    vertices.push_back(Vertex{ q1, col, glm::vec2(0,0), norm });
    vertices.push_back(Vertex{ q3, col, glm::vec2(1,1), norm });
    vertices.push_back(Vertex{ q4, col, glm::vec2(0,1), norm });
}

void CritterSystem::buildFrogMesh(const Frog& frog, std::vector<Vertex>& vertices) {
    float rad = glm::radians(frog.yaw);
    float cosY = cos(rad), sinY = sin(rad);

    auto transformPoint = [&](glm::vec3 local) -> glm::vec3 {
        float x = local.x * cosY + local.z * sinY;
        float z = -local.x * sinY + local.z * cosY;
        return frog.pos + glm::vec3(x, local.y, z);
    };

    glm::vec3 colSkin(0.38f, 0.44f, 0.22f); // Forest Toad Green
    glm::vec3 colBelly(0.68f, 0.72f, 0.45f); // Pale Yellow-Green
    glm::vec3 colEye(0.95f, 0.75f, 0.15f); // Golden Iris
    glm::vec3 norm(0.0f, 1.0f, 0.0f);

    float breathe = sin(frog.croakTimer) * 0.02f;
    float sX = 0.18f + breathe;
    float sY = 0.12f + breathe;
    float sZ = 0.24f;

    // Body Box
    glm::vec3 b1 = transformPoint(glm::vec3(-sX, 0.02f, -sZ));
    glm::vec3 b2 = transformPoint(glm::vec3( sX, 0.02f, -sZ));
    glm::vec3 b3 = transformPoint(glm::vec3( sX, 0.02f + sY, -sZ));
    glm::vec3 b4 = transformPoint(glm::vec3(-sX, 0.02f + sY, -sZ));

    glm::vec3 f1 = transformPoint(glm::vec3(-sX, 0.02f, sZ));
    glm::vec3 f2 = transformPoint(glm::vec3( sX, 0.02f, sZ));
    glm::vec3 f3 = transformPoint(glm::vec3( sX, 0.02f + sY, sZ));
    glm::vec3 f4 = transformPoint(glm::vec3(-sX, 0.02f + sY, sZ));

    // Top face
    vertices.push_back(Vertex{ b4, colSkin, glm::vec2(0,0), norm });
    vertices.push_back(Vertex{ b3, colSkin, glm::vec2(1,0), norm });
    vertices.push_back(Vertex{ f3, colSkin, glm::vec2(1,1), norm });
    vertices.push_back(Vertex{ b4, colSkin, glm::vec2(0,0), norm });
    vertices.push_back(Vertex{ f3, colSkin, glm::vec2(1,1), norm });
    vertices.push_back(Vertex{ f4, colSkin, glm::vec2(0,1), norm });

    // Front/Head face
    vertices.push_back(Vertex{ f1, colBelly, glm::vec2(0,0), norm });
    vertices.push_back(Vertex{ f2, colBelly, glm::vec2(1,0), norm });
    vertices.push_back(Vertex{ f3, colSkin, glm::vec2(1,1), norm });
    vertices.push_back(Vertex{ f1, colBelly, glm::vec2(0,0), norm });
    vertices.push_back(Vertex{ f3, colSkin, glm::vec2(1,1), norm });
    vertices.push_back(Vertex{ f4, colSkin, glm::vec2(0,1), norm });

    // Left Eye
    glm::vec3 eL = transformPoint(glm::vec3(-0.12f, 0.16f, 0.16f));
    vertices.push_back(Vertex{ eL + glm::vec3(-0.03f, 0.0f, 0.0f), colEye, glm::vec2(0,0), norm });
    vertices.push_back(Vertex{ eL + glm::vec3( 0.03f, 0.0f, 0.0f), colEye, glm::vec2(1,0), norm });
    vertices.push_back(Vertex{ eL + glm::vec3( 0.0f, 0.05f, 0.0f), colEye, glm::vec2(0.5f,1), norm });

    // Right Eye
    glm::vec3 eR = transformPoint(glm::vec3(0.12f, 0.16f, 0.16f));
    vertices.push_back(Vertex{ eR + glm::vec3(-0.03f, 0.0f, 0.0f), colEye, glm::vec2(0,0), norm });
    vertices.push_back(Vertex{ eR + glm::vec3( 0.03f, 0.0f, 0.0f), colEye, glm::vec2(1,0), norm });
    vertices.push_back(Vertex{ eR + glm::vec3( 0.0f, 0.05f, 0.0f), colEye, glm::vec2(0.5f,1), norm });
}

void CritterSystem::updateCritterMesh() {
    std::vector<Vertex> rawVertices;
    rawVertices.reserve((m_butterflies.size() * 12) + (m_fireflies.size() * 12) + (m_frogs.size() * 18));

    for (const auto& b : m_butterflies) buildButterflyMesh(b, rawVertices);
    for (const auto& f : m_fireflies) buildFireflyMesh(f, rawVertices);
    for (const auto& frog : m_frogs) buildFrogMesh(frog, rawVertices);

    m_vertexCount = rawVertices.size();

    glBindBuffer(GL_ARRAY_BUFFER, m_critterVBO);
    glBufferData(GL_ARRAY_BUFFER, rawVertices.size() * sizeof(Vertex), rawVertices.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void CritterSystem::Render(GLuint shaderProgram) {
    if (m_vertexCount == 0 || m_critterVAO == 0) return;

    GLint modelLoc = glGetUniformLocation(shaderProgram, "u_Model");
    glm::mat4 model = glm::mat4(1.0f);
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsInstanced"), 0);
    glUniform1i(glGetUniformLocation(shaderProgram, "u_ConformToTerrain"), 0);
    glUniform1f(glGetUniformLocation(shaderProgram, "u_WindStrength"), 0.0f);
    glUniform1f(glGetUniformLocation(shaderProgram, "u_Alpha"), 1.0f);

    glBindVertexArray(m_critterVAO);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)m_vertexCount);
    glBindVertexArray(0);
}
