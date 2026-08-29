#include "HorrorPropsSystem.h"
#include "WorldGenerator.h"
#include "Player.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <cstdlib>
#include <algorithm>

HorrorPropsSystem::HorrorPropsSystem()
    : m_hangedVAO(0)
    , m_hangedVBO(0)
    , m_hangedVertexCount(0)
    , m_bodyOnlyVAO(0)
    , m_bodyOnlyVBO(0)
    , m_bodyOnlyVertexCount(0)
    , m_bloodPoolVAO(0)
    , m_bloodPoolVBO(0)
    , m_clawVAO(0)
    , m_clawVBO(0)
    , m_clawVertexCount(0)
{
    initMeshes();
}

HorrorPropsSystem::~HorrorPropsSystem() {
    if (m_hangedVAO) glDeleteVertexArrays(1, &m_hangedVAO);
    if (m_hangedVBO) glDeleteBuffers(1, &m_hangedVBO);
    if (m_bodyOnlyVAO) glDeleteVertexArrays(1, &m_bodyOnlyVAO);
    if (m_bodyOnlyVBO) glDeleteBuffers(1, &m_bodyOnlyVBO);
    if (m_bloodPoolVAO) glDeleteVertexArrays(1, &m_bloodPoolVAO);
    if (m_bloodPoolVBO) glDeleteBuffers(1, &m_bloodPoolVBO);
    if (m_clawVAO) glDeleteVertexArrays(1, &m_clawVAO);
    if (m_clawVBO) glDeleteBuffers(1, &m_clawVBO);
}

