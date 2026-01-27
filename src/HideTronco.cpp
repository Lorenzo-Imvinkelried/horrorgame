#include "HideTronco.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <iostream>
#include <cmath>

// =================================================================================================
// ARCHIVO: HideTronco.cpp
// DESCRIPCION: Implementacion de la IA de sigilo SIMPLIFICADA (Dot Product + State Machine)
// =================================================================================================

// Helper Functions
float GetYawFromVector(glm::vec3 dir) {
    return glm::degrees(atan2(dir.x, dir.z));
}

glm::vec3 GetVectorFromYaw(float yaw) {
    float rad = glm::radians(yaw);
    return glm::vec3(sin(rad), 0.0f, cos(rad));
}

// =================================================================================================
// MOVEMENT CONTROLLER
// =================================================================================================

float MovementController::CalculateAngleDiff(glm::vec3 forward, glm::vec3 toTarget) {
    float dot = glm::dot(glm::normalize(forward), glm::normalize(toTarget));
    return glm::degrees(acos(glm::clamp(dot, -1.0f, 1.0f)));
}

MovementController::MoveStatus MovementController::MoveTowards(glm::vec3& monsterPos, float& monsterYaw, glm::vec3 targetPos, float deltaTime) {
    glm::vec3 dir = targetPos - monsterPos;
    float dist = glm::length(dir);

    // 1. CHEQUEO DE LLEGADA
    if (dist < arrivalThreshold) {
        return MoveStatus::ARRIVED;
    }

    // 2. CALCULO DE GIRO
    glm::vec3 dirNorm = glm::normalize(dir);
    
    // Smooth LookAt (Calculamos el angulo en el que deberiamos estar)
    float desiredYaw = GetYawFromVector(dirNorm);
    float yawDiff = std::fmod(desiredYaw - monsterYaw, 360.0f);
    if (yawDiff > 180.0f) yawDiff -= 360.0f;
    else if (yawDiff < -180.0f) yawDiff += 360.0f;

    // Safety
    if (std::isnan(yawDiff) || std::isinf(yawDiff)) yawDiff = 0.0f;

    // 3. LOGICA DE ROTACION (Girar rapido si estamos muy desalineados)
    float rotSpeed = 150.0f * deltaTime; 
    
    // Si estamos MUY desalineados (>45 grados), giramos in-place
    if (abs(yawDiff) > alignThreshold) {
        monsterYaw += (yawDiff > 0 ? rotSpeed : -rotSpeed);
        return MoveStatus::ROTATING;
    } 
    
    // 4. MOVIMIENTO
    // Giramos suave mientras nos movemos
    monsterYaw += yawDiff * (deltaTime * 5.0f); // Smooth adjust
    return MoveStatus::MOVING;
}

// =================================================================================================
// HIDE TRONCO MAIN IMPLEMENTATION
// =================================================================================================

HideTronco::HideTronco() : VAO(0), VBO(0) {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    Reset();
}

void HideTronco::Reset() {
    m_state = State::SMELLING;
    m_stateTimer = 0.0f;
    m_hasTarget = false;
    m_currentTargetPos = glm::vec3(0.0f);
    std::cout << "[HideTronco] Reset State -> SMELLING" << std::endl;
}

// THE CORE LOGIC: Using Dot Product to fit the tree
int HideTronco::SelectBestTree(const std::vector<glm::vec4>& trees, glm::vec3 monsterPos, glm::vec3 scentDir) {
    if (trees.empty()) return -1;
    
    int bestIdx = -1;
    float maxDot = -2.0f; // Dot product range [-1, 1], so -2 is safe minimum

    // [ALGORITMO DE PRODUCTO PUNTO]
    // Queremos un arbol que este "en la direccion del vector olor".
    // Dot(DirToTree, ScentDir) sera 1.0 si esta perfectamente alineado.
    
    glm::vec2 flatScentDir = glm::normalize(glm::vec2(scentDir.x, scentDir.z));
    
    for (int i = 0; i < trees.size(); i++) {
        glm::vec3 tPos = glm::vec3(trees[i]);
        
        // Vector del Monstruo al Arbol
        glm::vec3 dirToTree = tPos - monsterPos;
        if (glm::length(dirToTree) < 3.0f) continue; // Skip self/too close (Avoid re-selecting current tree)
        
        glm::vec2 flatDirToTree = glm::normalize(glm::vec2(dirToTree.x, dirToTree.z));

        // CALCULO
        float dot = glm::dot(flatDirToTree, flatScentDir);

        // BONUS: Distancia?
        // El usuario quiere "el arbol que mejor se fitea usando producto punto".
        // Pero quizas deberiamos penalizar arboles muy lejanos si tienen el mismo angulo.
        // Por la descripcion del usuario: "escanea arboles y el arbol que mejor se fitea 
        // usando producto punto entre el vector direccion olor [y] vector direccion del mob al arbol".
        // Asi que Prioridad = Dot Product.
        
        if (dot > maxDot) {
            maxDot = dot;
            bestIdx = i;
        }
    }
    
    // Solo aceptamos si esta medianamente en frente (> 0.0 -> 90 grados)
    // Si el mejor arbol esta DETRAS nuestro (-1.0), mejor no ir hacia atras, 
    // pero el usuario pidio "el arbol que mejor se fitea", asi que confiamos en el maxDot.
    
    return bestIdx;
}

