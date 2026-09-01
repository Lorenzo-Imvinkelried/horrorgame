#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

namespace DebugDraw {
    void PushQuad(std::vector<float>& data, float x, float y, float w, float h);
    void DrawArrow(float x, float y, float size, float angle, GLuint vao, GLuint vbo);
    void DrawArrow3D(glm::vec3 pos, float size, float angle, glm::vec3 color, GLuint vao, GLuint vbo);
    void DrawDonut(float cx, float cy, float cz, float rMin, float rMax, glm::vec3 color, GLuint vao, GLuint vbo);
    void DrawLine(glm::vec3 start, glm::vec3 end, glm::vec3 color, GLuint vao, GLuint vbo);
    void DrawCone(glm::vec3 apex, glm::vec3 baseCenter, float baseRadius, glm::vec3 color, GLuint vao, GLuint vbo, int segments = 16);
}
