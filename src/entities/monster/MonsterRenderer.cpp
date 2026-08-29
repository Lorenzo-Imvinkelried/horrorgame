#include "Monster.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <algorithm>
#include <cmath>
#include "ModelLoader.h"
#include "WorldGenerator.h"

void Monster::BuildDeformedMesh() {
    m_animTime = 0.0f;
    AnimateMesh();
}

void Monster::AnimateMesh() {
    m_meshVertices.clear();
    m_eyeVertices.clear();
    
    std::vector<BoxDef> animatedBoxes = m_basePose;
    
    float speed = glm::length(m_velocity);
    float speedFac = glm::clamp(speed / m_speed, 0.0f, 2.0f);
    float cycle = m_animTime * 2.2f;
    
    float limbSwingL = sin(cycle) * 0.45f * speedFac;
    float limbSwingR = -sin(cycle) * 0.45f * speedFac;
    
    float armSwingL = limbSwingL;
    float armSwingR = limbSwingR;
    float bodyLean = 0.0f;
    float headYawRad = glm::radians(m_headYaw);
    float headPitchRad = glm::radians(m_headPitch);
    
    if (m_action == MonsterAction::CHASE) {
        armSwingL = -0.9f + sin(cycle * 2.0f) * 0.15f;
        armSwingR = -0.9f + cos(cycle * 2.0f) * 0.15f;
        bodyLean = 0.4f;
    } else if (m_action == MonsterAction::TRACK_SCENT) {
        armSwingL = 0.2f + sin(cycle) * 0.1f;
        armSwingR = 0.2f - sin(cycle) * 0.1f;
        bodyLean = 0.3f;
        headPitchRad = 0.45f + sin(cycle * 2.0f) * 0.1f;
    } else if (m_startleCooldownTimer > 9.0f) {
        armSwingL = 1.3f;
        armSwingR = 1.3f;
        bodyLean = -0.15f;
        headPitchRad = -0.4f;
    }
    
    float hipBend = 0.0f;
    float kneeBend = 0.0f;
    float heightOffset = 0.0f;
    if (m_crouchFactor > 0.001f) {
        bodyLean = glm::mix(bodyLean, 0.6f, m_crouchFactor);
        hipBend = -0.5f * m_crouchFactor;
        kneeBend = 0.9f * m_crouchFactor;
        heightOffset = -0.4f * m_crouchFactor;
    }

    glm::vec3 lShoulder(-0.4f, 2.0f + heightOffset, 0.4f);
    glm::vec3 rShoulder(0.4f, 2.0f + heightOffset, 0.4f);
    glm::vec3 lHip(-0.18f, 1.15f + heightOffset, 0.0f);
    glm::vec3 rHip(0.18f, 1.15f + heightOffset, 0.0f);
    glm::vec3 lKnee(-0.18f, 0.75f + heightOffset, 0.0f);
    glm::vec3 rKnee(0.18f, 0.75f + heightOffset, 0.0f);
    glm::vec3 neckPivot(0.0f, 2.16f + heightOffset, 0.5f);
    glm::vec3 hipsPivot(0.0f, 1.2f + heightOffset, 0.0f);
    
    auto rotatePivotX = [](glm::vec3 p, glm::vec3 pivot, float angle) {
        glm::vec3 local = p - pivot;
        float s = sin(angle), c = cos(angle);
        float ny = local.y * c - local.z * s;
        float nz = local.y * s + local.z * c;
        return pivot + glm::vec3(local.x, ny, nz);
    };
    
    auto rotateNeck = [](glm::vec3 p, glm::vec3 pivot, float yaw, float pitch) {
        glm::vec3 local = p - pivot;
        float sY = sin(yaw), cY = cos(yaw);
        float nx = local.x * cY - local.z * sY;
        float nz = local.x * sY + local.z * cY;
        local.x = nx; local.z = nz;
        
        float sP = sin(pitch), cP = cos(pitch);
        float ny = local.y * cP - local.z * sP;
        float nz2 = local.y * sP + local.z * cP;
        return pivot + glm::vec3(local.x, ny, nz2);
    };
    
    for (auto& box : animatedBoxes) {
        bool isUpperBody = true;
        
        if (box.Name.find("L_LEG") != std::string::npos) {
            box.Pos = rotatePivotX(box.Pos, lHip, limbSwingL + hipBend);
            box.Rot.x += limbSwingL + hipBend;
            isUpperBody = false;
        } 
        else if (box.Name.find("L_CALF") != std::string::npos) {
            float calfSwing = (limbSwingL < 0.0f) ? -limbSwingL * 1.2f : 0.0f;
            box.Pos = rotatePivotX(box.Pos, lKnee, calfSwing + kneeBend);
            box.Rot.x += calfSwing + kneeBend;
            box.Pos = rotatePivotX(box.Pos, lHip, limbSwingL + hipBend);
            box.Rot.x += limbSwingL + hipBend;
            isUpperBody = false;
        } 
        else if (box.Name.find("R_LEG") != std::string::npos) {
            box.Pos = rotatePivotX(box.Pos, rHip, limbSwingR + hipBend);
            box.Rot.x += limbSwingR + hipBend;
            isUpperBody = false;
        } 
        else if (box.Name.find("R_CALF") != std::string::npos) {
            float calfSwing = (limbSwingR < 0.0f) ? -limbSwingR * 1.2f : 0.0f;
            box.Pos = rotatePivotX(box.Pos, rKnee, calfSwing + kneeBend);
            box.Rot.x += calfSwing + kneeBend;
            box.Pos = rotatePivotX(box.Pos, rHip, limbSwingR + hipBend);
            box.Rot.x += limbSwingR + hipBend;
            isUpperBody = false;
        }
        else if (box.Name.find("HIPS") != std::string::npos) {
            isUpperBody = false;
        }
        else if (box.Name.find("L_ARM") != std::string::npos || 
                 box.Name.find("L_FOREARM") != std::string::npos ||
                 box.Name.find("L_HAND") != std::string::npos ||
                 box.Name.find("L_CLAW") != std::string::npos) 
        {
            box.Pos = rotatePivotX(box.Pos, lShoulder, armSwingL);
            box.Rot.x += armSwingL;
        }
        else if (box.Name.find("R_ARM") != std::string::npos || 
                 box.Name.find("R_FOREARM") != std::string::npos ||
                 box.Name.find("R_HAND") != std::string::npos ||
                 box.Name.find("R_CLAW") != std::string::npos) 
        {
            box.Pos = rotatePivotX(box.Pos, rShoulder, armSwingR);
            box.Rot.x += armSwingR;
        }
        else if (box.Name.find("HEAD") != std::string::npos || 
                 box.Name.find("EYE") != std::string::npos) 
        {
            box.Pos = rotateNeck(box.Pos, neckPivot, headYawRad, headPitchRad);
            box.Rot.y += headYawRad;
            box.Rot.x += headPitchRad;
        }
        
        if (isUpperBody) {
            box.Pos = rotatePivotX(box.Pos, hipsPivot, bodyLean);
            box.Rot.x += bodyLean;
        }
    }
    
    std::vector<BoxDef> bodyBoxes;
    std::vector<BoxDef> eyeBoxes;

    for (auto& box : animatedBoxes) {
        if (box.Name.find("EYE") != std::string::npos) {
            box.Color = m_eyeColor * m_eyeBrightness;
            eyeBoxes.push_back(box);
        } else {
            bodyBoxes.push_back(box);
        }
    }

    ModelLoader::GenerateMesh(bodyBoxes, m_meshVertices);
    ModelLoader::GenerateMesh(eyeBoxes, m_eyeVertices);
}

