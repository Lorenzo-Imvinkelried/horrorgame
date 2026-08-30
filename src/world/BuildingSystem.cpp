#include "BuildingSystem.h"
#include "ParticleSystem.h"
#include "WorldGenerator.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>

BuildingSystem::BuildingSystem() : m_nextId(1) {
    for (int i = 0; i < 3; ++i) {
        m_VAO[i] = 0;
        m_VBO[i] = 0;
        m_instanceVBO[i] = 0;
        m_vertexCounts[i] = 0;
    }
}

BuildingSystem::~BuildingSystem() {
    for (int i = 0; i < 3; ++i) {
        if (m_VAO[i]) glDeleteVertexArrays(1, &m_VAO[i]);
        if (m_VBO[i]) glDeleteBuffers(1, &m_VBO[i]);
        if (m_instanceVBO[i]) glDeleteBuffers(1, &m_instanceVBO[i]);
    }
}

void BuildingSystem::Init() {
    buildMeshes();
}

void BuildingSystem::buildMeshes() {
    // -------------------------------------------------------------
    // 0. WALL (Pared de troncos y vigas de soporte)
    // -------------------------------------------------------------
    std::vector<BoxDef> wallBoxes;
    // Tablones centrales
    BoxDef mainWall;
    mainWall.Name = "WALL_MAIN";
    mainWall.Pos = glm::vec3(0.0f, 1.4f, 0.0f);
    mainWall.Scale = glm::vec3(3.2f, 2.8f, 0.32f);
    mainWall.Color = glm::vec3(0.36f, 0.27f, 0.17f);
    wallBoxes.push_back(mainWall);

    // Poste vertical izquierdo
    BoxDef postL;
    postL.Name = "POST_L";
    postL.Pos = glm::vec3(-1.50f, 1.4f, 0.0f);
    postL.Scale = glm::vec3(0.38f, 2.88f, 0.40f);
    postL.Color = glm::vec3(0.26f, 0.18f, 0.11f);
    wallBoxes.push_back(postL);

    // Poste vertical derecho
    BoxDef postR;
    postR.Name = "POST_R";
    postR.Pos = glm::vec3(1.50f, 1.4f, 0.0f);
    postR.Scale = glm::vec3(0.38f, 2.88f, 0.40f);
    postR.Color = glm::vec3(0.26f, 0.18f, 0.11f);
    wallBoxes.push_back(postR);

    // Viga superior horizontal
    BoxDef topBeam;
    topBeam.Name = "BEAM_TOP";
    topBeam.Pos = glm::vec3(0.0f, 2.72f, 0.0f);
    topBeam.Scale = glm::vec3(3.30f, 0.28f, 0.38f);
    topBeam.Color = glm::vec3(0.29f, 0.20f, 0.13f);
    wallBoxes.push_back(topBeam);

    // -------------------------------------------------------------
    // 1. ROOF / CAVE CEILING (Techo de tablones/piedra para refugios y cuevas)
    // -------------------------------------------------------------
    std::vector<BoxDef> roofBoxes;
    BoxDef roofSlab;
    roofSlab.Name = "ROOF_SLAB";
    roofSlab.Pos = glm::vec3(0.0f, 0.12f, 0.0f);
    roofSlab.Scale = glm::vec3(3.40f, 0.24f, 3.40f);
    roofSlab.Color = glm::vec3(0.33f, 0.25f, 0.16f);
    roofBoxes.push_back(roofSlab);

    BoxDef beamX;
    beamX.Name = "ROOF_BEAM_X";
    beamX.Pos = glm::vec3(0.0f, -0.06f, 0.0f);
    beamX.Scale = glm::vec3(3.35f, 0.18f, 0.32f);
    beamX.Color = glm::vec3(0.24f, 0.16f, 0.10f);
    roofBoxes.push_back(beamX);

    BoxDef beamZ;
    beamZ.Name = "ROOF_BEAM_Z";
    beamZ.Pos = glm::vec3(0.0f, -0.06f, 0.0f);
    beamZ.Scale = glm::vec3(0.32f, 0.18f, 3.35f);
    beamZ.Color = glm::vec3(0.24f, 0.16f, 0.10f);
    roofBoxes.push_back(beamZ);

    // -------------------------------------------------------------
    // 2. TORCH (Antorcha de suelo o pared con soporte y fuego)
    // -------------------------------------------------------------
    std::vector<BoxDef> torchBoxes;
    BoxDef torchPost;
    torchPost.Name = "TORCH_POST";
    torchPost.Pos = glm::vec3(0.0f, 0.45f, 0.0f);
    torchPost.Scale = glm::vec3(0.12f, 0.90f, 0.12f);
    torchPost.Color = glm::vec3(0.26f, 0.17f, 0.10f);
    torchBoxes.push_back(torchPost);

    BoxDef ironRing;
    ironRing.Name = "TORCH_RING";
    ironRing.Pos = glm::vec3(0.0f, 0.78f, 0.0f);
    ironRing.Scale = glm::vec3(0.22f, 0.12f, 0.22f);
    ironRing.Color = glm::vec3(0.20f, 0.20f, 0.22f);
    torchBoxes.push_back(ironRing);

    BoxDef flameCore;
    flameCore.Name = "TORCH_FLAME";
    flameCore.Pos = glm::vec3(0.0f, 0.94f, 0.0f);
    flameCore.Scale = glm::vec3(0.18f, 0.24f, 0.18f);
    flameCore.Color = glm::vec3(0.98f, 0.65f, 0.12f);
    torchBoxes.push_back(flameCore);

    // Generar mallas de OpenGL
    std::vector<std::vector<BoxDef>> allBoxes = { wallBoxes, roofBoxes, torchBoxes };
    for (int i = 0; i < 3; ++i) {
        std::vector<float> rawVertices;
        ModelLoader::GenerateMesh(allBoxes[i], rawVertices);
        m_vertexCounts[i] = (int)rawVertices.size() / 11;

        glGenVertexArrays(1, &m_VAO[i]);
        glGenBuffers(1, &m_VBO[i]);
        glBindVertexArray(m_VAO[i]);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO[i]);
        glBufferData(GL_ARRAY_BUFFER, rawVertices.size() * sizeof(float), rawVertices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(8 * sizeof(float)));
        glEnableVertexAttribArray(3);

        glGenBuffers(1, &m_instanceVBO[i]);
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

bool BuildingSystem::PlacePiece(BuildingType type, glm::vec3 pos, float yaw, ParticleSystem* particles) {
    BuildingPiece piece;
    piece.type = type;
    piece.pos = pos;
    piece.yaw = yaw;
    piece.id = m_nextId++;

    if (type == BuildingType::WALL) {
        piece.halfExtents = glm::vec3(1.60f, 1.40f, 0.20f);
    } else if (type == BuildingType::ROOF) {
        piece.halfExtents = glm::vec3(1.70f, 0.15f, 1.70f);
    } else { // TORCH
        piece.halfExtents = glm::vec3(0.20f, 0.50f, 0.20f);
    }

    m_pieces.push_back(piece);

    if (particles) {
        int count = (type == BuildingType::TORCH) ? 12 : 22;
        for (int i = 0; i < count; ++i) {
            glm::vec3 pVel((rand() % 100 / 50.0f - 1.0f) * 2.2f, 1.5f + (rand() % 100 / 50.0f) * 1.5f, (rand() % 100 / 50.0f - 1.0f) * 2.2f);
            glm::vec4 pCol = (type == BuildingType::TORCH) ? glm::vec4(0.95f, 0.65f, 0.15f, 1.0f) : glm::vec4(0.38f, 0.28f, 0.18f, 1.0f);
            particles->SpawnParticle(pos + glm::vec3(0, 0.5f, 0), pVel, pCol, 0.12f, 0.7f, -9.8f);
        }
    }

    return true;
}

bool BuildingSystem::RemovePieceAt(glm::vec3 aimPos, float maxDist, ParticleSystem* particles) {
    int bestIdx = -1;
    float bestDist = maxDist;

    for (size_t i = 0; i < m_pieces.size(); ++i) {
        float d = glm::distance(m_pieces[i].pos, aimPos);
        if (d < bestDist) {
            bestDist = d;
            bestIdx = (int)i;
        }
    }

    if (bestIdx != -1) {
        glm::vec3 dPos = m_pieces[bestIdx].pos;
        m_pieces.erase(m_pieces.begin() + bestIdx);

        if (particles) {
            for (int i = 0; i < 25; ++i) {
                glm::vec3 pVel((rand() % 100 / 50.0f - 1.0f) * 2.8f, 2.0f + (rand() % 100 / 50.0f) * 2.0f, (rand() % 100 / 50.0f - 1.0f) * 2.8f);
                particles->SpawnParticle(dPos + glm::vec3(0, 1.0f, 0), pVel, glm::vec4(0.30f, 0.22f, 0.15f, 1.0f), 0.14f, 0.8f, -9.8f);
            }
        }
        return true;
    }
    return false;
}

void BuildingSystem::Update(float deltaTime, glm::vec3 playerPos, ParticleSystem& particles) {
    // Generar fuego y humo para cada antorcha activa en el mundo
    for (const auto& piece : m_pieces) {
        if (piece.type != BuildingType::TORCH) continue;
        
        float distToPlayer = glm::distance(piece.pos, playerPos);
        if (distToPlayer > 80.0f) continue; // Culling de partículas lejanas

        if ((rand() % 100) < 45) {
            glm::vec3 flamePos = piece.pos + glm::vec3(
                (rand() % 100 / 50.0f - 1.0f) * 0.05f,
                0.95f + (rand() % 100 / 100.0f) * 0.08f,
                (rand() % 100 / 50.0f - 1.0f) * 0.05f
            );
            glm::vec3 flameVel(
                (rand() % 100 / 50.0f - 1.0f) * 0.15f,
                0.8f + (rand() % 100 / 50.0f) * 0.4f,
                (rand() % 100 / 50.0f - 1.0f) * 0.15f
            );
            glm::vec4 flameCol = ((rand() % 2) == 0) ? glm::vec4(0.98f, 0.72f, 0.15f, 0.9f) : glm::vec4(0.95f, 0.35f, 0.08f, 0.9f);
            particles.SpawnParticle(flamePos, flameVel, flameCol, 0.09f, 0.45f, 0.8f);
        }
    }
}

void BuildingSystem::Render(GLuint shaderProgram, glm::vec3 playerPos) {
    GLint modelLoc = glGetUniformLocation(shaderProgram, "u_Model");
    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsInstanced"), 0);
    glUniform1i(glGetUniformLocation(shaderProgram, "u_ConformToTerrain"), 0);

    for (const auto& piece : m_pieces) {
        int typeIdx = (int)piece.type;
        if (typeIdx < 0 || typeIdx > 2) continue;

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, piece.pos);
        model = glm::rotate(model, glm::radians(piece.yaw), glm::vec3(0.0f, 1.0f, 0.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        glBindVertexArray(m_VAO[typeIdx]);
        glDrawArrays(GL_TRIANGLES, 0, m_vertexCounts[typeIdx]);
    }
    glBindVertexArray(0);
}

void BuildingSystem::RenderGhost(GLuint shaderProgram, BuildingType type, glm::vec3 pos, float yaw, bool isValid, GLuint whiteTexID) {
    int typeIdx = (int)type;
    if (typeIdx < 0 || typeIdx > 2) return;

    GLint modelLoc = glGetUniformLocation(shaderProgram, "u_Model");
    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsInstanced"), 0);
    glUniform1i(glGetUniformLocation(shaderProgram, "u_ConformToTerrain"), 0);
    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsDebug"), 3); // Tint mode

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, pos);
    model = glm::rotate(model, glm::radians(yaw), glm::vec3(0.0f, 1.0f, 0.0f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    glBindVertexArray(m_VAO[typeIdx]);
    glDrawArrays(GL_TRIANGLES, 0, m_vertexCounts[typeIdx]);
    glBindVertexArray(0);

    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsDebug"), 0);
    glDisable(GL_BLEND);
}

bool BuildingSystem::CheckCollision(glm::vec3& entityPos, float radius, float height, glm::vec3& outPush) {
    outPush = glm::vec3(0.0f);
    bool collided = false;

    for (const auto& piece : m_pieces) {
        if (piece.type == BuildingType::TORCH) continue; // Antorchas no bloquean movimiento

        // Pared
        if (piece.type == BuildingType::WALL) {
            float rad = glm::radians(-piece.yaw);
            glm::vec3 localPos = entityPos - piece.pos;
            float cosY = cos(rad);
            float sinY = sin(rad);
            glm::vec3 rotatedLocal(
                localPos.x * cosY - localPos.z * sinY,
                localPos.y,
                localPos.x * sinY + localPos.z * cosY
            );

            float halfW = piece.halfExtents.x;
            float halfH = piece.halfExtents.y * 2.0f;
            float halfD = piece.halfExtents.z;

            if (rotatedLocal.y >= 0.0f && rotatedLocal.y <= halfH) {
                if (std::abs(rotatedLocal.x) < halfW + radius && std::abs(rotatedLocal.z) < halfD + radius) {
                    float overlapX = (halfW + radius) - std::abs(rotatedLocal.x);
                    float overlapZ = (halfD + radius) - std::abs(rotatedLocal.z);

                    if (overlapZ < overlapX) {
                        float signZ = (rotatedLocal.z > 0.0f) ? 1.0f : -1.0f;
                        rotatedLocal.z += signZ * overlapZ;
                    } else {
                        float signX = (rotatedLocal.x > 0.0f) ? 1.0f : -1.0f;
                        rotatedLocal.x += signX * overlapX;
                    }

                    // Rotar de regreso a coordenadas globales
                    float invRad = glm::radians(piece.yaw);
                    float cInv = cos(invRad);
                    float sInv = sin(invRad);
                    entityPos.x = piece.pos.x + (rotatedLocal.x * cInv - rotatedLocal.z * sInv);
                    entityPos.z = piece.pos.z + (rotatedLocal.x * sInv + rotatedLocal.z * cInv);
                    collided = true;
                }
            }
        }
        // Techo / Bóveda
        else if (piece.type == BuildingType::ROOF) {
            float halfW = piece.halfExtents.x;
            float halfL = piece.halfExtents.z;
            float topY = piece.pos.y + piece.halfExtents.y * 2.0f;
            float botY = piece.pos.y;

            if (std::abs(entityPos.x - piece.pos.x) < halfW && std::abs(entityPos.z - piece.pos.z) < halfL) {
                // Parado encima del techo
                if (entityPos.y >= botY - 0.2f && entityPos.y <= topY + 0.8f) {
                    entityPos.y = topY;
                    collided = true;
                }
            }
        }
    }

    return collided;
}

std::vector<glm::vec4> BuildingSystem::GetClosestTorches(glm::vec3 playerPos, int maxTorches) {
    std::vector<std::pair<float, glm::vec3>> torchList;
    for (const auto& piece : m_pieces) {
        if (piece.type == BuildingType::TORCH) {
            float d = glm::distance(piece.pos, playerPos);
            if (d < 45.0f) {
                torchList.push_back({ d, piece.pos + glm::vec3(0, 0.95f, 0) });
            }
        }
    }

    std::sort(torchList.begin(), torchList.end(), [](const auto& a, const auto& b) {
        return a.first < b.first;
    });

    std::vector<glm::vec4> result;
    for (size_t i = 0; i < torchList.size() && (int)result.size() < maxTorches; ++i) {
        result.push_back(glm::vec4(torchList[i].second, 1.0f));
    }
    return result;
}
