/*
 * FX Shader Runtime Loader
 * Handmade OpenGL 3.3 core shader loader with live reloading support
 * Inspired by RAD Game Tools, Casey Muratori, and Jonathan Blow
 *
 * Copyright (c) 2024 Giovanni Carlino
 * MIT License - see LICENSE file for details
 */

#ifndef FX_RUNTIME_H
#define FX_RUNTIME_H

#include "fx_gl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct FXUniform {
    const char* name;
    GLint location;
    GLenum type;
    struct FXUniform* next;
} FXUniform;

typedef struct FXInput {
    const char* name;
    GLint location;
    GLenum type;
    struct FXInput* next;
} FXInput;

typedef struct FXShader {
    const char* name;
    GLuint program;
    FXUniform* uniforms;
    FXInput* inputs;
    struct FXShader* next;
} FXShader;

// Core functions
FXShader* fx_load(const char* shader_name);
void fx_use(FXShader* shader);
void fx_set_uniform_float(FXShader* shader, const char* name, float value);
void fx_set_uniform_vec3(FXShader* shader, const char* name, float x, float y, float z);
void fx_set_uniform_vec4(FXShader* shader, const char* name, float x, float y, float z, float w);
void fx_set_uniform_mat4(FXShader* shader, const char* name, const float* matrix);
void fx_cleanup(FXShader* shader);

// Helper functions
static char* read_file(const char* path);
static GLuint compile_shader(const char* source, GLenum type);
static GLuint link_program(GLuint vertex, GLuint fragment);
static void parse_metadata(const char* meta_path, FXShader* shader);

#endif // FX_RUNTIME_H 