void HorrorPropsSystem::initMeshes() {
    // =========================================================================
    // 1. FULL HANGED CORPSE WITH ROPE
    // =========================================================================
    std::vector<BoxDef> hangedBoxes;
    hangedBoxes.push_back({ glm::vec3(0.0f, 3.4f, 0.0f), glm::vec3(0.035f, 3.8f, 0.035f), glm::vec3(0.0f), glm::vec3(0.55f, 0.48f, 0.35f), "ROPE_MAIN" });
    hangedBoxes.push_back({ glm::vec3(0.0f, 1.55f, 0.0f), glm::vec3(0.11f, 0.09f, 0.11f), glm::vec3(0.0f), glm::vec3(0.48f, 0.40f, 0.28f), "ROPE_KNOT" });
    hangedBoxes.push_back({ glm::vec3(0.0f, 1.40f, 0.06f), glm::vec3(0.24f, 0.28f, 0.24f), glm::vec3(0.35f, 0.0f, 0.0f), glm::vec3(0.65f, 0.62f, 0.58f), "HEAD" });
    hangedBoxes.push_back({ glm::vec3(0.0f, 1.52f, 0.04f), glm::vec3(0.26f, 0.14f, 0.26f), glm::vec3(0.35f, 0.0f, 0.0f), glm::vec3(0.18f, 0.14f, 0.12f), "HAIR" });
    hangedBoxes.push_back({ glm::vec3(0.0f, 0.95f, 0.0f), glm::vec3(0.44f, 0.58f, 0.28f), glm::vec3(0.0f), glm::vec3(0.35f, 0.30f, 0.22f), "TUNIC_TORSO" });
    hangedBoxes.push_back({ glm::vec3(0.0f, 0.64f, 0.0f), glm::vec3(0.46f, 0.16f, 0.30f), glm::vec3(0.0f), glm::vec3(0.30f, 0.25f, 0.18f), "TUNIC_LOWER" });
    hangedBoxes.push_back({ glm::vec3(-0.25f, 0.95f, -0.05f), glm::vec3(0.12f, 0.40f, 0.12f), glm::vec3(0.15f, 0.0f, 0.10f), glm::vec3(0.35f, 0.30f, 0.22f), "ARM_L" });
    hangedBoxes.push_back({ glm::vec3(0.25f, 0.95f, -0.05f), glm::vec3(0.12f, 0.40f, 0.12f), glm::vec3(0.15f, 0.0f, -0.10f), glm::vec3(0.35f, 0.30f, 0.22f), "ARM_R" });
    hangedBoxes.push_back({ glm::vec3(0.0f, 0.68f, -0.14f), glm::vec3(0.28f, 0.10f, 0.12f), glm::vec3(0.0f), glm::vec3(0.62f, 0.58f, 0.52f), "HANDS_TIED" });
    hangedBoxes.push_back({ glm::vec3(-0.12f, 0.46f, 0.0f), glm::vec3(0.17f, 0.44f, 0.17f), glm::vec3(0.0f), glm::vec3(0.24f, 0.20f, 0.16f), "THIGH_L" });
    hangedBoxes.push_back({ glm::vec3(0.12f, 0.46f, 0.0f), glm::vec3(0.17f, 0.44f, 0.17f), glm::vec3(0.0f), glm::vec3(0.24f, 0.20f, 0.16f), "THIGH_R" });
    hangedBoxes.push_back({ glm::vec3(-0.12f, 0.10f, 0.0f), glm::vec3(0.14f, 0.42f, 0.14f), glm::vec3(0.0f), glm::vec3(0.24f, 0.20f, 0.16f), "SHIN_L" });
    hangedBoxes.push_back({ glm::vec3(0.12f, 0.10f, 0.0f), glm::vec3(0.14f, 0.42f, 0.14f), glm::vec3(0.0f), glm::vec3(0.24f, 0.20f, 0.16f), "SHIN_R" });
    hangedBoxes.push_back({ glm::vec3(-0.12f, -0.15f, 0.04f), glm::vec3(0.12f, 0.10f, 0.20f), glm::vec3(0.30f, 0.0f, 0.0f), glm::vec3(0.60f, 0.56f, 0.50f), "FOOT_L" });
    hangedBoxes.push_back({ glm::vec3(0.12f, -0.15f, 0.04f), glm::vec3(0.12f, 0.10f, 0.20f), glm::vec3(0.30f, 0.0f, 0.0f), glm::vec3(0.60f, 0.56f, 0.50f), "FOOT_R" });

    std::vector<float> rawHanged;
    ModelLoader::GenerateMesh(hangedBoxes, rawHanged);
    m_hangedVertexCount = rawHanged.size() / 11;

    glGenVertexArrays(1, &m_hangedVAO);
    glGenBuffers(1, &m_hangedVBO);
    glBindVertexArray(m_hangedVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_hangedVBO);
    glBufferData(GL_ARRAY_BUFFER, rawHanged.size() * sizeof(float), rawHanged.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float))); glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6 * sizeof(float))); glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(8 * sizeof(float))); glEnableVertexAttribArray(3);
    glBindVertexArray(0);

    // =========================================================================
    // 2. FALLEN CORPSE (Without Rope)
    // =========================================================================
    std::vector<BoxDef> bodyOnlyBoxes;
    for (const auto& b : hangedBoxes) {
        if (b.Name != "ROPE_MAIN") {
            bodyOnlyBoxes.push_back(b);
        }
    }

    std::vector<float> rawBodyOnly;
    ModelLoader::GenerateMesh(bodyOnlyBoxes, rawBodyOnly);
    m_bodyOnlyVertexCount = rawBodyOnly.size() / 11;

    glGenVertexArrays(1, &m_bodyOnlyVAO);
    glGenBuffers(1, &m_bodyOnlyVBO);
    glBindVertexArray(m_bodyOnlyVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_bodyOnlyVBO);
    glBufferData(GL_ARRAY_BUFFER, rawBodyOnly.size() * sizeof(float), rawBodyOnly.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float))); glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6 * sizeof(float))); glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(8 * sizeof(float))); glEnableVertexAttribArray(3);
    glBindVertexArray(0);

    // =========================================================================
    // 3. BLOOD POOL DECAL
    // =========================================================================
    float poolVerts[] = {
        -0.95f, 0.0f, -0.95f,   0.38f, 0.03f, 0.03f,   0.0f, 0.0f,   0.0f, 1.0f, 0.0f,
         0.95f, 0.0f, -0.95f,   0.38f, 0.03f, 0.03f,   1.0f, 0.0f,   0.0f, 1.0f, 0.0f,
         0.95f, 0.0f,  0.95f,   0.38f, 0.03f, 0.03f,   1.0f, 1.0f,   0.0f, 1.0f, 0.0f,
        -0.95f, 0.0f, -0.95f,   0.38f, 0.03f, 0.03f,   0.0f, 0.0f,   0.0f, 1.0f, 0.0f,
         0.95f, 0.0f,  0.95f,   0.38f, 0.03f, 0.03f,   1.0f, 1.0f,   0.0f, 1.0f, 0.0f,
        -0.95f, 0.0f,  0.95f,   0.38f, 0.03f, 0.03f,   0.0f, 1.0f,   0.0f, 1.0f, 0.0f
    };

    glGenVertexArrays(1, &m_bloodPoolVAO);
    glGenBuffers(1, &m_bloodPoolVBO);
    glBindVertexArray(m_bloodPoolVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_bloodPoolVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(poolVerts), poolVerts, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float))); glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6 * sizeof(float))); glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(8 * sizeof(float))); glEnableVertexAttribArray(3);
    glBindVertexArray(0);

    // =========================================================================
    // 4. CLAW MARKED TRUNKS
    // =========================================================================
    std::vector<BoxDef> clawBoxes;
    clawBoxes.push_back({ glm::vec3(-0.12f, 0.15f, 0.0f), glm::vec3(0.018f, 0.65f, 0.020f), glm::vec3(0.0f, 0.0f, 0.08f), glm::vec3(0.12f, 0.03f, 0.02f), "CLAW_1" });
    clawBoxes.push_back({ glm::vec3(-0.04f, 0.00f, 0.0f), glm::vec3(0.020f, 0.75f, 0.020f), glm::vec3(0.0f, 0.0f, 0.04f), glm::vec3(0.10f, 0.02f, 0.02f), "CLAW_2" });
    clawBoxes.push_back({ glm::vec3(0.04f, -0.05f, 0.0f), glm::vec3(0.020f, 0.70f, 0.020f), glm::vec3(0.0f, 0.0f, -0.04f), glm::vec3(0.11f, 0.03f, 0.02f), "CLAW_3" });
    clawBoxes.push_back({ glm::vec3(0.12f, 0.08f, 0.0f), glm::vec3(0.018f, 0.60f, 0.020f), glm::vec3(0.0f, 0.0f, -0.08f), glm::vec3(0.12f, 0.03f, 0.02f), "CLAW_4" });
    clawBoxes.push_back({ glm::vec3(-0.04f, -0.45f, 0.005f), glm::vec3(0.012f, 0.30f, 0.015f), glm::vec3(0.0f), glm::vec3(0.42f, 0.04f, 0.04f), "BLOOD_1" });
    clawBoxes.push_back({ glm::vec3(0.04f, -0.48f, 0.005f), glm::vec3(0.012f, 0.25f, 0.015f), glm::vec3(0.0f), glm::vec3(0.38f, 0.04f, 0.04f), "BLOOD_2" });

    std::vector<float> rawClaws;
    ModelLoader::GenerateMesh(clawBoxes, rawClaws);
    m_clawVertexCount = rawClaws.size() / 11;

    glGenVertexArrays(1, &m_clawVAO);
    glGenBuffers(1, &m_clawVBO);
    glBindVertexArray(m_clawVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_clawVBO);
    glBufferData(GL_ARRAY_BUFFER, rawClaws.size() * sizeof(float), rawClaws.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float))); glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6 * sizeof(float))); glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(8 * sizeof(float))); glEnableVertexAttribArray(3);
    glBindVertexArray(0);
}

