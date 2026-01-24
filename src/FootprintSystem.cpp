#include "FootprintSystem.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

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
    struct FullVert {
        float x,y,z;
        float r,g,b;
        float u,v;
        float nx,ny,nz;
    };
    
    FullVert q[] = {
        { -0.2f, 0.0f, -0.2f,  1,1,1, 0,0, 0,1,0 },
        {  0.2f, 0.0f, -0.2f,  1,1,1, 1,0, 0,1,0 },
        {  0.2f, 0.0f,  0.2f,  1,1,1, 1,1, 0,1,0 },
        {  0.2f, 0.0f,  0.2f,  1,1,1, 1,1, 0,1,0 },
        { -0.2f, 0.0f,  0.2f,  1,1,1, 0,1, 0,1,0 },
        { -0.2f, 0.0f, -0.2f,  1,1,1, 0,0, 0,1,0 },
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(q), q, GL_STATIC_DRAW);
    
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
    fp.pos = pos;
    fp.pos.y += 0.01f; // Lower offset to look merged
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
        
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    
    glDisable(GL_BLEND);
}
