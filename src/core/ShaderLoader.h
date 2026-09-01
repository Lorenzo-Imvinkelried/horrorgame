#pragma once
#include <glad/glad.h>
#include <string>

namespace ShaderLoader {
    GLuint Load(const char* vertPath, const char* fragPath);
}