void HorrorPropsSystem::Update(float deltaTime, glm::vec3 playerPos, const std::vector<glm::vec4>& nearbyTrees, 
                               ParticleSystem& particles, ScentSystem& scentSystem) 
{
    // Register any new nearby hanging peasants
    for (const auto& t : nearbyTrees) {
        float h = sin(t.x * 47.193f + t.z * 83.719f) * 43758.5453f;
        int val = std::abs((int)(h * 1000.0f)) % 100;

        if (val < 12) {
            int treeKey = (int)(((long long)(t.x * 10.0f)) * 73856093 ^ ((long long)(t.z * 10.0f)) * 19349663);
            if (m_corpseRegistry.find(treeKey) == m_corpseRegistry.end()) {
                float branchAngle = (float)(val % 8) * 0.7853f;
                float branchDist = 1.1f * t.w;
                glm::vec3 anchorPos(
                    t.x + cos(branchAngle) * branchDist,
                    t.y + 5.2f * t.w,
                    t.z + sin(branchAngle) * branchDist
                );

                PeasantCorpseInstance corpse;
                corpse.treePos = glm::vec3(t.x, t.y, t.z);
                corpse.anchorPos = anchorPos;
                corpse.currentPos = anchorPos;
                corpse.treeScale = t.w;
                corpse.state = PeasantBodyState::HANGING;
                corpse.fallVelocity = 0.0f;
                corpse.scentEmitted = false;
                corpse.loreIndex = val % 4;

                m_corpseRegistry[treeKey] = corpse;
            }
        }
    }

    // Update dynamic falling bodies & blood pools
    for (auto& pair : m_corpseRegistry) {
        auto& corpse = pair.second;

        if (corpse.state == PeasantBodyState::FALLING) {
            corpse.fallVelocity += 9.8f * deltaTime * 2.2f;
            corpse.currentPos.y -= corpse.fallVelocity * deltaTime;

            float groundY = WorldGenerator::GetHeight(corpse.currentPos.x, corpse.currentPos.z);
            if (corpse.currentPos.y <= groundY + 0.35f) {
                corpse.currentPos.y = groundY + 0.15f;
                corpse.state = PeasantBodyState::ON_GROUND;

                // Explosion of blood and gore on impact
                for (int i = 0; i < 35; ++i) {
                    glm::vec3 pVel((rand()%100/50.0f - 1.0f)*3.8f, (rand()%100/50.0f + 0.2f)*3.2f, (rand()%100/50.0f - 1.0f)*3.8f);
                    particles.SpawnParticle(corpse.currentPos + glm::vec3(0, 0.2f, 0), pVel, glm::vec4(0.48f, 0.04f, 0.04f, 1.0f), 0.18f, 1.2f, -9.8f);
                }

                // Fresh blood puddle emits high intensity scent for Shadow Monsters!
                if (!corpse.scentEmitted) {
                    scentSystem.AddBloodScent(corpse.currentPos);
                    corpse.scentEmitted = true;
                }
            }
        }
        else if (corpse.state == PeasantBodyState::ON_GROUND) {
            // Ongoing vapor from blood pool
            if ((rand() % 100) < 15) {
                glm::vec3 steamPos = corpse.currentPos + glm::vec3((rand()%100/100.0f - 0.5f)*1.2f, 0.1f, (rand()%100/100.0f - 0.5f)*1.2f);
                particles.SpawnParticle(steamPos, glm::vec3(0.0f, 0.4f, 0.0f), glm::vec4(0.5f, 0.1f, 0.1f, 0.4f), 0.15f, 0.9f, 0.0f);
            }
        }
    }
}