glm::vec3 HideTronco::Update(glm::vec3 monsterPos, float& monsterYaw, glm::vec3 scentDir, ChunkManager& chunkManager, float deltaTime) {
    
    if (glm::length(scentDir) < 0.001f) {
        // No scent? Idle.
        // return monsterPos; 
    }

    // STATE MACHINE
    switch (m_state) {
        
        // --- 1. DETECTAR OLOR (SMELLING) ---
        case State::SMELLING: {
            // "El mob detecta el olor... escanea arboles"
            
            // 1. Check Distance (Chase Mode?)
            // We assume scentDir points to the Strongest Node.
            // But we don't know distance here easily unless passed. 
            // We'll rely on Monster.cpp to handle "Close Range Chase".
            // Here we just do hiding logic.

            std::vector<glm::vec4> trees;
            chunkManager.GetTreesInRange(monsterPos, m_searchRadius, trees);
            
            // 2. Select Tree using Dot Product
            int treeIdx = SelectBestTree(trees, monsterPos, scentDir);
            
            if (treeIdx != -1) {
                // FOUND TREE
                m_currentTargetPos = glm::vec3(trees[treeIdx]);
                m_hasTarget = true;
                
                // Transition -> MOVING
                m_state = State::MOVING;
                std::cout << "[HideTronco] Tree Found via Dot Product. Moving." << std::endl;
            } else {
                // NO TREE FOUND
                // Fallback: Just move in scent direction a bit?
                // User said: "busca el siguiente tronco y asi". If no trunk?
                // Let's just walk in scent direction for a bit to find new chunks.
                m_currentTargetPos = monsterPos + scentDir * 10.0f;
                m_hasTarget = true;
                m_state = State::MOVING;
                 std::cout << "[HideTronco] No Tree. Finding Forward." << std::endl;
            }
            break;
        }

        // --- 2. MOVERSE (MOVING) ---
        case State::MOVING: {
            // "Va caminando hacia ese tronco, se bloquea y no uele mas"
            // We do NOT check for new trees here. Locked in.

            auto status = m_movement.MoveTowards(monsterPos, monsterYaw, m_currentTargetPos, deltaTime);
            
            if (status == MovementController::MoveStatus::ARRIVED) {
                // ARRIVED AT TREE
                m_state = State::WAITING;
                m_stateTimer = m_waitDuration; // 1s or 2s
                m_hasTarget = false; // We reached it
                // std::cout << "[HideTronco] Arrived. Waiting." << std::endl;
                return monsterPos; // Stop
            } else if (status == MovementController::MoveStatus::ROTATING) {
                return monsterPos; // Rotate in place
            } else {
                return m_currentTargetPos; // Move
            }
            break;
        }

        // --- 3. ESPERAR (WAITING) ---
        case State::WAITING: {
            // "luego pasa 1 segundo busca el siguiente tronco"
            m_stateTimer -= deltaTime;
            if (m_stateTimer <= 0.0f) {
                m_state = State::SMELLING; // Loop
            }
            return monsterPos; // Stay put
        }
        
        case State::CHASING: {
             // Not used internally yet, handled by Monster.cpp override usually
             break;
        }
    }

    return monsterPos;
}

void HideTronco::RenderDebug(GLuint shaderProgram, glm::vec3 monsterPos, glm::vec3 scentDir) {
    if (!m_hasTarget) return;

    std::vector<float> lines;
    
    // 1. Line to Target Tree (Green)
    glm::vec3 p1 = monsterPos + glm::vec3(0, 1.5f, 0);
    glm::vec3 p2 = m_currentTargetPos + glm::vec3(0, 1.5f, 0); 
    
    lines.push_back(p1.x); lines.push_back(p1.y); lines.push_back(p1.z); // Pos
    lines.push_back(0); lines.push_back(1); lines.push_back(0); // Col (Green)
    lines.push_back(0); lines.push_back(0); lines.push_back(0); // Norm

    lines.push_back(p2.x); lines.push_back(p2.y); lines.push_back(p2.z);
    lines.push_back(0); lines.push_back(1); lines.push_back(0);
    lines.push_back(0); lines.push_back(0); lines.push_back(0);
    
    // 2. Scent Direction visualized (Red small line)
    glm::vec3 pScentEnd = p1 + scentDir * 5.0f;
    lines.push_back(p1.x); lines.push_back(p1.y); lines.push_back(p1.z); 
    lines.push_back(1); lines.push_back(0); lines.push_back(0); // Red
    lines.push_back(0); lines.push_back(0); lines.push_back(0);

    lines.push_back(pScentEnd.x); lines.push_back(pScentEnd.y); lines.push_back(pScentEnd.z);
    lines.push_back(1); lines.push_back(0); lines.push_back(0);
    lines.push_back(0); lines.push_back(0); lines.push_back(0);

    // Draw
    GLint modelLoc = glGetUniformLocation(shaderProgram, "u_Model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));

    glDisable(GL_DEPTH_TEST); 
    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsDebug"), 0); // Use Vertex Colors

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, lines.size() * sizeof(float), lines.data(), GL_DYNAMIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0); 
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3*sizeof(float))); 
    glEnableVertexAttribArray(1);

    glLineWidth(4.0f);
    glDrawArrays(GL_LINES, 0, lines.size() / 9);
    glLineWidth(1.0f);

    glEnable(GL_DEPTH_TEST);
}
