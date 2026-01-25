#pragma once
#include <vector>
#include <string>
#include <glm/glm.hpp>

struct BoxDef {
    glm::vec3 Pos;
    glm::vec3 Scale;
    glm::vec3 Rot;
    glm::vec3 Color;
    std::string Name;
};

class ModelLoader {
public:
    static std::vector<BoxDef> Load(const std::string& path);
    static void GenerateMesh(const std::vector<BoxDef>& boxes, std::vector<float>& outVertices);
};