bool HorrorPropsSystem::CheckSwordCut(glm::vec3 attackPos, float range, ParticleSystem& particles) {
    bool cutAny = false;

    for (auto& pair : m_corpseRegistry) {
        auto& corpse = pair.second;
        if (corpse.state == PeasantBodyState::HANGING) {
            // Check distance from player attack position to rope/corpse
            glm::vec3 bodyCenter = corpse.anchorPos - glm::vec3(0.0f, 2.0f, 0.0f);
            float dist = glm::distance(attackPos, bodyCenter);

            if (dist < (range + 1.8f)) {
                corpse.state = PeasantBodyState::FALLING;
                corpse.fallVelocity = 1.0f;
                cutAny = true;

                // Rope cut debris particles
                for (int i = 0; i < 20; ++i) {
                    glm::vec3 pVel((rand()%100/50.0f - 1.0f)*2.0f, (rand()%100/50.0f)*2.5f, (rand()%100/50.0f - 1.0f)*2.0f);
                    particles.SpawnParticle(bodyCenter, pVel, glm::vec4(0.55f, 0.48f, 0.35f, 1.0f), 0.12f, 0.7f, -9.8f);
                }
            }
        }
    }

    return cutAny;
}

std::string HorrorPropsSystem::GetNearbyPrompt(glm::vec3 playerPos) {
    for (const auto& pair : m_corpseRegistry) {
        const auto& corpse = pair.second;
        if (corpse.state == PeasantBodyState::ON_GROUND) {
            float dist = glm::distance(playerPos, corpse.currentPos);
            if (dist < 2.5f) {
                return "[E] REGISTRAR CADAVER";
            }
        }
    }
    return "";
}

