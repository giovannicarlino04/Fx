/*
 * FX Shader Runtime Loader Implementation
 * Handmade OpenGL 3.3 core shader loader with live reloading support
 * Inspired by RAD Game Tools, Casey Muratori, and Jonathan Blow
 *
 * Copyright (c) 2024 Giovanni Carlino
 * MIT License - see LICENSE file for details
 */

#include "fx_runtime.h"

static char* read_file(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* data = (char*)malloc(size + 1);
    fread(data, 1, size, f);
    data[size] = '\0';
    fclose(f);
    return data;
}

static GLuint compile_shader(const char* source, GLenum type) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar info_log[512];
        glGetShaderInfoLog(shader, 512, NULL, info_log);
        fprintf(stderr, "Shader compilation error: %s\n", info_log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

static GLuint link_program(GLuint vertex, GLuint fragment) {
    GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar info_log[512];
        glGetProgramInfoLog(program, 512, NULL, info_log);
        fprintf(stderr, "Program linking error: %s\n", info_log);
        glDeleteProgram(program);
        return 0;
    }
    
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    return program;
}

static void parse_metadata(const char* meta_path, FXShader* shader) {
    char* meta_data = read_file(meta_path);
    if (!meta_data) return;
    
    char* line = strtok(meta_data, "\n");
    while (line) {
        if (strncmp(line, "uniform ", 8) == 0) {
            char type[32], name[64];
            if (sscanf(line + 8, "%31s %63s", type, name) == 2) {
                FXUniform* uniform = (FXUniform*)calloc(1, sizeof(FXUniform));
                uniform->name = strdup(name);
                uniform->location = glGetUniformLocation(shader->program, name);
                uniform->next = shader->uniforms;
                shader->uniforms = uniform;
            }
        } else if (strncmp(line, "input ", 6) == 0) {
            char type[32], name[64];
            if (sscanf(line + 6, "%31s %63s", type, name) == 2) {
                FXInput* input = (FXInput*)calloc(1, sizeof(FXInput));
                input->name = strdup(name);
                input->location = glGetAttribLocation(shader->program, name);
                input->next = shader->inputs;
                shader->inputs = input;
            }
        }
        line = strtok(NULL, "\n");
    }
    free(meta_data);
}

FXShader* fx_load(const char* shader_name) {
    char vert_path[256], frag_path[256], meta_path[256];
    snprintf(vert_path, sizeof(vert_path), "%s.vert.glsl", shader_name);
    snprintf(frag_path, sizeof(frag_path), "%s.frag.glsl", shader_name);
    snprintf(meta_path, sizeof(meta_path), "%s.meta", shader_name);
    
    char* vert_source = read_file(vert_path);
    char* frag_source = read_file(frag_path);
    if (!vert_source || !frag_source) {
        fprintf(stderr, "Could not load shader: %s or %s\n", vert_path, frag_path);
        if (vert_source) free(vert_source);
        if (frag_source) free(frag_source);
        return NULL;
    }
    
    GLuint vertex_shader = compile_shader(vert_source, GL_VERTEX_SHADER);
    GLuint fragment_shader = compile_shader(frag_source, GL_FRAGMENT_SHADER);
    
    free(vert_source);
    free(frag_source);
    
    if (!vertex_shader || !fragment_shader) {
        return NULL;
    }
    
    GLuint program = link_program(vertex_shader, fragment_shader);
    if (!program) {
        return NULL;
    }
    
    FXShader* shader = (FXShader*)calloc(1, sizeof(FXShader));
    shader->name = strdup(shader_name);
    shader->program = program;
    
    parse_metadata(meta_path, shader);
    
    return shader;
}

void fx_use(FXShader* shader) {
    if (shader) {
        glUseProgram(shader->program);
    }
}

void fx_set_uniform_float(FXShader* shader, const char* name, float value) {
    if (!shader) return;
    GLint location = glGetUniformLocation(shader->program, name);
    if (location != -1) {
        glUniform1f(location, value);
    }
}

void fx_set_uniform_vec3(FXShader* shader, const char* name, float x, float y, float z) {
    if (!shader) return;
    GLint location = glGetUniformLocation(shader->program, name);
    if (location != -1) {
        glUniform3f(location, x, y, z);
    }
}

void fx_set_uniform_vec4(FXShader* shader, const char* name, float x, float y, float z, float w) {
    if (!shader) return;
    GLint location = glGetUniformLocation(shader->program, name);
    if (location != -1) {
        glUniform4f(location, x, y, z, w);
    }
}

void fx_set_uniform_mat4(FXShader* shader, const char* name, const float* matrix) {
    if (!shader) return;
    GLint location = glGetUniformLocation(shader->program, name);
    if (location != -1) {
        glUniformMatrix4fv(location, 1, GL_FALSE, matrix);
    }
}

void fx_cleanup(FXShader* shader) {
    if (!shader) return;
    
    // Clean up uniforms
    FXUniform* u = shader->uniforms;
    while (u) {
        FXUniform* next = u->next;
        free((void*)u->name);
        free(u);
        u = next;
    }
    
    // Clean up inputs
    FXInput* i = shader->inputs;
    while (i) {
        FXInput* next = i->next;
        free((void*)i->name);
        free(i);
        i = next;
    }
    
    glDeleteProgram(shader->program);
    free((void*)shader->name);
    free(shader);
} 