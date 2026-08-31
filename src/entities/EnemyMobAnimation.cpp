#include "EnemyMob.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

void EnemyMob::updateModelMesh() {
    if (m_baseBoxes.empty() || m_VAO == 0) return;

    std::vector<TransformedBox> transformedBoxes;
    transformedBoxes.reserve(m_baseBoxes.size());

    float legSwing = (m_speed > 0.1f) ? sin(m_animTimer) * 0.48f : 0.0f;
    float armSwing = (m_speed > 0.1f) ? sin(m_animTimer) * 0.40f : sin(m_animTimer * 1.5f) * 0.05f;
    float torsoBob = (m_speed > 0.1f) ? std::abs(sin(m_animTimer * 2.0f)) * 0.04f : sin(m_animTimer * 1.2f) * 0.02f;

    // Pivotes de articulaciones según escala y anatomía
    glm::vec3 shoulderPivotL(-0.30f, 1.25f, 0.0f);
    glm::vec3 shoulderPivotR( 0.30f, 1.25f, 0.0f);
    glm::vec3 hipPivotL(-0.14f, 0.75f, 0.0f);
    glm::vec3 hipPivotR( 0.14f, 0.75f, 0.0f);

    if (m_type == EnemyType::NEUTRAL_GIANT) {
        shoulderPivotL = glm::vec3(-0.52f, 1.65f, 0.0f);
        shoulderPivotR = glm::vec3( 0.52f, 1.65f, 0.0f);
        hipPivotL = glm::vec3(-0.24f, 0.95f, 0.0f);
        hipPivotR = glm::vec3( 0.24f, 0.95f, 0.0f);
    } else if (m_type == EnemyType::TREANT) {
        shoulderPivotL = glm::vec3(-0.65f, 2.40f, 0.10f);
        shoulderPivotR = glm::vec3( 0.65f, 2.40f, 0.10f);
        hipPivotL = glm::vec3(-0.35f, 0.70f, 0.0f);
        hipPivotR = glm::vec3( 0.35f, 0.70f, 0.0f);
    } else if (m_type == EnemyType::BERSERKER_WARRIOR || m_type == EnemyType::DEATH_KNIGHT) {
        shoulderPivotL = glm::vec3(-0.36f, 1.25f, 0.0f);
        shoulderPivotR = glm::vec3( 0.36f, 1.25f, 0.0f);
        hipPivotL = glm::vec3(-0.16f, 0.75f, 0.0f);
        hipPivotR = glm::vec3( 0.16f, 0.75f, 0.0f);
    }

    // Cinemática de Ataque Melee (Windup -> Golpe Rasante -> Recuperación)
    float rightArmRotX = armSwing;
    float rightArmRotY = 0.0f;
    float rightArmRotZ = 0.0f;

    float leftArmRotX = -armSwing;
    float leftArmRotY = 0.0f;
    float leftArmRotZ = 0.0f;

    if (m_attackAnimProgress > 0.0f) {
        if (m_attackAnimProgress < 0.35f) {
            // Fase 1: Cargar arma hacia atrás y elevar el brazo (Windup)
            float t = m_attackAnimProgress / 0.35f;
            rightArmRotX = glm::mix(armSwing, -1.65f, t);
            rightArmRotY = glm::mix(0.0f, 0.50f, t);
            rightArmRotZ = glm::mix(0.0f, -0.30f, t);

            // Brazo izquierdo se prepara para equilibrar
            leftArmRotX = glm::mix(-armSwing, 0.65f, t);
        } else if (m_attackAnimProgress < 0.65f) {
            // Fase 2: Tajo descendente violento con el arma hacia adelante (Slash / Chop)
            float t = (m_attackAnimProgress - 0.35f) / 0.30f;
            rightArmRotX = glm::mix(-1.65f, 1.45f, t);
            rightArmRotY = glm::mix(0.50f, -0.60f, t);
            rightArmRotZ = glm::mix(-0.30f, 0.35f, t);

            leftArmRotX = glm::mix(0.65f, -0.85f, t);
        } else {
            // Fase 3: Retorno suave a la postura de guardia (Recovery)
            float t = (m_attackAnimProgress - 0.65f) / 0.35f;
            rightArmRotX = glm::mix(1.45f, armSwing, t);
            rightArmRotY = glm::mix(-0.60f, 0.0f, t);
            rightArmRotZ = glm::mix(0.35f, 0.0f, t);

            leftArmRotX = glm::mix(-0.85f, -armSwing, t);
        }
    }

    for (const auto& box : m_baseBoxes) {
        glm::mat4 M = glm::mat4(1.0f);
        M = glm::translate(M, box.Pos);
        M = glm::rotate(M, box.Rot.z, glm::vec3(0,0,1));
        M = glm::rotate(M, box.Rot.y, glm::vec3(0,1,0));
        M = glm::rotate(M, box.Rot.x, glm::vec3(1,0,0));
        M = glm::scale(M, box.Scale);

        glm::vec3 finalColor = box.Color;

        // Ojos brillantes espectrales
        if (box.Name == "EYE_L" || box.Name == "EYE_R") {
            float pulse = 1.3f + 0.5f * sin(m_eyePulse);
            if (m_type == EnemyType::NEUTRAL_GIANT && m_isEnraged) {
                finalColor = glm::vec3(1.0f, 0.25f, 0.05f) * pulse;
            } else {
                finalColor *= pulse;
            }
        }

        // Destello de golpe (Hit Flash)
        if (m_hitFlashTimer > 0.0f) {
            finalColor = glm::mix(finalColor, glm::vec3(1.0f, 0.95f, 0.95f), 0.65f);
        }

        // 1. Pierna Izquierda
        if (box.Name == "LEG_L") {
            glm::mat4 legM = glm::translate(glm::mat4(1.0f), hipPivotL) * glm::rotate(glm::mat4(1.0f), legSwing, glm::vec3(1,0,0)) * glm::translate(glm::mat4(1.0f), -hipPivotL) * M;
            transformedBoxes.push_back({ legM, finalColor });
        }
        // 2. Pierna Derecha
        else if (box.Name == "LEG_R") {
            glm::mat4 legM = glm::translate(glm::mat4(1.0f), hipPivotR) * glm::rotate(glm::mat4(1.0f), -legSwing, glm::vec3(1,0,0)) * glm::translate(glm::mat4(1.0f), -hipPivotR) * M;
            transformedBoxes.push_back({ legM, finalColor });
        }
        // 3. Brazo Izquierdo + Armas de mano izquierda
        else if (box.Name == "ARM_L" || box.Name == "SHIELD" || box.Name == "CLAW_L" || box.Name.find("AXE_L") != std::string::npos || box.Name.find("DAGGER_L") != std::string::npos || box.Name.find("BOW") != std::string::npos) {
            glm::mat4 armRot = glm::rotate(glm::mat4(1.0f), leftArmRotZ, glm::vec3(0,0,1)) * glm::rotate(glm::mat4(1.0f), leftArmRotY, glm::vec3(0,1,0)) * glm::rotate(glm::mat4(1.0f), leftArmRotX, glm::vec3(1,0,0));
            glm::mat4 armM = glm::translate(glm::mat4(1.0f), shoulderPivotL) * armRot * glm::translate(glm::mat4(1.0f), -shoulderPivotL) * M;
            transformedBoxes.push_back({ armM, finalColor });
        }
        // 4. Brazo Derecho + Armas de mano derecha (Espadas, Hachas, Mandobles, Garras, Mazas)
        else if (box.Name == "ARM_R" || box.Name.find("SWORD") != std::string::npos || box.Name.find("AXE_R") != std::string::npos || box.Name.find("CLAYMORE") != std::string::npos || box.Name.find("DAGGER_R") != std::string::npos || box.Name.find("ARROW") != std::string::npos || box.Name == "CLUB" || box.Name.find("STAFF") != std::string::npos || box.Name == "ORB" || box.Name == "CLAW_R") {
            glm::mat4 armRot = glm::rotate(glm::mat4(1.0f), rightArmRotZ, glm::vec3(0,0,1)) * glm::rotate(glm::mat4(1.0f), rightArmRotY, glm::vec3(0,1,0)) * glm::rotate(glm::mat4(1.0f), rightArmRotX, glm::vec3(1,0,0));
            glm::mat4 armM = glm::translate(glm::mat4(1.0f), shoulderPivotR) * armRot * glm::translate(glm::mat4(1.0f), -shoulderPivotR) * M;
            transformedBoxes.push_back({ armM, finalColor });
        }
        // 5. Capa
        else if (box.Name.find("CAPE") != std::string::npos) {
            float capeFlutter = (m_speed > 0.1f) ? sin(m_animTimer * 1.8f) * 0.18f : 0.04f;
            glm::mat4 capeM = glm::translate(glm::mat4(1.0f), box.Pos) * glm::rotate(glm::mat4(1.0f), capeFlutter, glm::vec3(1,0,0)) * glm::translate(glm::mat4(1.0f), -box.Pos) * M;
            transformedBoxes.push_back({ capeM, finalColor });
        }
        // 6. Torso / Cabeza con balanceo orgánico
        else {
            float attackTorsoTwist = (m_attackAnimProgress > 0.0f && m_attackAnimProgress < 0.65f) ? -0.22f : 0.0f;
            glm::mat4 bodyM = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, torsoBob, 0.0f)) * glm::rotate(glm::mat4(1.0f), attackTorsoTwist, glm::vec3(0,1,0)) * M;
            transformedBoxes.push_back({ bodyM, finalColor });
        }
    }

    std::vector<float> rawVertices;
    ModelLoader::GenerateMeshTransformed(transformedBoxes, rawVertices);
    m_vertexCount = rawVertices.size() / 11;

    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, rawVertices.size() * sizeof(float), rawVertices.data(), GL_DYNAMIC_DRAW);
}