bool HorrorPropsSystem::TryLootNearby(glm::vec3 playerPos, Player* player, DamageNumberSystem& damageNumbers, LoreDocumentModal& outModal) {
    for (auto& pair : m_corpseRegistry) {
        auto& corpse = pair.second;
        if (corpse.state == PeasantBodyState::ON_GROUND) {
            float dist = glm::distance(playerPos, corpse.currentPos);
            if (dist < 2.5f) {
                corpse.state = PeasantBodyState::LOOTED;

                // Grant rewards: EXP & Healing
                if (player != nullptr) {
                    bool leveledUp = false;
                    player->Stats.AddExp(60, leveledUp);
                    damageNumbers.SpawnExp(corpse.currentPos + glm::vec3(0, 1.0f, 0), 60);
                    if (leveledUp) damageNumbers.SpawnLevelUp(player->Position);

                    // Restore some HP
                    player->Stats.CurrentHP = std::min(player->Stats.CurrentHP + 35, player->Stats.MaxHP);
                }

                // Fill Lore Notepad Modal
                outModal.active = true;
                outModal.title = "REGISTRO_FORENSE.TXT";

                if (corpse.loreIndex == 0) {
                    outModal.line1 = "INFORME FORENSE #04: SUJETO 14.";
                    outModal.line2 = "SIN SIGNOS DE LUCHA PREVIA.";
                    outModal.line3 = "LOS CIERVOS DEL BOSQUE BEBIERON SU";
                    outModal.line4 = "SANGRE ANTES DEL AMANECER...";
                    outModal.rewardText = "+60 EXP | +35 HP (MEDICINA RECUPERADA)";
                } else if (corpse.loreIndex == 1) {
                    outModal.line1 = "NOTA ENCONTRADA EN EL BOLSILLO:";
                    outModal.line2 = "NO ENCIENDAS LA LINTERNA EN LA NIEBLA.";
                    outModal.line3 = "ELLOS NO VEN TU CUERPO, VEN TU LUZ";
                    outModal.line4 = "Y HUELEN TU MIEDO EN LA OSCURIDAD.";
                    outModal.rewardText = "+60 EXP | +35 HP (RECURSOS ENCONTRADOS)";
                } else if (corpse.loreIndex == 2) {
                    outModal.line1 = "DIARIO DEL GUARDABOSQUES:";
                    outModal.line2 = "LOS GIGANTES ANCESTRALES NO ATACAN";
                    outModal.line3 = "SI RESPETAS SUS VALLES. PERO DE NOCHE";
                    outModal.line4 = "EL ODIO ROJO CONSUME SU MENTE.";
                    outModal.rewardText = "+60 EXP | +35 HP (POCION VITAL)";
                } else {
                    outModal.line1 = "ADVERTENCIA FINAL:";
                    outModal.line2 = "SI EL LAGO GUARDA SILENCIO, HUYE.";
                    outModal.line3 = "EL DEVORADOR ESPERA SUMERGIDO A QUE";
                    outModal.line4 = "PISES LA ORILLA PARA ARRASTRARTE.";
                    outModal.rewardText = "+60 EXP | +35 HP (VENDAJES EXTRAIDOS)";
                }

                return true;
            }
        }
    }

    return false;
}

