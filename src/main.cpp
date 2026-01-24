#include <glad/glad.h>
#include <SFML/Window.hpp>
#include <SFML/OpenGL.hpp>
#include <glm/glm.hpp>
#include <SFML/Window/Mouse.hpp>
#include "WeaponSystem.h"
#include "ParticleSystem.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <string>

#include "Monster.h"
#include "ScentManager.h"
#include "WorldGenerator.h"
#include "Player.h"
#include "WindSystem.h"
#include "FootprintSystem.h"
#include "ChunkManager.h"

// Shader loader helper
GLuint LoadShader(const char* vertPath, const char* fragPath) {
    auto loadFile = [](const char* path) {
        FILE* f = fopen(path, "rb");
        if(!f) return std::string("");
        fseek(f, 0, SEEK_END);
        long len = ftell(f);
        fseek(f, 0, SEEK_SET);
        char* buf = (char*)malloc(len + 1);
        fread(buf, 1, len, f);
        buf[len] = 0;
        std::string s(buf);
        free(buf);
        fclose(f);
        return s;
    };

    std::string vStr = loadFile(vertPath);
    std::string fStr = loadFile(fragPath);
    const char* vSrc = vStr.c_str();
    const char* fSrc = fStr.c_str();

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vSrc, NULL);
    glCompileShader(vs);

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fSrc, NULL);
    glCompileShader(fs);

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

// Helper to push a quad (2 triangles) to a buffer
void PushQuad(std::vector<float>& data, float x, float y, float w, float h) {
    // Triangle 1
    data.push_back(x);     data.push_back(y);     data.push_back(0);
    data.push_back(x);     data.push_back(y+h);   data.push_back(0);
    data.push_back(x+w);   data.push_back(y);     data.push_back(0);
    // Triangle 2
    data.push_back(x+w);   data.push_back(y);     data.push_back(0);
    data.push_back(x);     data.push_back(y+h);   data.push_back(0);
    data.push_back(x+w);   data.push_back(y+h);   data.push_back(0);
}

// Helper to draw a single digit using THICK QUADS (0-9)
void DrawDigitSolid(int d, float x, float y, float size, GLuint vao, GLuint vbo) {
    // Segments are now rectangles with width/thickness
    struct Seg { float x, y, w, h; };
    float t = 0.2f; // Thickness relative to size (0..1)
    
    // 7-segment layout (Horizontal: w=1, h=t. Vertical: w=t, h=0.5)
    //   0
    // 1   2
    //   3
    // 4   5
    //   6
    static const Seg segments[] = {
        {0,1-t, 1,t},    // 0 (Top)
        {0,0.5f, t,0.5f},// 1 (Top-Left)
        {1-t,0.5f, t,0.5f},// 2 (Top-Right)
        {0,0.5f-t/2, 1,t},// 3 (Middle)
        {0,0, t,0.5f},   // 4 (Bot-Left)
        {1-t,0, t,0.5f}, // 5 (Bot-Right)
        {0,0, 1,t}       // 6 (Bottom)
    };
    
    static const int digits[10][7] = {
        {1,1,1,0,1,1,1}, // 0
        {0,0,1,0,0,1,0}, // 1
        {1,0,1,1,1,0,1}, // 2
        {1,0,1,1,0,1,1}, // 3
        {0,1,1,1,0,1,0}, // 4
        {1,1,0,1,0,1,1}, // 5
        {1,1,0,1,1,1,1}, // 6
        {1,0,1,0,0,1,0}, // 7
        {1,1,1,1,1,1,1}, // 8
        {1,1,1,1,0,1,0}  // 9
    };

    std::vector<float> data;
    for(int i=0; i<7; i++) {
        if(digits[d][i]) {
            PushQuad(data, x + segments[i].x * size, y + segments[i].y * size, segments[i].w * size, segments[i].h * size);
        }
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_DYNAMIC_DRAW);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(data.size()/3));
}

