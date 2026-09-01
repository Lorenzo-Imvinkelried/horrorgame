#include "ShaderLoader.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>

namespace ShaderLoader {

GLuint Load(const char* vertPath, const char* fragPath) {
    auto loadFile = [](const char* path) -> std::string {
        std::vector<std::string> candidates = {
            std::string(path),
            "../" + std::string(path),
            "../../" + std::string(path),
            "bin/" + std::string(path)
        };
        FILE* f = nullptr;
        for (const auto& c : candidates) {
            f = fopen(c.c_str(), "rb");
            if (f) break;
        }
        if (!f) return std::string("");
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

#ifdef __EMSCRIPTEN__
    auto adaptForWebGL2 = [](std::string& src, bool isFragment) {
        size_t pos = src.find("#version 330 core");
        if (pos != std::string::npos) {
            std::string header = "#version 300 es\n";
            if (isFragment) {
                header += "precision highp float;\nprecision highp int;\nprecision mediump sampler2D;\n";
            } else {
                header += "precision highp float;\nprecision highp int;\n";
            }
            src.replace(pos, 17, header);
        }
    };
    adaptForWebGL2(vStr, false);
    adaptForWebGL2(fStr, true);
#endif

    if (vStr.empty()) std::cerr << "[Shader] Failed to read vertex shader: " << vertPath << std::endl;
    if (fStr.empty()) std::cerr << "[Shader] Failed to read fragment shader: " << fragPath << std::endl;

    const char* vSrc = vStr.c_str();
    const char* fSrc = fStr.c_str();

    GLint success;
    GLchar infoLog[512];

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vSrc, NULL);
    glCompileShader(vs);
    glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vs, 512, NULL, infoLog);
        std::cerr << "[Shader] Vertex compile error (" << vertPath << "):\n" << infoLog << std::endl;
    }

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fSrc, NULL);
    glCompileShader(fs);
    glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fs, 512, NULL, infoLog);
        std::cerr << "[Shader] Fragment compile error (" << fragPath << "):\n" << infoLog << std::endl;
    }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(prog, 512, NULL, infoLog);
        std::cerr << "[Shader] Link error (" << vertPath << " & " << fragPath << "):\n" << infoLog << std::endl;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

} // namespace ShaderLoader