void HorrorPropsSystem::Render(GLuint shaderProgram, const std::vector<glm::vec4>& nearbyTrees, float globalTime, glm::vec2 windDir) {
    if (nearbyTrees.empty()) return;

    GLint modelLoc = glGetUniformLocation(shaderProgram, "u_Model");

    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsInstanced"), 0);
    glUniform1i(glGetUniformLocation(shaderProgram, "u_ConformToTerrain"), 0);
    glUniform1f(glGetUniformLocation(shaderProgram, "u_WindStrength"), 0.0f);
    glUniform1f(glGetUniformLocation(shaderProgram, "u_Alpha"), 1.0f);

    float windAngle = atan2(windDir.y, windDir.x);

    // =========================================================================
    // 1. RENDER CORPSES (Hanging, Falling, or Fallen on ground)
    // =========================================================================
    for (const auto& pair : m_corpseRegistry) {
        const auto& corpse = pair.second;

        if (corpse.state == PeasantBodyState::HANGING) {
            float sway = sin(globalTime * 1.5f + corpse.treePos.x * 0.35f) * 0.08f;

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, corpse.anchorPos);
            model = glm::rotate(model, windAngle, glm::vec3(0, 1, 0));
            model = glm::rotate(model, sway, glm::vec3(1, 0, 0));

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glBindVertexArray(m_hangedVAO);
            glDrawArrays(GL_TRIANGLES, 0, (GLsizei)m_hangedVertexCount);
        }
        else if (corpse.state == PeasantBodyState::FALLING) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, corpse.currentPos);
            model = glm::rotate(model, globalTime * 4.0f, glm::vec3(1, 0, 0)); // Tumbles in air

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glBindVertexArray(m_bodyOnlyVAO);
            glDrawArrays(GL_TRIANGLES, 0, (GLsizei)m_bodyOnlyVertexCount);
        }
        else if (corpse.state == PeasantBodyState::ON_GROUND || corpse.state == PeasantBodyState::LOOTED) {
            // Render Blood Pool Decal beneath body
            glm::mat4 poolModel = glm::mat4(1.0f);
            poolModel = glm::translate(poolModel, glm::vec3(corpse.currentPos.x, corpse.currentPos.y + 0.03f, corpse.currentPos.z));
            poolModel = glm::scale(poolModel, glm::vec3(1.4f));

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(poolModel));
            glBindVertexArray(m_bloodPoolVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            // Render Fallen Peasant Body lying flat on ground
            glm::mat4 bodyModel = glm::mat4(1.0f);
            bodyModel = glm::translate(bodyModel, corpse.currentPos);
            bodyModel = glm::rotate(bodyModel, glm::radians(90.0f), glm::vec3(1, 0, 0));
            bodyModel = glm::rotate(bodyModel, corpse.treePos.x, glm::vec3(0, 0, 1));

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(bodyModel));
            glBindVertexArray(m_bodyOnlyVAO);
            glDrawArrays(GL_TRIANGLES, 0, (GLsizei)m_bodyOnlyVertexCount);
        }
    }

    // =========================================================================
    // 2. RENDER CLAW MARKED TRUNKS
    // =========================================================================
    for (const auto& t : nearbyTrees) {
        float h = sin(t.x * 47.193f + t.z * 83.719f) * 43758.5453f;
        int val = std::abs((int)(h * 1000.0f)) % 100;

        if (val >= 12 && val < 32 && m_clawVAO != 0) {
            float trunkRadius = 0.35f * t.w;
            float markAngle = (float)(val % 6) * 1.047f;

            glm::vec3 markPos(
                t.x + cos(markAngle) * trunkRadius,
                t.y + 1.7f * t.w,
                t.z + sin(markAngle) * trunkRadius
            );

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, markPos);
            model = glm::rotate(model, -markAngle + 1.5708f, glm::vec3(0, 1, 0));
            model = glm::scale(model, glm::vec3(t.w * 0.95f));

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glBindVertexArray(m_clawVAO);
            glDrawArrays(GL_TRIANGLES, 0, (GLsizei)m_clawVertexCount);
        }
    }

    glBindVertexArray(0);
}
