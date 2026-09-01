#include "DebugDraw.h"
#include <cmath>

namespace DebugDraw {

void PushQuad(std::vector<float>& data, float x, float y, float w, float h) {
    // Formato 5 floats: (x, y, z, u, v)
    data.push_back(x);     data.push_back(y);     data.push_back(0.0f); data.push_back(0.0f); data.push_back(0.0f);
    data.push_back(x);     data.push_back(y+h);   data.push_back(0.0f); data.push_back(0.0f); data.push_back(0.0f);
    data.push_back(x+w);   data.push_back(y);     data.push_back(0.0f); data.push_back(0.0f); data.push_back(0.0f);

    data.push_back(x+w);   data.push_back(y);     data.push_back(0.0f); data.push_back(0.0f); data.push_back(0.0f);
    data.push_back(x);     data.push_back(y+h);   data.push_back(0.0f); data.push_back(0.0f); data.push_back(0.0f);
    data.push_back(x+w);   data.push_back(y+h);   data.push_back(0.0f); data.push_back(0.0f); data.push_back(0.0f);
}

void DrawArrow(float x, float y, float size, float angle, GLuint vao, GLuint vbo) {
    std::vector<float> data;
    auto pushRotated = [&](float lx, float ly) {
        float rx = lx * cos(angle) - ly * sin(angle);
        float ry = lx * sin(angle) + ly * cos(angle);
        data.push_back(x + rx * size);
        data.push_back(y + ry * size); 
        data.push_back(0.0f);
        data.push_back(0.0f);
        data.push_back(0.0f);
    };

    auto addTri = [&](float x1, float y1, float x2, float y2, float x3, float y3) {
        pushRotated(x1, y1); pushRotated(x2, y2); pushRotated(x3, y3);
    };

    addTri(0.5f, 0.0f, -0.2f, 0.3f, -0.2f, -0.3f);
    addTri(-0.5f, -0.1f, -0.2f, -0.1f, -0.2f, 0.1f);
    addTri(-0.5f, -0.1f, -0.2f, 0.1f, -0.5f, 0.1f);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_DYNAMIC_DRAW);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(data.size()/5));
    glBindVertexArray(0);
}

void DrawArrow3D(glm::vec3 pos, float size, float angle, glm::vec3 color, GLuint vao, GLuint vbo) {
    std::vector<float> data;
    auto push = [&](float lx, float lz) {
        float rx = lx * cos(angle) - lz * sin(angle);
        float rz = lx * sin(angle) + lz * cos(angle);
        
        data.push_back(pos.x + rx * size);
        data.push_back(pos.y);
        data.push_back(pos.z + rz * size);
        
        data.push_back(color.r); data.push_back(color.g); data.push_back(color.b);
        data.push_back(0); data.push_back(0);
        data.push_back(0); data.push_back(1); data.push_back(0);
    };

    push(1.0f, 0.0f); push(0.0f, 0.5f); push(0.0f, -0.5f);
    push(0.0f, 0.2f); push(-1.0f, 0.2f); push(-1.0f, -0.2f);
    push(0.0f, 0.2f); push(-1.0f, -0.2f); push(0.0f, -0.2f);
    
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_DYNAMIC_DRAW);
    glBindVertexArray(vao);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3*sizeof(float))); glEnableVertexAttribArray(1);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(8*sizeof(float))); glEnableVertexAttribArray(3);
    
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(data.size()/11));
    glBindVertexArray(0);
}

void DrawDonut(float cx, float cy, float cz, float rMin, float rMax, glm::vec3 color, GLuint vao, GLuint vbo) {
    std::vector<float> data;
    int segments = 64;
    float step = 6.2831853f / segments;

    for (int i = 0; i < segments; i++) {
        float theta1 = i * step;
        float theta2 = (i + 1) * step;

        float c1 = cos(theta1); float s1 = sin(theta1);
        float c2 = cos(theta2); float s2 = sin(theta2);
        
        auto addVert = [&](float r, float c, float s) {
            data.push_back(cx + c * r); data.push_back(cy); data.push_back(cz + s * r);
            data.push_back(color.r); data.push_back(color.g); data.push_back(color.b);
            data.push_back(0); data.push_back(0);
            data.push_back(0); data.push_back(1); data.push_back(0);
        };
        
        addVert(rMin, c1, s1);
        addVert(rMax, c1, s1);
        addVert(rMin, c2, s2);
        
        addVert(rMax, c1, s1);
        addVert(rMax, c2, s2);
        addVert(rMin, c2, s2);
    }

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_DYNAMIC_DRAW);
    glBindVertexArray(vao);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3*sizeof(float))); glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6*sizeof(float))); glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(8*sizeof(float))); glEnableVertexAttribArray(3);
    
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(data.size()/11));
    glBindVertexArray(0);
}

void DrawLine(glm::vec3 start, glm::vec3 end, glm::vec3 color, GLuint vao, GLuint vbo) {
    std::vector<float> data;
    data.push_back(start.x); data.push_back(start.y); data.push_back(start.z);
    data.push_back(color.r); data.push_back(color.g); data.push_back(color.b);
    data.push_back(0); data.push_back(0);
    data.push_back(0); data.push_back(1); data.push_back(0);
    
    data.push_back(end.x); data.push_back(end.y); data.push_back(end.z);
    data.push_back(color.r); data.push_back(color.g); data.push_back(color.b);
    data.push_back(0); data.push_back(0);
    data.push_back(0); data.push_back(1); data.push_back(0);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_DYNAMIC_DRAW);
    glBindVertexArray(vao);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3*sizeof(float))); glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6*sizeof(float))); glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(8*sizeof(float))); glEnableVertexAttribArray(3);
    
    glDrawArrays(GL_LINES, 0, 2);
    glBindVertexArray(0);
}

void DrawCone(glm::vec3 apex, glm::vec3 baseCenter, float baseRadius, glm::vec3 color, GLuint vao, GLuint vbo, int segments) {
    std::vector<float> data;
    glm::vec3 axis = glm::normalize(baseCenter - apex);
    glm::vec3 arbitrary = (std::abs(axis.y) < 0.99f) ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
    glm::vec3 u = glm::normalize(glm::cross(axis, arbitrary));
    glm::vec3 v = glm::normalize(glm::cross(axis, u));

    float step = 6.2831853f / segments;
    for (int i = 0; i < segments; ++i) {
        float t1 = i * step;
        float t2 = (i + 1) * step;

        glm::vec3 p1 = baseCenter + (u * cosf(t1) + v * sinf(t1)) * baseRadius;
        glm::vec3 p2 = baseCenter + (u * cosf(t2) + v * sinf(t2)) * baseRadius;

        auto addV = [&](glm::vec3 pos) {
            data.push_back(pos.x); data.push_back(pos.y); data.push_back(pos.z);
            data.push_back(color.r); data.push_back(color.g); data.push_back(color.b);
            data.push_back(0); data.push_back(0);
            data.push_back(0); data.push_back(1); data.push_back(0);
        };

        addV(apex);
        addV(p1);
        addV(p2);
    }

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_DYNAMIC_DRAW);
    glBindVertexArray(vao);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3*sizeof(float))); glEnableVertexAttribArray(1);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(data.size()/11));
    glBindVertexArray(0);
}

} // namespace DebugDraw
