#pragma once

#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#define gladLoadGL() (1)
#ifndef glPointSize
#define glPointSize(x) ((void)0)
#endif
#else
#include_next <glad/glad.h>
#endif
