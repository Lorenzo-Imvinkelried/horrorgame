#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <unordered_map>
#include "ModelLoader.h"
#include "ParticleSystem.h"
#include "DamageNumberSystem.h"
#include "ScentSystem.h"
#include "UIRenderer.h"

class Player;

enum class PeasantBodyState {
    HANGING,
    FALLING,
    ON_GROUND,
    LOOTED
};

struct PeasantCorpseInstance {
    glm::vec3 treePos;
    glm::vec3 anchorPos;
    glm::vec3 currentPos;
    float treeScale;
    PeasantBodyState state;
    float fallVelocity;
    bool scentEmitted;
    int loreIndex;
};

class HorrorPropsSystem {
public:
    HorrorPropsSystem();
    ~HorrorPropsSystem();

    void Update(float deltaTime, glm::vec3 playerPos, const std::vector<glm::vec4>& nearbyTrees, 
                ParticleSystem& particles, ScentSystem& scentSystem);

    bool CheckSwordCut(glm::vec3 attackPos, float range, ParticleSystem& particles);
    bool TryLootNearby(glm::vec3 playerPos, Player* player, DamageNumberSystem& damageNumbers, LoreDocumentModal& outModal);
    std::string GetNearbyPrompt(glm::vec3 playerPos);

    void Render(GLuint shaderProgram, const std::vector<glm::vec4>& nearbyTrees, float globalTime, glm::vec2 windDir);

private:
    void initMeshes();

    std::unordered_map<int, PeasantCorpseInstance> m_corpseRegistry;

    GLuint m_hangedVAO;
    GLuint m_hangedVBO;
    size_t m_hangedVertexCount;

    GLuint m_bodyOnlyVAO;
    GLuint m_bodyOnlyVBO;
    size_t m_bodyOnlyVertexCount;

    GLuint m_bloodPoolVAO;
    GLuint m_bloodPoolVBO;

    GLuint m_clawVAO;
    GLuint m_clawVBO;
    size_t m_clawVertexCount;
};
