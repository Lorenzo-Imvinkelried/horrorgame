#pragma once

#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#define gladLoadGL() (1)
#else
#include_next <glad/glad.h>
#endif
