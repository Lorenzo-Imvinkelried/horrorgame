#include "ParticleSystem.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Config.h" // Added for distance scaling

ParticleSystem::ParticleSystem() {
    float quad[] = {
        // Pos        // Tex (Standard UV)
        -0.5f, -0.5f, 0.0f, 0.0f,
         0.5f, -0.5f, 1.0f, 0.0f,
         0.5f,  0.5f, 1.0f, 1.0f,
         0.5f,  0.5f, 1.0f, 1.0f,
        -0.5f,  0.5f, 0.0f, 1.0f,
        -0.5f, -0.5f, 0.0f, 0.0f
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    
    // Position (2D for billboard start, expanding in shader or model matrix)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Tex
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2*sizeof(float)));
    glEnableVertexAttribArray(2);
}

ParticleSystem::~ParticleSystem() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}

void ParticleSystem::SpawnParticle(glm::vec3 pos, glm::vec3 vel, glm::vec4 color, float size, float life, float gravity) {
    Particle p;
    p.pos = pos;
    p.velocity = vel;
    p.color = color;
    p.size = size;
    p.life = life;
    p.maxLife = life;
    p.gravity = gravity;
    particles.push_back(p);
}

void ParticleSystem::Update(float deltaTime) {
    for (auto it = particles.begin(); it != particles.end(); ) {
        it->life -= deltaTime;
        if (it->life <= 0.0f) {
            it = particles.erase(it);
            continue;
        }

        it->velocity.y += it->gravity * deltaTime;
        it->pos += it->velocity * deltaTime;
        ++it;
    }
}

void ParticleSystem::Render(GLuint shaderProgram, glm::vec3 camPos) {
    if (particles.empty()) return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE); // Particles don't write to depth buffer
    
    // We reuse the main shader, assuming u_Color and simple vertex transform
    // Note: To make them billboard, we need to construct the model matrix carefully
    // OR use a billboard shader. For simplicity in this codebase, we'll construct the Model matrix
    // to face the camera.
    
    glBindVertexArray(VAO);
    GLint modelLoc = glGetUniformLocation(shaderProgram, "u_Model");
    GLint instancedLoc = glGetUniformLocation(shaderProgram, "u_IsInstanced"); 
    GLint particleModeLoc = glGetUniformLocation(shaderProgram, "u_ParticleMode");
    GLint alphaLoc = glGetUniformLocation(shaderProgram, "u_Alpha");

    glUniform1i(instancedLoc, 0);
    glUniform1i(particleModeLoc, 1); // ENABLE PARTICLE MODE

    for (const auto& p : particles) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, p.pos);
        
        // DISTANCE SCALING
        float dist = glm::length(p.pos - camPos);
        float distScale = 1.0f;
        if (dist > Config::Graphics::DistantScaleStart) {
            distScale = 1.0f + (dist - Config::Graphics::DistantScaleStart) * Config::Graphics::DistantScaleFactor;
            if(distScale > Config::Graphics::DistantScaleMax) distScale = Config::Graphics::DistantScaleMax;
        }

        // Billboard alignment
        glm::mat4 viewLook = glm::lookAt(p.pos, camPos, glm::vec3(0,1,0));
        model = glm::inverse(viewLook); 
        model = glm::scale(model, glm::vec3(p.size * distScale)); 

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        
        // Pass Alpha and Color
        glUniform1f(alphaLoc, p.color.a);
        glVertexAttrib3f(1, p.color.r, p.color.g, p.color.b); 
        
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    
    glUniform1i(particleModeLoc, 0); // DISABLE PARTICLE MODE
    glUniform1f(alphaLoc, 1.0f);    // RESET ALPHA
    glDepthMask(GL_TRUE);          // Restore depth writing
    glDisable(GL_BLEND);
}
