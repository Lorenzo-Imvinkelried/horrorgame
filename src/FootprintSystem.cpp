#include "FootprintSystem.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "WorldGenerator.h"

FootprintSystem::FootprintSystem() {
    // 1. Generate Gradient Texture (Radial Falloff)
    int tw = 32, th = 32;
    std::vector<unsigned char> pixels(tw * th * 3);
    for(int y = 0; y < th; y++) {
        for(int x = 0; x < tw; x++) {
            float dx = (x - tw/2.0f) / (tw/2.0f);
            float dy = (y - th/2.0f) / (th/2.0f);
            float d = sqrt(dx*dx + dy*dy);
            float alpha = 1.0f - d;
            if(alpha < 0) alpha = 0;
            
            // For Multiply Blending: 1.0 (White) = No change, 0.0 (Black) = Darkest
            // We want a dark center fading to white edges
            unsigned char val = (unsigned char)(255 * (1.0f - alpha * 0.5f)); // Only darken by 50% max
            int idx = (y * tw + x) * 3;
            pixels[idx] = val;
            pixels[idx+1] = val;
            pixels[idx+2] = val;
        }
    }

    glGenTextures(1, &TexID);
    glBindTexture(GL_TEXTURE_2D, TexID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tw, th, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // 2. Vertex Setup
    // 2. Vertex Setup (Tessellated Grid for Terrain Conforming)
    std::vector<FullVert> verts;
    int gridDetail = 3; // 3x3 grid = 9 quads = 18 triangles
    float size = 0.4f;  // Total width (from -0.2 to 0.2)
    float start = -size/2.0f;
    float step = size / gridDetail;

    for(int z=0; z<gridDetail; z++) {
        for(int x=0; x<gridDetail; x++) {
            float x1 = start + x * step;
            float z1 = start + z * step;
            float x2 = x1 + step;
            float z2 = z1 + step;

            // UVs
            float u1 = (float)x / gridDetail;
            float v1 = (float)z / gridDetail;
            float u2 = (float)(x+1) / gridDetail;
            float v2 = (float)(z+1) / gridDetail;

            // Quad (2 Triangles, Matching Terrain Diagonal 01-10)
            // Terrain uses: (00, 01, 10) and (10, 01, 11)
            // Here: 00=(x1,z1), 01=(x1,z2), 10=(x2,z1), 11=(x2,z2)
            
            // Tri 1: 00 - 01 - 10
            verts.push_back({x1, 0, z1,  1,1,1, u1,v1, 0,1,0});
            verts.push_back({x1, 0, z2,  1,1,1, u1,v2, 0,1,0});
            verts.push_back({x2, 0, z1,  1,1,1, u2,v1, 0,1,0});

            // Tri 2: 10 - 01 - 11
            verts.push_back({x2, 0, z1,  1,1,1, u2,v1, 0,1,0});
            verts.push_back({x1, 0, z2,  1,1,1, u1,v2, 0,1,0});
            verts.push_back({x2, 0, z2,  1,1,1, u2,v2, 0,1,0});
        }
    }
    m_vertexCount = (int)verts.size();

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(FullVert), verts.data(), GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(FullVert), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(FullVert), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(FullVert), (void*)(6*sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(FullVert), (void*)(8*sizeof(float)));
    glEnableVertexAttribArray(3);
}

FootprintSystem::~FootprintSystem() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteTextures(1, &TexID);
}

void FootprintSystem::AddFootprint(glm::vec3 pos, float rotation) {
    Footprint fp;
    // Calculate precise mesh height
    float groundY = WorldGenerator::GetExactHeight(pos.x, pos.z);
    
    fp.pos = glm::vec3(pos.x, groundY + 0.02f, pos.z); // Fixed 2cm bias from precise surface
    fp.rotation = rotation;
    fp.life = 10.0f;
    
    footprints.push_back(fp);
    if(footprints.size() > maxFootprints) {
        footprints.erase(footprints.begin());
    }
}

void FootprintSystem::Update(float deltaTime) {
    for(auto it = footprints.begin(); it != footprints.end(); ) {
        it->life -= deltaTime;
        if(it->life <= 0.0f) it = footprints.erase(it);
        else ++it;
    }
}

void FootprintSystem::Render(GLuint shaderProgram) {
    if(footprints.empty()) return;

    glBindVertexArray(VAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, TexID);
    
    // MULTIPLY BLENDING: Darkens the destination
    glEnable(GL_BLEND);
    glBlendFunc(GL_DST_COLOR, GL_ZERO); 
    
    // POLYGON OFFSET: Pull depth closer to camera to prevent floating while avoiding z-fighting
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-2.5f, -2.5f); // Strong bias towards camera

    GLint modelLoc = glGetUniformLocation(shaderProgram, "u_Model");
    GLint instancedLoc = glGetUniformLocation(shaderProgram, "u_IsInstanced");
    glUniform1i(instancedLoc, 0);
    
    for(const auto& fp : footprints) {
        // As footprint fades, we want it to go towards White (No multiply effect)
        // However, vertex color tweak would be needed. 
        // For PS1 feel, let's keep it simple: linear scale of color?
        // Actually, let's just draw them.
        
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, fp.pos);
        model = glm::rotate(model, glm::radians(fp.rotation), glm::vec3(0,1,0));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        
        glDrawArrays(GL_TRIANGLES, 0, m_vertexCount);
    }
    
    glDisable(GL_POLYGON_OFFSET_FILL);
    glDisable(GL_BLEND);
}
