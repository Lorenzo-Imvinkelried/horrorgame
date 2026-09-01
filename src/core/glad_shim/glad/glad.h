#pragma once

#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
typedef void* (*GLADloadproc)(const char *name);
#define gladLoadGL() (1)
#define gladLoadGLES2Loader(x) (1)
#ifndef glPointSize
#define glPointSize(x) ((void)0)
#endif
#ifndef glDepthRange
#define glDepthRange(n, f) glDepthRangef((float)(n), (float)(f))
#endif
#ifndef glClearDepth
#define glClearDepth(d) glClearDepthf((float)(d))
#endif
#else
#include_next <glad/glad.h>
#endif
