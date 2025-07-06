/*
 * Handmade OpenGL Function Loader
 * Pure C implementation for Windows - no external dependencies
 * Inspired by RAD Game Tools, Casey Muratori, and Jonathan Blow
 *
 * Copyright (c) 2024 Giovanni Carlino
 * MIT License - see LICENSE file for details
 */

#ifndef FX_GL_H
#define FX_GL_H

#include <windows.h>
#include <GL/gl.h>
#include <stddef.h>
#include <stdio.h>

// OpenGL constants we need
#ifndef GL_VERTEX_SHADER
#define GL_VERTEX_SHADER 0x8B31
#endif
#ifndef GL_FRAGMENT_SHADER
#define GL_FRAGMENT_SHADER 0x8B30
#endif
#ifndef GL_COMPILE_STATUS
#define GL_COMPILE_STATUS 0x8B81
#endif
#ifndef GL_LINK_STATUS
#define GL_LINK_STATUS 0x8B82
#endif
#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER 0x8892
#endif
#ifndef GL_STATIC_DRAW
#define GL_STATIC_DRAW 0x88E4
#endif
#ifndef GL_TRIANGLES
#define GL_TRIANGLES 0x0004
#endif
#ifndef GL_COLOR_BUFFER_BIT
#define GL_COLOR_BUFFER_BIT 0x00004000
#endif
#ifndef GL_FALSE
#define GL_FALSE 0
#endif
#ifndef APIENTRY
#define APIENTRY __stdcall
#endif
#ifndef APIENTRYP
#define APIENTRYP APIENTRY *
#endif
// Type definitions
typedef char GLchar;

// Function pointer types
typedef void (APIENTRYP PFNGLGENVERTEXARRAYS)(GLsizei, GLuint*);
typedef void (APIENTRYP PFNGLBINDVERTEXARRAY)(GLuint);
typedef void (APIENTRYP PFNGLGENBUFFERS)(GLsizei, GLuint*);
typedef void (APIENTRYP PFNGLBINDBUFFER)(GLenum, GLuint);
typedef void (APIENTRYP PFNGLBUFFERDATA)(GLenum, ptrdiff_t, const void*, GLenum);
typedef void (APIENTRYP PFNGLVERTEXATTRIBPOINTER)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*);
typedef void (APIENTRYP PFNGLENABLEVERTEXATTRIBARRAY)(GLuint);
typedef void (APIENTRYP PFNGLUSEPROGRAM)(GLuint);
typedef GLuint (APIENTRYP PFNGLCREATESHADER)(GLenum);
typedef void (APIENTRYP PFNGLSHADERSOURCE)(GLuint, GLsizei, const char* const*, const GLint*);
typedef void (APIENTRYP PFNGLCOMPILESHADER)(GLuint);
typedef void (APIENTRYP PFNGLGETSHADERIV)(GLuint, GLenum, GLint*);
typedef void (APIENTRYP PFNGLGETSHADERINFOLOG)(GLuint, GLsizei, GLsizei*, char*);
typedef void (APIENTRYP PFNGLDELETESHADER)(GLuint);
typedef GLuint (APIENTRYP PFNGLCREATEPROGRAM)(void);
typedef void (APIENTRYP PFNGLATTACHSHADER)(GLuint, GLuint);
typedef void (APIENTRYP PFNGLLINKPROGRAM)(GLuint);
typedef void (APIENTRYP PFNGLGETPROGRAMIV)(GLuint, GLenum, GLint*);
typedef void (APIENTRYP PFNGLGETPROGRAMINFOLOG)(GLuint, GLsizei, GLsizei*, char*);
typedef void (APIENTRYP PFNGLDELETEPROGRAM)(GLuint);
typedef GLint (APIENTRYP PFNGLGETUNIFORMLOCATION)(GLuint, const char*);
typedef void (APIENTRYP PFNGLUNIFORM1F)(GLint, float);
typedef void (APIENTRYP PFNGLUNIFORM3F)(GLint, float, float, float);
typedef void (APIENTRYP PFNGLUNIFORM4F)(GLint, float, float, float, float);
typedef void (APIENTRYP PFNGLUNIFORMMATRIX4FV)(GLint, GLsizei, GLboolean, const float*);
typedef GLint (APIENTRYP PFNGLGETATTRIBLOCATION)(GLuint, const char*);

// Function pointers
extern PFNGLGENVERTEXARRAYS glGenVertexArrays;
extern PFNGLBINDVERTEXARRAY glBindVertexArray;
extern PFNGLGENBUFFERS glGenBuffers;
extern PFNGLBINDBUFFER glBindBuffer;
extern PFNGLBUFFERDATA glBufferData;
extern PFNGLVERTEXATTRIBPOINTER glVertexAttribPointer;
extern PFNGLENABLEVERTEXATTRIBARRAY glEnableVertexAttribArray;
extern PFNGLUSEPROGRAM glUseProgram;
extern PFNGLCREATESHADER glCreateShader;
extern PFNGLSHADERSOURCE glShaderSource;
extern PFNGLCOMPILESHADER glCompileShader;
extern PFNGLGETSHADERIV glGetShaderiv;
extern PFNGLGETSHADERINFOLOG glGetShaderInfoLog;
extern PFNGLDELETESHADER glDeleteShader;
extern PFNGLCREATEPROGRAM glCreateProgram;
extern PFNGLATTACHSHADER glAttachShader;
extern PFNGLLINKPROGRAM glLinkProgram;
extern PFNGLGETPROGRAMIV glGetProgramiv;
extern PFNGLGETPROGRAMINFOLOG glGetProgramInfoLog;
extern PFNGLDELETEPROGRAM glDeleteProgram;
extern PFNGLGETUNIFORMLOCATION glGetUniformLocation;
extern PFNGLUNIFORM1F glUniform1f;
extern PFNGLUNIFORM3F glUniform3f;
extern PFNGLUNIFORM4F glUniform4f;
extern PFNGLUNIFORMMATRIX4FV glUniformMatrix4fv;
extern PFNGLGETATTRIBLOCATION glGetAttribLocation;

// Loader function
static void* fxgl_get_proc(const char* name) {
    void* p = (void*)wglGetProcAddress(name);
    if (!p) {
        HMODULE module = LoadLibraryA("opengl32.dll");
        p = (void*)GetProcAddress(module, name);
    }
    return p;
}

#endif // FX_GL_H 