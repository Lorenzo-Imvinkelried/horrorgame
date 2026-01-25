#include "ModelLoader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

std::vector<BoxDef> ModelLoader::Load(const std::string& path) {
    std::vector<BoxDef> boxes;
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "ERROR: Could not open model file: " << path << std::endl;
        return boxes;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::stringstream ss(line);
        std::string type;
        ss >> type;

        if (type == "BOX") {
            BoxDef box;
            ss >> box.Pos.x >> box.Pos.y >> box.Pos.z;
            ss >> box.Scale.x >> box.Scale.y >> box.Scale.z;
            ss >> box.Rot.x >> box.Rot.y >> box.Rot.z;
            ss >> box.Color.r >> box.Color.g >> box.Color.b;
            ss >> box.Name; // Optional
            boxes.push_back(box);
        }
    }
    return boxes;
}

void ModelLoader::GenerateMesh(const std::vector<BoxDef>& boxes, std::vector<float>& outVertices) {
    // Unit Cube vertices [-0.5, 0.5] with UVs
    static const float cubeVertices[] = {
        // Positions          // Normals            // UVs
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
    };

    for (const auto& box : boxes) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, box.Pos);
        model = glm::rotate(model, box.Rot.z, glm::vec3(0,0,1));
        model = glm::rotate(model, box.Rot.y, glm::vec3(0,1,0));
        model = glm::rotate(model, box.Rot.x, glm::vec3(1,0,0));
        model = glm::scale(model, box.Scale);

        for (int i = 0; i < 36; i++) {
            glm::vec4 p(cubeVertices[i*8 + 0], cubeVertices[i*8 + 1], cubeVertices[i*8 + 2], 1.0f);
            glm::vec4 n(cubeVertices[i*8 + 3], cubeVertices[i*8 + 4], cubeVertices[i*8 + 5], 0.0f);
            float u = cubeVertices[i*8 + 6];
            float v = cubeVertices[i*8 + 7];

            glm::vec4 transP = model * p;
            glm::vec4 transN = model * n;
            
            outVertices.push_back(transP.x); outVertices.push_back(transP.y); outVertices.push_back(transP.z);
            outVertices.push_back(box.Color.r); outVertices.push_back(box.Color.g); outVertices.push_back(box.Color.b);
            outVertices.push_back(u); outVertices.push_back(v);
            outVertices.push_back(transN.x); outVertices.push_back(transN.y); outVertices.push_back(transN.z);
        }
    }
}