void Monster::Render(GLuint shaderProgram, GLuint whiteTexID) {
    GLint modelLoc = glGetUniformLocation(shaderProgram, "u_Model");
    
    glm::mat4 bodyModel = glm::mat4(1.0f);
    bodyModel = glm::translate(bodyModel, m_visualPos);
    bodyModel = glm::rotate(bodyModel, glm::radians(m_visualYaw), glm::vec3(0,1,0));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(bodyModel));
    
    // 1. Draw Body
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(m_meshVertices.size()/11)); 

    // 2. Draw Eyes
    glBindTexture(GL_TEXTURE_2D, whiteTexID);
    glBindVertexArray(VAO_Eyes);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(m_eyeVertices.size()/11));
}

void Monster::RenderDebug(GLuint shaderProgram) {
    GLint modelLoc = glGetUniformLocation(shaderProgram, "u_Model");
    
    glDisable(GL_DEPTH_TEST); 
    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsDebug"), 1); 
    
    glm::mat4 bodyModel = glm::mat4(1.0f);
    bodyModel = glm::translate(bodyModel, m_visualPos);
    bodyModel = glm::rotate(bodyModel, glm::radians(m_visualYaw), glm::vec3(0,1,0));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(bodyModel));
    
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(m_meshVertices.size() / 11));
    
    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsDebug"), 0);

    // Face vector line
    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsDebug"), 1);

    glm::vec3 headCenterLocal(0.0f, 2.4f, 0.25f);
    glm::mat4 bodyMat = glm::mat4(1.0f);
    bodyMat = glm::translate(bodyMat, m_visualPos);
    bodyMat = glm::rotate(bodyMat, glm::radians(m_visualYaw), glm::vec3(0,1,0));
    glm::vec3 headPos = glm::vec3(bodyMat * glm::vec4(headCenterLocal, 1.0f));

    float totalYaw = m_visualYaw + m_headYaw;
    glm::vec3 forwardDir(sin(glm::radians(totalYaw)), 0.0f, cos(glm::radians(totalYaw)));
    
    glm::vec3 lineEnd = headPos + forwardDir * 2.0f;

    std::vector<float> lineData = {
        headPos.x, headPos.y, headPos.z,   1,0,0,  0,0,0,
        lineEnd.x, lineEnd.y, lineEnd.z,   1,0,0,  0,0,0
    };

    GLuint lineVAO, lineVBO;
    glGenVertexArrays(1, &lineVAO);
    glGenBuffers(1, &lineVBO);
    glBindVertexArray(lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glBufferData(GL_ARRAY_BUFFER, lineData.size() * sizeof(float), lineData.data(), GL_STREAM_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
    glDrawArrays(GL_LINES, 0, 2);

    glDeleteVertexArrays(1, &lineVAO);
    glDeleteBuffers(1, &lineVBO);

    // Scent vectors
    if (glm::length(m_debugScentDir) > 0.1f) {
        glUniform1i(glGetUniformLocation(shaderProgram, "u_IsDebug"), 2);
        
        glm::vec3 sLineEnd = headPos + glm::normalize(m_debugScentDir) * 3.5f;
        std::vector<float> sLineData = {
            headPos.x, headPos.y, headPos.z,   0,1,1,  0,0,0,
            sLineEnd.x, sLineEnd.y, sLineEnd.z, 0,1,1,  0,0,0
        };

        GLuint sVAO, sVBO;
        glGenVertexArrays(1, &sVAO);
        glGenBuffers(1, &sVBO);
        glBindVertexArray(sVAO);
        glBindBuffer(GL_ARRAY_BUFFER, sVBO);
        glBufferData(GL_ARRAY_BUFFER, sLineData.size() * sizeof(float), sLineData.data(), GL_STREAM_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3*sizeof(float)));
        glEnableVertexAttribArray(1);

        glDrawArrays(GL_LINES, 0, 2);
        glDeleteVertexArrays(1, &sVAO);
        glDeleteBuffers(1, &sVBO);
    }

    // Detected Trees
    if (!m_detectedTrees.empty()) {
        std::vector<float> points;
        for (const auto& t : m_detectedTrees) {
            float y = WorldGenerator::GetHeight(t.x, t.z) + 1.0f;
            points.push_back(t.x); points.push_back(y); points.push_back(t.z);
            points.push_back(0.0f); points.push_back(1.0f); points.push_back(0.0f);
            points.push_back(0.0f); points.push_back(0.0f); points.push_back(0.0f);
        }

        GLuint pVAO, pVBO;
        glGenVertexArrays(1, &pVAO);
        glGenBuffers(1, &pVBO);
        glBindVertexArray(pVAO);
        glBindBuffer(GL_ARRAY_BUFFER, pVBO);
        glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(float), points.data(), GL_STREAM_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3*sizeof(float))); glEnableVertexAttribArray(1);

        glUniform1i(glGetUniformLocation(shaderProgram, "u_IsDebug"), 3);
#ifndef __EMSCRIPTEN__
        glPointSize(12.0f);
#endif
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
        glDrawArrays(GL_POINTS, 0, (GLsizei)m_detectedTrees.size());
#ifndef __EMSCRIPTEN__
        glPointSize(1.0f); 
#endif

        glDeleteVertexArrays(1, &pVAO);
        glDeleteBuffers(1, &pVBO);
        glUniform1i(glGetUniformLocation(shaderProgram, "u_IsDebug"), 0);
    }

    glEnable(GL_DEPTH_TEST);
}