// Helper to draw a rotating arrow
void DrawArrow(float x, float y, float size, float angle, GLuint vao, GLuint vbo) {
    // Arrow shape pointing Right (0 radians)
    // Triangle Head: (0.5, 0), (-0.2, 0.3), (-0.2, -0.3)
    // Shaft: (-0.5, 0.1), (-0.2, 0.1), (-0.2, -0.1), (-0.5, -0.1)
    
    std::vector<float> data;
    auto pushRotated = [&](float lx, float ly) {
        // Rotate
        float rx = lx * cos(angle) - ly * sin(angle);
        float ry = lx * sin(angle) + ly * cos(angle);
        // Translate & Scale
        data.push_back(x + rx * size);
        data.push_back(y + ry * size); 
        data.push_back(0.0f);
    };

    auto addTri = [&](float x1, float y1, float x2, float y2, float x3, float y3) {
        pushRotated(x1, y1); pushRotated(x2, y2); pushRotated(x3, y3);
    };

    // Head
    addTri(0.5f, 0.0f, -0.2f, 0.3f, -0.2f, -0.3f);
    // Shaft (2 tris)
    addTri(-0.5f, -0.1f, -0.2f, -0.1f, -0.2f, 0.1f);
    addTri(-0.5f, -0.1f, -0.2f, 0.1f, -0.5f, 0.1f);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_DYNAMIC_DRAW);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(data.size()/3));
}

