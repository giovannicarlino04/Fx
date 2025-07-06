/*
 * Handmade OpenGL Function Loader Implementation
 * Pure C implementation for Windows - no external dependencies
 * Inspired by RAD Game Tools, Casey Muratori, and Jonathan Blow
 *
 * Copyright (c) 2024 Giovanni Carlino
 * MIT License - see LICENSE file for details
 */

#include "fx_gl.h"

// Define the function pointers
PFNGLGENVERTEXARRAYS glGenVertexArrays = NULL;
PFNGLBINDVERTEXARRAY glBindVertexArray = NULL;
PFNGLGENBUFFERS glGenBuffers = NULL;
PFNGLBINDBUFFER glBindBuffer = NULL;
PFNGLBUFFERDATA glBufferData = NULL;
PFNGLVERTEXATTRIBPOINTER glVertexAttribPointer = NULL;
PFNGLENABLEVERTEXATTRIBARRAY glEnableVertexAttribArray = NULL;
PFNGLUSEPROGRAM glUseProgram = NULL;
PFNGLCREATESHADER glCreateShader = NULL;
PFNGLSHADERSOURCE glShaderSource = NULL;
PFNGLCOMPILESHADER glCompileShader = NULL;
PFNGLGETSHADERIV glGetShaderiv = NULL;
PFNGLGETSHADERINFOLOG glGetShaderInfoLog = NULL;
PFNGLDELETESHADER glDeleteShader = NULL;
PFNGLCREATEPROGRAM glCreateProgram = NULL;
PFNGLATTACHSHADER glAttachShader = NULL;
PFNGLLINKPROGRAM glLinkProgram = NULL;
PFNGLGETPROGRAMIV glGetProgramiv = NULL;
PFNGLGETPROGRAMINFOLOG glGetProgramInfoLog = NULL;
PFNGLDELETEPROGRAM glDeleteProgram = NULL;
PFNGLGETUNIFORMLOCATION glGetUniformLocation = NULL;
PFNGLUNIFORM1F glUniform1f = NULL;
PFNGLUNIFORM3F glUniform3f = NULL;
PFNGLUNIFORM4F glUniform4f = NULL;
PFNGLUNIFORMMATRIX4FV glUniformMatrix4fv = NULL;
PFNGLGETATTRIBLOCATION glGetAttribLocation = NULL;