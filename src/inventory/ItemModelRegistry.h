#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <unordered_map>

struct ItemMesh {
    GLuint vao = 0;
    GLuint vbo = 0;
    size_t vertexCount = 0;
};

class ItemModelRegistry {
public:
    static ItemModelRegistry& Get();

    ItemModelRegistry();
    ~ItemModelRegistry();

    void Init();
    const ItemMesh* GetMesh(const std::string& stringId) const;
    void RenderItemInSlot(const std::string& stringId,
                          int vpX, int vpY, int vpW, int vpH,
                          bool isHovered, float globalTime,
                          GLuint shaderProgram);

private:
    void registerBoxes(const std::string& stringId, const std::vector<struct BoxDef>& boxes);
    void buildAllItemModels();

    std::unordered_map<std::string, ItemMesh> m_meshes;
    ItemMesh m_defaultMesh;
    bool m_initialized = false;
};