int main() {
    sf::ContextSettings settings;
    settings.depthBits = 24;
    settings.stencilBits = 8;
    settings.antialiasingLevel = 0;
    settings.majorVersion = 3;
    settings.minorVersion = 3;
    settings.attributeFlags = sf::ContextSettings::Core;

    // Use Borderless Fullscreen
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    sf::Window window(desktop, "GamePS1Horror", sf::Style::None, settings);
    window.setVerticalSyncEnabled(true);
    window.setMouseCursorVisible(false);

    if (!gladLoadGL()) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, desktop.width, desktop.height);
    float aspect = (float)desktop.width / (float)desktop.height;

    GLuint shaderProgram = LoadShader("assets/shaders/ps1.vert", "assets/shaders/ps1.frag");
    GLuint uiProgram = LoadShader("assets/shaders/ui.vert", "assets/shaders/ui.frag");

    // Noise Texture
    std::vector<unsigned char> textureData = WorldGenerator::GenerateNoiseTexture(64, 64);
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 64, 64, 0, GL_RGB, GL_UNSIGNED_BYTE, textureData.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Create Soft Shadow Texture (Radial Gradient)
    int shadowSize = 64;
    std::vector<unsigned char> shadowData(shadowSize * shadowSize * 3);
    for (int y = 0; y < shadowSize; y++) {
        for (int x = 0; x < shadowSize; x++) {
            float dx = (float)x / shadowSize - 0.5f;
            float dy = (float)y / shadowSize - 0.5f;
            float dist = sqrt(dx*dx + dy*dy) * 2.0f; // 0 at center, 1 at edge
            
            // Multiply Logic (White=Transp, Grey=Darker)
            float value = glm::smoothstep(0.0f, 1.0f, dist); 
            value = 0.4f + 0.6f * value; // Center=0.4, Edge=1.0
            if (value > 1.0f) value = 1.0f;

            unsigned char val = (unsigned char)(value * 255);
            int idx = (y * shadowSize + x) * 3;
            shadowData[idx] = val;
            shadowData[idx+1] = val;
            shadowData[idx+2] = val;
        }
    }
    GLuint shadowTexID;
    glGenTextures(1, &shadowTexID);
    glBindTexture(GL_TEXTURE_2D, shadowTexID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, shadowSize, shadowSize, 0, GL_RGB, GL_UNSIGNED_BYTE, shadowData.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // White Texture for UI & Shadows
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // Fix alignment for 1x1 buffer
    GLuint whiteTexID;
    glGenTextures(1, &whiteTexID);
    glBindTexture(GL_TEXTURE_2D, whiteTexID);
    unsigned char whitePixel[3] = {255, 255, 255};
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, whitePixel);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Render Distance Increased (User Request)
    ChunkManager chunkManager(12); 

    // SKY COLOR (Clear Color)
    glClearColor(0.4f, 0.6f, 1.0f, 1.0f); // Sky Blue
    
    float globalTime = 0.0f;
    bool debugCam = false;
    glm::vec3 freeCamPos(0, 50, 0);
    glm::vec3 freeCamFront(0, -1, 0);
    float freeCamYaw = -90.0f;
    float freeCamPitch = -45.0f;

    // SKY COLOR (Clear Color)
    glClearColor(0.4f, 0.6f, 1.0f, 1.0f); // Sky Blue

    // Shadow Setup
    auto shadowMesh = WorldGenerator::GetShadowMesh();
    GLuint shadowVAO, shadowVBO;
    glGenVertexArrays(1, &shadowVAO);
    glGenBuffers(1, &shadowVBO);
    glBindVertexArray(shadowVAO);
    glBindBuffer(GL_ARRAY_BUFFER, shadowVBO);
    glBufferData(GL_ARRAY_BUFFER, shadowMesh.size() * sizeof(Vertex), shadowMesh.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position)); glEnableVertexAttribArray(0);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord)); glEnableVertexAttribArray(2); // Needed for gradient

    // Tree Setup (Structural Separation)
    // 1. TRUNK (Rigid)
    auto trunkMesh = WorldGenerator::GetTreeTrunkMesh();
    GLuint trunkVAO, trunkVBO;
    glGenVertexArrays(1, &trunkVAO);
    glGenBuffers(1, &trunkVBO);
    glBindVertexArray(trunkVAO);
    glBindBuffer(GL_ARRAY_BUFFER, trunkVBO);
    glBufferData(GL_ARRAY_BUFFER, trunkMesh.size() * sizeof(Vertex), trunkMesh.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position)); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord)); glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));   glEnableVertexAttribArray(3);
    
    // 2. LEAVES (Swaying)
    auto leavesMesh = WorldGenerator::GetTreeLeavesMesh();
    GLuint leavesVAO, leavesVBO;
    glGenVertexArrays(1, &leavesVAO);
    glGenBuffers(1, &leavesVBO);
    glBindVertexArray(leavesVAO);
    glBindBuffer(GL_ARRAY_BUFFER, leavesVBO);
    glBufferData(GL_ARRAY_BUFFER, leavesMesh.size() * sizeof(Vertex), leavesMesh.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position)); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord)); glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));   glEnableVertexAttribArray(3);

    // Instance Buffer (Shared)
    GLuint instanceVBO;
    glGenBuffers(1, &instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    
    // Setup Instance Attribute for Trunks
    glBindVertexArray(trunkVAO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glVertexAttribDivisor(4, 1); 

    // Setup Instance Attribute for Leaves
    glBindVertexArray(leavesVAO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glVertexAttribDivisor(4, 1); 

    // Setup Instance Attribute for Shadows
    glBindVertexArray(shadowVAO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glVertexAttribDivisor(4, 1); 

    // UI Setup
    GLuint uiVAO, uiVBO;
    glGenVertexArrays(1, &uiVAO);
    glGenBuffers(1, &uiVBO);
    glBindVertexArray(uiVAO);
    glBindBuffer(GL_ARRAY_BUFFER, uiVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // -- RETRO RESOLUTION SETUP --
    // Increased to 640x480 (VGA) for less extreme pixelation, while keeping fixed retro aspect
    int const INTERNAL_W = 640;
    int const INTERNAL_H = 480;
    float const INTERNAL_ASPECT = (float)INTERNAL_W / (float)INTERNAL_H;

    // FBO
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    // Texture Attachment
    GLuint texColorBuffer;
    glGenTextures(1, &texColorBuffer);
    glBindTexture(GL_TEXTURE_2D, texColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, INTERNAL_W, INTERNAL_H, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texColorBuffer, 0);

    // RBO (Depth/Stencil)
    GLuint rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, INTERNAL_W, INTERNAL_H);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Screen Quad Resources
    GLuint screenShader = LoadShader("assets/shaders/screen.vert", "assets/shaders/screen.frag");
    float quadVertices[] = { 
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };
    GLuint quadVAO, quadVBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    WindSystem windSystem;
    Player player(glm::vec3(0.0f, 10.0f, 0.0f));
    FootprintSystem footprints;
    WeaponSystem weapon;           // NEW
    ParticleSystem particles;
    ScentManager scentManager;
    Monster monster(glm::vec3(0)); 

    // Procedural Spawning ("The Donut")
    {
        float px = (float)(rand() % 300 - 150);
        float pz = (float)(rand() % 300 - 150);
        player.Position = glm::vec3(px, WorldGenerator::GetHeight(px, pz) + 1.0f, pz);
        
        float angle = (float)(rand() % 360) * 3.14159f / 180.0f;
        float dist = 150.0f + (float)(rand() % 100); 
        float mx = px + cos(angle) * dist;
        float mz = pz + sin(angle) * dist;
        monster.SetPosition(glm::vec3(mx, WorldGenerator::GetHeight(mx, mz), mz));
        monster.LookAt(player.Position);
    }

    sf::Clock clock;
    sf::Clock fpsClock;
    int frameCount = 0;
    int currentFPS = 0;

    while (window.isOpen()) {
        float deltaTime = clock.restart().asSeconds();
        if (deltaTime > 0.1f) deltaTime = 0.1f; 

        frameCount++;
        if (fpsClock.getElapsedTime().asSeconds() >= 1.0f) {
            currentFPS = frameCount;
            frameCount = 0;
            fpsClock.restart();
        }

        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) window.close();
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Escape) window.close();
                
                // Manual Wind Control (Arrow Keys)
                if (event.key.code == sf::Keyboard::Left)  windSystem.SetDirection(-1.0f, 0.0f);
                if (event.key.code == sf::Keyboard::Right) windSystem.SetDirection(1.0f, 0.0f);
                if (event.key.code == sf::Keyboard::Up)    windSystem.SetDirection(0.0f, -1.0f); // Up is Z-
                if (event.key.code == sf::Keyboard::Down)  windSystem.SetDirection(0.0f, 1.0f);  // Down is Z+
                
                // Debug Camera Toggle
                if (event.key.code == sf::Keyboard::F3) debugCam = !debugCam;
            }
        }

        chunkManager.Update(player.Position);
        scentManager.Update(player.Position, deltaTime);
        monster.Update(deltaTime, player.Position, player.Front, windSystem.GetDirection(), chunkManager, scentManager);
        
        if (!debugCam) {
            player.ProcessKeyboard(0, deltaTime, chunkManager, footprints);
            player.Update(deltaTime);
        } else {
            // Free Cam Movement
            float camSpeed = 20.0f * deltaTime;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) freeCamPos += freeCamFront * camSpeed;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) freeCamPos -= freeCamFront * camSpeed;
            glm::vec3 camRight = glm::normalize(glm::cross(freeCamFront, glm::vec3(0,1,0)));
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) freeCamPos -= camRight * camSpeed;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) freeCamPos += camRight * camSpeed;
        }

        if (window.hasFocus()) {
            sf::Vector2i mousePos = sf::Mouse::getPosition(window);
            sf::Vector2i center(desktop.width / 2, desktop.height / 2);
            sf::Mouse::setPosition(center, window);
            float xoff = (float)(mousePos.x - center.x);
            float yoff = (float)(center.y - mousePos.y);

            if (!debugCam) {
                player.ProcessMouseMovement(xoff, yoff);
            } else {
                freeCamYaw += xoff * 0.1f;
                freeCamPitch += yoff * 0.1f;
                if (freeCamPitch > 89.0f) freeCamPitch = 89.0f;
                if (freeCamPitch < -89.0f) freeCamPitch = -89.0f;
                glm::vec3 f;
                f.x = cos(glm::radians(freeCamYaw)) * cos(glm::radians(freeCamPitch));
                f.y = sin(glm::radians(freeCamPitch));
                f.z = sin(glm::radians(freeCamYaw)) * cos(glm::radians(freeCamPitch));
                freeCamFront = glm::normalize(f);
            }

            if (sf::Mouse::isButtonPressed(sf::Mouse::Left) && !debugCam) {
                weapon.TryFire(player.Position, player.Front, particles, footprints, chunkManager);
            }
        }

        footprints.Update(deltaTime);
        globalTime += deltaTime;

        // Draw Loop
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glViewport(0, 0, INTERNAL_W, INTERNAL_H);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glUniform1f(glGetUniformLocation(shaderProgram, "u_Time"), globalTime);
        glUniform2f(glGetUniformLocation(shaderProgram, "u_Resolution"), (float)INTERNAL_W, (float)INTERNAL_H);
        glUniform1i(glGetUniformLocation(shaderProgram, "u_Snap"), 1); 

        glm::mat4 view = debugCam ? glm::lookAt(freeCamPos, freeCamPos + freeCamFront, glm::vec3(0,1,0)) : player.GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(70.0f), (float)INTERNAL_ASPECT, 0.1f, 1000.0f);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "u_View"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "u_Projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "u_Model"), 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));

        // FRUSTUM CULLING UPDATE
        chunkManager.UpdateVisibility(projection * view);

        // Wind System Update
        windSystem.Update(deltaTime);
        weapon.Update(deltaTime);     // NEW
        particles.Update(deltaTime);  // NEW
        glm::vec2 windDir = windSystem.GetDirection();
        glUniform2f(glGetUniformLocation(shaderProgram, "u_WindDirection"), windDir.x, windDir.y);

        // 1. Terrain Render
        glUniform1i(glGetUniformLocation(shaderProgram, "u_IsInstanced"), 0);
        glUniform1i(glGetUniformLocation(shaderProgram, "u_ConformToTerrain"), 0);
        glUniform1f(glGetUniformLocation(shaderProgram, "u_WindStrength"), 0.0f); // STATIC TERRAIN
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID); // Noise texture
        glUniform1i(glGetUniformLocation(shaderProgram, "u_Texture"), 0);
        chunkManager.RenderTerrain(shaderProgram);

        // 2. Tree & Shadow Collection
        std::vector<glm::vec4> treePositions;
        chunkManager.CollectAllTreePositions(treePositions);
        
        if (!treePositions.empty()) {
            // Update Shared Instance Data ONCE
            glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
            glBufferData(GL_ARRAY_BUFFER, treePositions.size() * sizeof(glm::vec4), treePositions.data(), GL_STREAM_DRAW);
            
            // Common Uniforms
            glUniform1i(glGetUniformLocation(shaderProgram, "u_IsInstanced"), 1);
            glUniform1i(glGetUniformLocation(shaderProgram, "u_ConformToTerrain"), 0); 
            glBindTexture(GL_TEXTURE_2D, textureID);

            // PASS A: TRUNKS (RIGID)
            glUniform1f(glGetUniformLocation(shaderProgram, "u_WindStrength"), 0.0f); // STATIC
            glBindVertexArray(trunkVAO);
            // Safety Enables
            glEnableVertexAttribArray(0); glEnableVertexAttribArray(1); glEnableVertexAttribArray(2); glEnableVertexAttribArray(3); glEnableVertexAttribArray(4);
            glDrawArraysInstanced(GL_TRIANGLES, 0, (GLsizei)trunkMesh.size(), (GLsizei)treePositions.size());

            // PASS B: LEAVES (SWAYING)
            glUniform1f(glGetUniformLocation(shaderProgram, "u_WindStrength"), 1.0f); // WIND ON
            glBindVertexArray(leavesVAO);
            // Safety Enables
            glEnableVertexAttribArray(0); glEnableVertexAttribArray(1); glEnableVertexAttribArray(2); glEnableVertexAttribArray(3); glEnableVertexAttribArray(4);
            glDrawArraysInstanced(GL_TRIANGLES, 0, (GLsizei)leavesMesh.size(), (GLsizei)treePositions.size());
        }

        // 3. Footprints
        glUniform1i(glGetUniformLocation(shaderProgram, "u_ConformToTerrain"), 1); // Snap footprints too
        glUniform1f(glGetUniformLocation(shaderProgram, "u_WindStrength"), 0.0f); // STATIC FOOTPRINTS
        footprints.Render(shaderProgram);
        glUniform1i(glGetUniformLocation(shaderProgram, "u_ConformToTerrain"), 0);

        particles.Render(shaderProgram, player.Position);
        
        // 4. Monster Render (Normal)
        monster.Render(shaderProgram);

        // Debug Highlights (Always on top)
        if (debugCam) {
            monster.RenderDebug(shaderProgram);
            player.RenderDebug(shaderProgram);
        }

        // 5. Weapon (Overlay - Clear Depth)
        glClear(GL_DEPTH_BUFFER_BIT); 
        glm::mat4 viewIdentity = glm::mat4(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "u_View"), 1, GL_FALSE, glm::value_ptr(viewIdentity));
        weapon.Render(shaderProgram);

        // 4. UI Pass (SEPARATE SHADER PASS - IN FBO FOR RETRO LOOK)
        glDisable(GL_DEPTH_TEST);
        glDepthRange(0, 0.01); // Force Near
        glUseProgram(uiProgram); // Switch to specialized UI shader
        
        // NUCLEAR STATE RESET
        glBindBuffer(GL_ARRAY_BUFFER, 0); 
        glBindVertexArray(uiVAO); 
        glDisableVertexAttribArray(1); 
        glDisableVertexAttribArray(2); 
        glDisableVertexAttribArray(3); 
        glDisableVertexAttribArray(4); 
        
        // Force safe defaults
        glVertexAttrib2f(2, 0.0f, 0.0f); 
        
        glUniform3f(glGetUniformLocation(uiProgram, "u_Color"), 1.0f, 1.0f, 1.0f);
        glUniform1i(glGetUniformLocation(uiProgram, "u_Texture"), 0);
        glBindTexture(GL_TEXTURE_2D, whiteTexID);

        // Crosshair
        std::vector<float> chData;
        float chS = 0.03f; float chT = 0.004f;
        // Fix aspect for UI in 320x240
        PushQuad(chData, -chS, -chT, chS*2, chT*2); 
        PushQuad(chData, -chT, -chS*INTERNAL_ASPECT, chT*2, chS*INTERNAL_ASPECT*2); 
        glBindBuffer(GL_ARRAY_BUFFER, uiVBO);
        glBufferData(GL_ARRAY_BUFFER, chData.size() * sizeof(float), chData.data(), GL_DYNAMIC_DRAW);
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(chData.size()/3));

        // FPS Counter
        std::string fpsStr = std::to_string(currentFPS);
        float charX = 0.8f; 
        float charSize = 0.06f; 
        for(char c : fpsStr) {
            DrawDigitSolid(c - '0', charX, 0.85f, charSize, uiVAO, uiVBO); 
            charX += charSize * 1.3f; 
        }

        // Wind Arrow
        float windAngle = atan2(windDir.y, windDir.x);
        DrawArrow(0.9f, 0.1f, 0.05f, -windAngle, uiVAO, uiVBO); 

        glEnable(GL_DEPTH_TEST);
        glDepthRange(0, 1.0); // Restore Depth

        // =================================================================================
        // PASS 2: UPSCALE TO SCREEN
        // =================================================================================
        glBindFramebuffer(GL_FRAMEBUFFER, 0); // Back to default
        glViewport(0, 0, desktop.width, desktop.height); 
        
        // Debug: Clear to BLACK to distinguish "Quad Failed" (Black) from "FBO Empty" (Sky Blue)
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f); 
        glClear(GL_COLOR_BUFFER_BIT); 

        glDisable(GL_DEPTH_TEST); // IMPORTANT: Disable depth for 2D Quad
        glUseProgram(screenShader);
        glBindVertexArray(quadVAO);
        // Bind FBO texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texColorBuffer); 
        glUniform1i(glGetUniformLocation(screenShader, "u_ScreenTexture"), 0);
        
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Restore State for next frame's Pass 1
        glClearColor(0.4f, 0.6f, 1.0f, 1.0f); // Sky Blue for World
        glEnable(GL_DEPTH_TEST);

        window.display();
    }

    return 0;
}
