#ifndef GL_PLATFORM_H
#define GL_PLATFORM_H

#ifdef __APPLE__
#include <OpenGL/gl3.h>
#elif __EMSCRIPTEN__
#include <GLES2/gl2.h>
#else
#include <GL/glew.h>
#endif

#endif
