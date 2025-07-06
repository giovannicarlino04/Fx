/*
 * FX Shader Compiler (fxc)
 * A minimal, handmade shader compiler for .fx files
 * Inspired by RAD Game Tools, Casey Muratori, and Jonathan Blow
 *
 * Copyright (c) 2024 Giovanni Carlino
 * MIT License - see LICENSE file for details
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Debug logging
#define LOG_LEVEL 3  // 0=off, 1=errors, 2=warnings, 3=info, 4=debug
#define LOG_ERROR(fmt, ...) if (LOG_LEVEL >= 1) fprintf(stderr, "[ERROR] " fmt "\n", ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  if (LOG_LEVEL >= 2) fprintf(stderr, "[WARN]  " fmt "\n", ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)  if (LOG_LEVEL >= 3) fprintf(stderr, "[INFO]  " fmt "\n", ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) if (LOG_LEVEL >= 4) fprintf(stderr, "[DEBUG] " fmt "\n", ##__VA_ARGS__)

// Token types
typedef enum {
    TOKEN_EOF,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_LBRACE,   // {
    TOKEN_RBRACE,   // }
    TOKEN_LPAREN,   // (
    TOKEN_RPAREN,   // )
    TOKEN_SEMICOLON,// ;
    TOKEN_COMMA,    // ,
    TOKEN_EQUAL,    // =
    TOKEN_ASTERISK, // *
    TOKEN_DOT,      // .
    TOKEN_COLON,    // :
    TOKEN_MINUS,    // -
    TOKEN_PLUS,     // +
    TOKEN_SLASH,    // /
    TOKEN_LT,       // <
    TOKEN_GT,       // >
    TOKEN_AMPERSAND,// &
    TOKEN_PIPE,     // |
    TOKEN_EXCLAMATION, // !
    // Keywords
    TOKEN_SHADER,
    TOKEN_UNIFORM,
    TOKEN_INPUT,
    TOKEN_VOID,
    TOKEN_OUT,
    // New syntax keywords
    TOKEN_VERTEX_SHADER,
    TOKEN_FRAGMENT_SHADER,
    // Types
    TOKEN_FLOAT,
    TOKEN_VEC2,
    TOKEN_VEC3,
    TOKEN_VEC4,
    TOKEN_MAT4,
    TOKEN_SAMPLER2D,
    TOKEN_SAMPLERCUBE,
} TokenType;

// Token struct
typedef struct {
    TokenType type;
    const char* text;
    int length;
    int line;
    int col;
} Token;

// Lexer state
typedef struct {
    const char* src;
    size_t pos;
    int line;
    int col;
} Lexer;

void lexer_init(Lexer* lex, const char* src) {
    lex->src = src;
    lex->pos = 0;
    lex->line = 1;
    lex->col = 1;
    LOG_DEBUG("Lexer initialized with source length: %zu", strlen(src));
}

// Forward declaration
Token lexer_next(Lexer* lex);

// Helper: print token type as string
const char* token_type_str(TokenType type) {
    switch(type) {
        case TOKEN_EOF: return "EOF";
        case TOKEN_IDENTIFIER: return "IDENTIFIER";
        case TOKEN_NUMBER: return "NUMBER";
        case TOKEN_LBRACE: return "{";
        case TOKEN_RBRACE: return "}";
        case TOKEN_LPAREN: return "(";
        case TOKEN_RPAREN: return ")";
        case TOKEN_SEMICOLON: return ";";
        case TOKEN_COMMA: return ",";
        case TOKEN_EQUAL: return "=";
        case TOKEN_ASTERISK: return "*";
        case TOKEN_DOT: return ".";
        case TOKEN_COLON: return ":";
        case TOKEN_MINUS: return "-";
        case TOKEN_PLUS: return "+";
        case TOKEN_SLASH: return "/";
        case TOKEN_LT: return "<";
        case TOKEN_GT: return ">";
        case TOKEN_AMPERSAND: return "&";
        case TOKEN_PIPE: return "|";
        case TOKEN_EXCLAMATION: return "!";
        case TOKEN_SHADER: return "shader";
        case TOKEN_UNIFORM: return "uniform";
        case TOKEN_INPUT: return "input";
        case TOKEN_VOID: return "void";
        case TOKEN_OUT: return "out";
        case TOKEN_VERTEX_SHADER: return "vertex_shader";
        case TOKEN_FRAGMENT_SHADER: return "fragment_shader";
        case TOKEN_FLOAT: return "float";
        case TOKEN_VEC2: return "vec2";
        case TOKEN_VEC3: return "vec3";
        case TOKEN_VEC4: return "vec4";
        case TOKEN_MAT4: return "mat4";
        case TOKEN_SAMPLER2D: return "sampler2D";
        case TOKEN_SAMPLERCUBE: return "samplerCube";
        default: return "?";
    }
}

// --- AST Structures ---

typedef struct FXUniform {
    const char* type;
    const char* name;
    struct FXUniform* next;
} FXUniform;

typedef struct FXInput {
    const char* type;
    const char* name;
    struct FXInput* next;
} FXInput;

typedef struct FXExpr FXExpr;

typedef struct FXStatement {
    const char* text;
    int length;
    struct FXStatement* next;
} FXStatement;

typedef struct FXFunction {
    const char* name;
    int is_vertex;
    int is_fragment;
    const char* out_type;
    const char* out_name;
    FXStatement* statements;
    struct FXFunction* next;
} FXFunction;

typedef struct FXShader {
    const char* name;
    FXUniform* uniforms;
    FXInput* inputs;
    FXFunction* functions;
    struct FXShader* next;
} FXShader;

typedef struct {
    Lexer* lex;
    Token current;
} Parser;

// Function prototypes
FXShader* parse_shader_file(Parser* p);
void generate_glsl(FXShader* shader, const char* output_path);
void generate_metadata(FXShader* shader, const char* output_path);
static char* parse_function_body_to_glsl(Parser* p, int is_vertex);
static FXUniform* copy_uniform_list(FXUniform* src);
static FXInput* copy_input_list(FXInput* src);
static void cleanup_shader(FXShader* shader);
static void cleanup_uniform_list(FXUniform* uniforms);
static void cleanup_input_list(FXInput* inputs);
static void cleanup_function_list(FXFunction* functions);

// Main function: read file, print tokens
int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <file.fx>\n", argv[0]);
        return 1;
    }
    
    LOG_INFO("Compiling shader: %s", argv[1]);
    
    // Read input file
    FILE* f = fopen(argv[1], "rb");
    if (!f) {
        LOG_ERROR("Could not open file: %s", argv[1]);
        return 1;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* src = (char*)malloc(size + 1);
    fread(src, 1, size, f);
    src[size] = '\0';
    fclose(f);
    
    LOG_DEBUG("Read %ld bytes from file", size);
    
    // Parse
    Lexer lex;
    lexer_init(&lex, src);
    Parser parser;
    parser.lex = &lex;
    FXShader* shaders = parse_shader_file(&parser);
    
    if (!shaders) {
        LOG_ERROR("Failed to parse shader file or no shaders found");
        free(src);
        return 1;
    }
    
    // Generate output for each shader
    for (FXShader* s = shaders; s; s = s->next) {
        char output_path[256];
        snprintf(output_path, sizeof(output_path), "%s_%s", argv[1], s->name);
        LOG_INFO("Generating shader: %s", s->name);
        generate_glsl(s, output_path);
        generate_metadata(s, output_path);
    }
    
    // Clean up memory
    FXShader* s = shaders;
    while (s) {
        FXShader* next = s->next;
        cleanup_shader(s);
        s = next;
    }
    
    free(src);
    LOG_INFO("Compilation completed successfully");
    return 0;
}

static int is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static int is_digit(char c) {
    return c >= '0' && c <= '9';
}

static int is_alnum(char c) {
    return is_alpha(c) || is_digit(c);
}

static void skip_whitespace(Lexer* lex) {
    for (;;) {
        char c = lex->src[lex->pos];
        if (c == ' ' || c == '\t' || c == '\r') {
            lex->pos++;
            lex->col++;
        } else if (c == '\n') {
            lex->pos++;
            lex->line++;
            lex->col = 1;
        } else if (c == '/' && lex->src[lex->pos+1] == '/') {
            // Single-line comment
            lex->pos += 2;
            lex->col += 2;
            while (lex->src[lex->pos] && lex->src[lex->pos] != '\n') {
                lex->pos++;
                lex->col++;
            }
        } else if (c == '/' && lex->src[lex->pos+1] == '*') {
            // Multi-line comment
            lex->pos += 2;
            lex->col += 2;
            while (lex->src[lex->pos] && !(lex->src[lex->pos] == '*' && lex->src[lex->pos+1] == '/')) {
                if (lex->src[lex->pos] == '\n') {
                    lex->line++;
                    lex->col = 1;
                } else {
                    lex->col++;
                }
                lex->pos++;
            }
            if (lex->src[lex->pos]) {
                lex->pos += 2;
                lex->col += 2;
            }
        } else {
            break;
        }
    }
}

static int match_keyword(const char* text, int len, const char* kw) {
    int kwlen = (int)strlen(kw);
    return len == kwlen && strncmp(text, kw, kwlen) == 0;
}

static TokenType check_keyword(const char* text, int len) {
    // Keywords
    if (match_keyword(text, len, "shader")) return TOKEN_SHADER;
    if (match_keyword(text, len, "uniform")) return TOKEN_UNIFORM;
    if (match_keyword(text, len, "input")) return TOKEN_INPUT;
    if (match_keyword(text, len, "void")) return TOKEN_VOID;
    if (match_keyword(text, len, "out")) return TOKEN_OUT;
    // New syntax keywords
    if (match_keyword(text, len, "vertex_shader")) return TOKEN_VERTEX_SHADER;
    if (match_keyword(text, len, "fragment_shader")) return TOKEN_FRAGMENT_SHADER;
    // Types
    if (match_keyword(text, len, "float")) return TOKEN_FLOAT;
    if (match_keyword(text, len, "vec2")) return TOKEN_VEC2;
    if (match_keyword(text, len, "vec3")) return TOKEN_VEC3;
    if (match_keyword(text, len, "vec4")) return TOKEN_VEC4;
    if (match_keyword(text, len, "mat4")) return TOKEN_MAT4;
    if (match_keyword(text, len, "sampler2D")) return TOKEN_SAMPLER2D;
    if (match_keyword(text, len, "samplerCube")) return TOKEN_SAMPLERCUBE;
    return TOKEN_IDENTIFIER;
}

Token lexer_next(Lexer* lex) {
    skip_whitespace(lex);
    
    if (lex->pos >= strlen(lex->src)) {
        return (Token){TOKEN_EOF, lex->src + lex->pos, 0, lex->line, lex->col};
    }
    
    char c = lex->src[lex->pos];
    int start_pos = lex->pos;
    int start_line = lex->line;
    int start_col = lex->col;
    
    // Identifiers and keywords
    if (is_alpha(c) || c == '_') {
        while (is_alnum(lex->src[lex->pos]) || lex->src[lex->pos] == '_') {
            lex->pos++;
            lex->col++;
        }
        int len = lex->pos - start_pos;
        TokenType type = check_keyword(lex->src + start_pos, len);
        Token t = {type, lex->src + start_pos, len, start_line, start_col};
        return t;
    }
    
    // Numbers
    if (is_digit(c)) {
        int num_start = lex->pos;
        int num_col = lex->col;
        while (is_digit(lex->src[lex->pos])) {
            lex->pos++;
            lex->col++;
        }
        if (lex->src[lex->pos] == '.') {
            lex->pos++;
            lex->col++;
            while (is_digit(lex->src[lex->pos])) {
                lex->pos++;
                lex->col++;
            }
        }
        int len = lex->pos - num_start;
        Token t = {TOKEN_NUMBER, lex->src + num_start, len, start_line, num_col};
        return t;
    }
    // Symbols
    switch (c) {
        case '{': lex->pos++; lex->col++; return (Token){TOKEN_LBRACE, lex->src + start_pos, 1, start_line, start_col};
        case '}': lex->pos++; lex->col++; return (Token){TOKEN_RBRACE, lex->src + start_pos, 1, start_line, start_col};
        case '(': lex->pos++; lex->col++; return (Token){TOKEN_LPAREN, lex->src + start_pos, 1, start_line, start_col};
        case ')': lex->pos++; lex->col++; return (Token){TOKEN_RPAREN, lex->src + start_pos, 1, start_line, start_col};
        case ';': lex->pos++; lex->col++; return (Token){TOKEN_SEMICOLON, lex->src + start_pos, 1, start_line, start_col};
        case ',': lex->pos++; lex->col++; return (Token){TOKEN_COMMA, lex->src + start_pos, 1, start_line, start_col};
        case '=': lex->pos++; lex->col++; return (Token){TOKEN_EQUAL, lex->src + start_pos, 1, start_line, start_col};
        case '*': lex->pos++; lex->col++; return (Token){TOKEN_ASTERISK, lex->src + start_pos, 1, start_line, start_col};
        case '.': lex->pos++; lex->col++; return (Token){TOKEN_DOT, lex->src + start_pos, 1, start_line, start_col};
        case ':': lex->pos++; lex->col++; return (Token){TOKEN_COLON, lex->src + start_pos, 1, start_line, start_col};
        case '-': lex->pos++; lex->col++; return (Token){TOKEN_MINUS, lex->src + start_pos, 1, start_line, start_col};
        case '+': lex->pos++; lex->col++; return (Token){TOKEN_PLUS, lex->src + start_pos, 1, start_line, start_col};
        case '/': lex->pos++; lex->col++; return (Token){TOKEN_SLASH, lex->src + start_pos, 1, start_line, start_col};
        case '<': lex->pos++; lex->col++; return (Token){TOKEN_LT, lex->src + start_pos, 1, start_line, start_col};
        case '>': lex->pos++; lex->col++; return (Token){TOKEN_GT, lex->src + start_pos, 1, start_line, start_col};
        case '&': lex->pos++; lex->col++; return (Token){TOKEN_AMPERSAND, lex->src + start_pos, 1, start_line, start_col};
        case '|': lex->pos++; lex->col++; return (Token){TOKEN_PIPE, lex->src + start_pos, 1, start_line, start_col};
        case '!': lex->pos++; lex->col++; return (Token){TOKEN_EXCLAMATION, lex->src + start_pos, 1, start_line, start_col};
        default:
            // Unknown character, skip it
            lex->pos++;
            lex->col++;
            return (Token){TOKEN_EOF, lex->src + start_pos, 1, start_line, start_col};
    }
}

// --- Parser Implementation ---

static void parser_advance(Parser* p) {
    p->current = lexer_next(p->lex);
}

static int parser_match(Parser* p, TokenType type) {
    if (p->current.type == type) {
        parser_advance(p);
        return 1;
    }
    return 0;
}

static int parser_expect(Parser* p, TokenType type, const char* msg) {
    if (!parser_match(p, type)) {
        LOG_ERROR("Parse error: expected %s at line %d, col %d (got %s)", 
                  msg, p->current.line, p->current.col, token_type_str(p->current.type));
        return 0; // Return error instead of exit
    }
    return 1; // Success
}

static char* token_to_str(const Token* t) {
    char* s = (char*)malloc(t->length + 1);
    memcpy(s, t->text, t->length);
    s[t->length] = 0;
    return s;
}

static FXUniform* parse_uniform(Parser* p) {
    if (!parser_expect(p, TOKEN_UNIFORM, "'uniform'")) return NULL;
    if (p->current.type < TOKEN_FLOAT || p->current.type > TOKEN_SAMPLERCUBE) {
        LOG_ERROR("Parse error: expected type after 'uniform' at line %d", p->current.line);
        return NULL;
    }
    char* type = token_to_str(&p->current);
    parser_advance(p);
    if (p->current.type != TOKEN_IDENTIFIER) {
        LOG_ERROR("Parse error: expected identifier after type in uniform declaration at line %d", p->current.line);
        free(type);
        return NULL;
    }
    char* name = token_to_str(&p->current);
    parser_advance(p);
    if (!parser_expect(p, TOKEN_SEMICOLON, ";")) {
        free(type);
        free(name);
        return NULL;
    }
    FXUniform* u = (FXUniform*)calloc(1, sizeof(FXUniform));
    u->type = type;
    u->name = name;
    return u;
}

static FXInput* parse_input(Parser* p) {
    if (!parser_expect(p, TOKEN_INPUT, "'input'")) return NULL;
    if (p->current.type < TOKEN_FLOAT || p->current.type > TOKEN_SAMPLERCUBE) {
        LOG_ERROR("Parse error: expected type after 'input' at line %d", p->current.line);
        return NULL;
    }
    char* type = token_to_str(&p->current);
    parser_advance(p);
    if (p->current.type != TOKEN_IDENTIFIER) {
        LOG_ERROR("Parse error: expected identifier after type in input declaration at line %d", p->current.line);
        free(type);
        return NULL;
    }
    char* name = token_to_str(&p->current);
    parser_advance(p);
    if (!parser_expect(p, TOKEN_SEMICOLON, ";")) {
        free(type);
        free(name);
        return NULL;
    }
    FXInput* in = (FXInput*)calloc(1, sizeof(FXInput));
    in->type = type;
    in->name = name;
    return in;
}

static FXStatement* parse_statement(Parser* p) {
    // For now, just grab everything up to the next '}' or end of function
    int start = p->current.text - p->lex->src;
    int depth = 0;
    while (p->current.type != TOKEN_RBRACE && p->current.type != TOKEN_EOF) {
        if (p->current.type == TOKEN_LBRACE) depth++;
        if (p->current.type == TOKEN_RBRACE && depth > 0) depth--;
        parser_advance(p);
        if (depth == 0 && (p->current.type == TOKEN_RBRACE || p->current.type == TOKEN_EOF)) break;
    }
    int end = p->current.text - p->lex->src;
    FXStatement* stmt = (FXStatement*)calloc(1, sizeof(FXStatement));
    stmt->text = p->lex->src + start;
    stmt->length = end - start;
    return stmt;
}

static FXFunction* parse_function(Parser* p) {
    LOG_DEBUG("Parsing function at line %d", p->current.line);
    
    parser_expect(p, TOKEN_VOID, "'void'");
    parser_expect(p, TOKEN_IDENTIFIER, "function name");
    char* name = token_to_str(&p->current);
    int is_vertex = strcmp(name, "vertex") == 0;
    int is_fragment = strcmp(name, "fragment") == 0;
    
    LOG_DEBUG("Function name: %s (vertex=%d, fragment=%d)", name, is_vertex, is_fragment);
    
    parser_expect(p, TOKEN_LPAREN, "(");
    char* out_type = NULL;
    char* out_name = NULL;
    // Handle fragment function output parameter
    if (is_fragment && parser_match(p, TOKEN_OUT)) {
        if (p->current.type < TOKEN_FLOAT || p->current.type > TOKEN_MAT4) {
            LOG_ERROR("Parse error: expected type after 'out' in fragment()");
            exit(1);
        }
        out_type = token_to_str(&p->current);
        parser_advance(p);
        parser_expect(p, TOKEN_IDENTIFIER, "output param name");
        out_name = token_to_str(&p->current);
        LOG_DEBUG("Fragment output: %s %s", out_type, out_name);
    }
    // For now, we don't handle input parameters to functions
    // They are declared as shader inputs instead
    parser_expect(p, TOKEN_RPAREN, ")");
    parser_expect(p, TOKEN_LBRACE, "{");
    FXStatement* stmts = parse_statement(p);
    parser_expect(p, TOKEN_RBRACE, "}");
    
    FXFunction* fn = (FXFunction*)calloc(1, sizeof(FXFunction));
    fn->name = name;
    fn->is_vertex = is_vertex;
    fn->is_fragment = is_fragment;
    fn->out_type = out_type;
    fn->out_name = out_name;
    fn->statements = stmts;
    
    LOG_DEBUG("Parsed function: %s", name);
    return fn;
}

// New parser for vertex_shader and fragment_shader syntax
static FXFunction* parse_new_function(Parser* p) {
    LOG_DEBUG("Parsing new-style function at line %d", p->current.line);
    
    TokenType function_type = p->current.type;
    int is_vertex = (function_type == TOKEN_VERTEX_SHADER);
    int is_fragment = (function_type == TOKEN_FRAGMENT_SHADER);
    
    parser_advance(p); // Consume vertex_shader or fragment_shader
    
    char* name = NULL;
    if (p->current.type == TOKEN_IDENTIFIER) {
        name = token_to_str(&p->current);
        parser_advance(p);
    } else {
        // Anonymous function: use default name
        name = strdup(is_vertex ? "vertex" : "fragment");
    }
    
    LOG_DEBUG("New function: %s (vertex=%d, fragment=%d)", name, is_vertex, is_fragment);
    
    parser_expect(p, TOKEN_LPAREN, "(");
    while (p->current.type != TOKEN_RPAREN && p->current.type != TOKEN_EOF) {
        // Type
        if (p->current.type == TOKEN_FLOAT || p->current.type == TOKEN_VEC2 || p->current.type == TOKEN_VEC3 ||
            p->current.type == TOKEN_VEC4 || p->current.type == TOKEN_MAT4 || p->current.type == TOKEN_SAMPLER2D ||
            p->current.type == TOKEN_SAMPLERCUBE || p->current.type == TOKEN_IDENTIFIER) {
            parser_advance(p);
        } else {
            LOG_ERROR("Parse error: expected parameter type at line %d, col %d (got %s)", p->current.line, p->current.col, token_type_str(p->current.type));
            exit(1);
        }
        // Name
        parser_expect(p, TOKEN_IDENTIFIER, "parameter name");
        
        // Check for colon (semantic)
        if (p->current.type == TOKEN_COLON) {
            parser_advance(p);
            parser_expect(p, TOKEN_IDENTIFIER, "semantic");
        }
        
        // Check for comma or end
        if (p->current.type == TOKEN_COMMA) {
            parser_advance(p);
        } else if (p->current.type == TOKEN_RPAREN) {
            break;
        } else {
            LOG_ERROR("Parse error: expected ',' or ')' in parameter list at line %d (got %s)", p->current.line, token_type_str(p->current.type));
            exit(1);
        }
    }
    parser_expect(p, TOKEN_RPAREN, ")");
    parser_expect(p, TOKEN_LBRACE, "{");
    char* body_glsl = parse_function_body_to_glsl(p, is_vertex);
    parser_expect(p, TOKEN_RBRACE, "}");
    
    // Create a statement from the processed body
    FXStatement* stmts = (FXStatement*)calloc(1, sizeof(FXStatement));
    stmts->text = body_glsl;
    stmts->length = strlen(body_glsl);
    
    FXFunction* fn = (FXFunction*)calloc(1, sizeof(FXFunction));
    fn->name = name;
    fn->is_vertex = is_vertex;
    fn->is_fragment = is_fragment;
    fn->out_type = NULL;
    fn->out_name = NULL;
    fn->statements = stmts;
    
    LOG_DEBUG("Parsed new function: %s", name);
    return fn;
}

static FXShader* parse_shader(Parser* p) {
    LOG_DEBUG("Parsing shader at line %d", p->current.line);
    
    parser_expect(p, TOKEN_SHADER, "'shader'");
    parser_expect(p, TOKEN_IDENTIFIER, "shader name");
    char* name = token_to_str(&p->current);
    
    LOG_DEBUG("Shader name: %s", name);
    
    parser_expect(p, TOKEN_LBRACE, "{");
    FXUniform* uniforms = NULL;
    FXInput* inputs = NULL;
    FXFunction* functions = NULL;
    FXUniform** uptr = &uniforms;
    FXInput** iptr = &inputs;
    FXFunction** fptr = &functions;
    
    while (p->current.type != TOKEN_RBRACE && p->current.type != TOKEN_EOF) {
        if (p->current.type == TOKEN_UNIFORM) {
            LOG_DEBUG("Parsing uniform at line %d", p->current.line);
            FXUniform* u = parse_uniform(p);
            *uptr = u;
            uptr = &u->next;
        } else if (p->current.type == TOKEN_INPUT) {
            LOG_DEBUG("Parsing input at line %d", p->current.line);
            FXInput* in = parse_input(p);
            *iptr = in;
            iptr = &in->next;
        } else if (p->current.type == TOKEN_VOID) {
            LOG_DEBUG("Parsing void function at line %d", p->current.line);
            FXFunction* fn = parse_function(p);
            *fptr = fn;
            fptr = &fn->next;
        } else {
            LOG_ERROR("Parse error: unexpected token '%.*s' in shader block at line %d", 
                      p->current.length, p->current.text, p->current.line);
            exit(1);
        }
    }
    
    parser_expect(p, TOKEN_RBRACE, "}");
    
    FXShader* shader = (FXShader*)calloc(1, sizeof(FXShader));
    shader->name = name;
    shader->uniforms = uniforms;
    shader->inputs = inputs;
    shader->functions = functions;
    
    LOG_DEBUG("Parsed shader: %s", name);
    return shader;
}

// New parser for standalone vertex/fragment shaders with uniforms/inputs
static FXShader* parse_standalone_shader(Parser* p) {
    LOG_DEBUG("Parsing standalone shader at line %d", p->current.line);
    
    TokenType shader_type = p->current.type;
    int is_vertex = (shader_type == TOKEN_VERTEX_SHADER);
    (void)is_vertex; // Suppress unused variable warning
    
    // Create a shader with a default name based on type
    char* name = (shader_type == TOKEN_VERTEX_SHADER) ? strdup("vertex") : strdup("fragment");
    
    LOG_DEBUG("Standalone shader type: %s", name);
    
    // Parse uniforms and inputs before the function
    FXUniform* uniforms = NULL;
    FXInput* inputs = NULL;
    FXUniform** uptr = &uniforms;
    FXInput** iptr = &inputs;
    
    // Parse uniforms and inputs before the function
    while (p->current.type == TOKEN_UNIFORM || p->current.type == TOKEN_INPUT) {
        if (p->current.type == TOKEN_UNIFORM) {
            LOG_DEBUG("Parsing uniform at line %d", p->current.line);
            FXUniform* u = parse_uniform(p);
            *uptr = u;
            uptr = &u->next;
        } else if (p->current.type == TOKEN_INPUT) {
            LOG_DEBUG("Parsing input at line %d", p->current.line);
            FXInput* in = parse_input(p);
            *iptr = in;
            iptr = &in->next;
        }
    }
    
    // Now parse the function
    FXFunction* fn = parse_new_function(p);
    
    FXShader* shader = (FXShader*)calloc(1, sizeof(FXShader));
    shader->name = name;
    shader->uniforms = uniforms;
    shader->inputs = inputs;
    shader->functions = fn;
    
    LOG_DEBUG("Parsed standalone shader: %s", name);
    return shader;
}

FXShader* parse_shader_file(Parser* p) {
    LOG_DEBUG("Starting to parse shader file");
    
    FXShader* shaders = NULL;
    FXShader** sptr = &shaders;
    parser_advance(p); // Load first token

    // Collect top-level uniforms/inputs for new syntax
    FXUniform* pending_uniforms = NULL;
    FXInput* pending_inputs = NULL;
    FXUniform** uptr = &pending_uniforms;
    FXInput** iptr = &pending_inputs;
    
    while (p->current.type != TOKEN_EOF) {
        if (p->current.type == TOKEN_SHADER) {
            LOG_DEBUG("Found shader block at line %d", p->current.line);
            FXShader* s = parse_shader(p);
            *sptr = s;
            sptr = &s->next;
        } else if (p->current.type == TOKEN_UNIFORM) {
            LOG_DEBUG("Found top-level uniform at line %d", p->current.line);
            FXUniform* u = parse_uniform(p);
            *uptr = u;
            uptr = &u->next;
        } else if (p->current.type == TOKEN_INPUT) {
            LOG_DEBUG("Found top-level input at line %d", p->current.line);
            FXInput* in = parse_input(p);
            *iptr = in;
            iptr = &in->next;
        } else if (p->current.type == TOKEN_VERTEX_SHADER || p->current.type == TOKEN_FRAGMENT_SHADER) {
            LOG_DEBUG("Found standalone shader at line %d", p->current.line);
            FXShader* s = parse_standalone_shader(p);
            // Copy pending uniforms/inputs to each shader
            if (pending_uniforms) {
                s->uniforms = copy_uniform_list(pending_uniforms);
            }
            if (pending_inputs) {
                s->inputs = copy_input_list(pending_inputs);
            }
            *sptr = s;
            sptr = &s->next;
        } else {
            LOG_ERROR("Parse error: unexpected token at line %d: '%.*s'", 
                      p->current.line, p->current.length, p->current.text);
            exit(1);
        }
    }
    
    LOG_DEBUG("Finished parsing shader file");
    return shaders;
}

// --- Code Generation ---

static void write_glsl_header(FILE* f) {
    fprintf(f, "#version 330 core\n");
    fprintf(f, "precision highp float;\n\n");
}

static void write_uniforms(FILE* f, FXUniform* uniforms) {
    for (FXUniform* u = uniforms; u; u = u->next) {
        fprintf(f, "uniform %s %s;\n", u->type, u->name);
    }
    if (uniforms) fprintf(f, "\n");
}

static void write_inputs(FILE* f, FXInput* inputs, int is_vertex) {
    int location = 0;
    for (FXInput* in = inputs; in; in = in->next) {
        if (is_vertex) {
            fprintf(f, "layout(location = %d) in %s %s;\n", location++, in->type, in->name);
        } else {
            fprintf(f, "in %s %s;\n", in->type, in->name);
        }
    }
    if (inputs) fprintf(f, "\n");
}

static void write_vertex_outputs_as_fragment_inputs(FILE* f) {
    // These are the outputs from vertex shader that become inputs to fragment shader
    fprintf(f, "in vec3 v_normal;\n");
    fprintf(f, "in vec3 v_position;\n");
    fprintf(f, "in vec2 v_texCoord;\n");
    fprintf(f, "\n");
}

static void write_function(FILE* f, FXFunction* fn, FXInput* inputs) {
    (void)inputs; // Suppress unused parameter warning
    
    if (fn->is_vertex) {
        fprintf(f, "void main() {\n");
        // Write vertex shader body (already processed)
        if (fn->statements) {
            fprintf(f, "%s", fn->statements->text);
        }
        fprintf(f, "}\n");
    } else if (fn->is_fragment) {
        // Fragment shader needs output declaration
        fprintf(f, "out vec4 fragColor;\n\n");
        fprintf(f, "void main() {\n");
        // Write fragment shader body (already processed)
        if (fn->statements) {
            fprintf(f, "%s", fn->statements->text);
        }
        fprintf(f, "}\n");
    }
}

void generate_glsl(FXShader* shader, const char* output_path) {
    LOG_DEBUG("Generating GLSL for shader: %s", shader->name);
    
    char vert_path[256], frag_path[256];
    snprintf(vert_path, sizeof(vert_path), "%s.vert.glsl", output_path);
    snprintf(frag_path, sizeof(frag_path), "%s.frag.glsl", output_path);

    // Find vertex and fragment functions
    FXFunction* vertex_fn = NULL;
    FXFunction* fragment_fn = NULL;
    for (FXFunction* fn = shader->functions; fn; fn = fn->next) {
        if (fn->is_vertex) vertex_fn = fn;
        if (fn->is_fragment) fragment_fn = fn;
    }

    LOG_DEBUG("Found vertex function: %s", vertex_fn ? vertex_fn->name : "none");
    LOG_DEBUG("Found fragment function: %s", fragment_fn ? fragment_fn->name : "none");

    // Vertex shader
    if (vertex_fn) {
        FILE* f = fopen(vert_path, "w");
        if (!f) { 
            LOG_ERROR("Could not open output file: %s", vert_path); 
            exit(1); 
        }
        write_glsl_header(f);
        write_uniforms(f, shader->uniforms);
        write_inputs(f, shader->inputs, 1);
        write_function(f, vertex_fn, shader->inputs);
        fclose(f);
        LOG_INFO("Generated: %s", vert_path);
    }
    // Fragment shader
    if (fragment_fn) {
        FILE* f = fopen(frag_path, "w");
        if (!f) { 
            LOG_ERROR("Could not open output file: %s", frag_path); 
            exit(1); 
        }
        write_glsl_header(f);
        write_uniforms(f, shader->uniforms);
        write_vertex_outputs_as_fragment_inputs(f);
        write_function(f, fragment_fn, shader->inputs);
        fclose(f);
        LOG_INFO("Generated: %s", frag_path);
    }
}

void generate_metadata(FXShader* shader, const char* output_path) {
    char meta_path[256];
    snprintf(meta_path, sizeof(meta_path), "%s.meta", output_path);
    FILE* f = fopen(meta_path, "w");
    if (!f) {
        fprintf(stderr, "Could not open metadata file: %s\n", meta_path);
        exit(1);
    }
    
    fprintf(f, "shader %s\n", shader->name);
    fprintf(f, "uniforms %d\n", 0); // Count uniforms
    for (FXUniform* u = shader->uniforms; u; u = u->next) {
        fprintf(f, "uniform %s %s\n", u->type, u->name);
    }
    fprintf(f, "inputs %d\n", 0); // Count inputs
    for (FXInput* in = shader->inputs; in; in = in->next) {
        fprintf(f, "input %s %s\n", in->type, in->name);
    }
    
    fclose(f);
    printf("Generated: %s\n", meta_path);
}

// Parse function body and convert to GLSL
static char* parse_function_body_to_glsl(Parser* p, int is_vertex) {
    // Start collecting the function body
    char* body = (char*)malloc(4096);
    int body_pos = 0;
    int depth = 0;
    TokenType prev_token = TOKEN_EOF;
    
    while (p->current.type != TOKEN_RBRACE && p->current.type != TOKEN_EOF) {
        if (p->current.type == TOKEN_LBRACE) {
            depth++;
            body[body_pos++] = '{';
            parser_advance(p);
        } else if (p->current.type == TOKEN_RBRACE) {
            if (depth > 0) {
                depth--;
                body[body_pos++] = '}';
                parser_advance(p);
            } else {
                break; // End of function
            }
        } else if (p->current.type == TOKEN_OUT) {
            // Handle out declarations - convert to proper GLSL
            parser_advance(p); // Skip 'out'
            
            // Get type
            if (p->current.type < TOKEN_FLOAT || p->current.type > TOKEN_SAMPLERCUBE) {
                LOG_ERROR("Parse error: expected type after 'out' at line %d", p->current.line);
                exit(1);
            }
            
            // For vertex shader, convert to varying out
            // For fragment shader, it's the output color
            if (is_vertex) {
                body_pos += sprintf(body + body_pos, "out ");
                // Copy type
                memcpy(body + body_pos, p->current.text, p->current.length);
                body_pos += p->current.length;
                body[body_pos++] = ' ';
                parser_advance(p);
                
                // Get variable name
                if (p->current.type != TOKEN_IDENTIFIER) {
                    LOG_ERROR("Parse error: expected identifier after type in out declaration at line %d", p->current.line);
                    exit(1);
                }
                memcpy(body + body_pos, p->current.text, p->current.length);
                body_pos += p->current.length;
                parser_advance(p);
                
                // Skip semantic if present
                if (p->current.type == TOKEN_COLON) {
                    parser_advance(p); // Skip colon
                    if (p->current.type == TOKEN_IDENTIFIER) {
                        parser_advance(p); // Skip semantic
                    }
                }
                
                body[body_pos++] = ';';
                body[body_pos++] = '\n';
            } else {
                // Fragment shader - skip the out declaration since we handle it in main
                parser_advance(p); // Skip type
                if (p->current.type == TOKEN_IDENTIFIER) {
                    parser_advance(p); // Skip name
                }
                if (p->current.type == TOKEN_COLON) {
                    parser_advance(p); // Skip colon
                    if (p->current.type == TOKEN_IDENTIFIER) {
                        parser_advance(p); // Skip semantic
                    }
                }
            }
            
            // Skip semicolon
            if (p->current.type == TOKEN_SEMICOLON) {
                parser_advance(p);
            }
        } else {
            // Add space before current token if needed
            int add_space_before = 0;
            if (prev_token != TOKEN_EOF) {
                // Handle compound assignment operators (+=, -=, etc.)
                if (p->current.type == TOKEN_EQUAL && 
                    (prev_token == TOKEN_PLUS || prev_token == TOKEN_MINUS || 
                     prev_token == TOKEN_ASTERISK || prev_token == TOKEN_SLASH)) {
                    // No space before = in compound operators
                    add_space_before = 0;
                }
                // Space before operators
                else if (p->current.type == TOKEN_EQUAL || p->current.type == TOKEN_PLUS || 
                    p->current.type == TOKEN_MINUS || p->current.type == TOKEN_ASTERISK ||
                    p->current.type == TOKEN_SLASH || p->current.type == TOKEN_LT || 
                    p->current.type == TOKEN_GT) {
                    add_space_before = 1;
                }
                // Space after operators  
                else if (prev_token == TOKEN_EQUAL || prev_token == TOKEN_PLUS || 
                        prev_token == TOKEN_MINUS || prev_token == TOKEN_ASTERISK ||
                        prev_token == TOKEN_SLASH || prev_token == TOKEN_LT || 
                        prev_token == TOKEN_GT) {
                    add_space_before = 1;
                }
                // Space between identifiers
                else if (prev_token == TOKEN_IDENTIFIER && p->current.type == TOKEN_IDENTIFIER) {
                    add_space_before = 1;
                }
                // Space after type keywords
                else if ((prev_token >= TOKEN_FLOAT && prev_token <= TOKEN_SAMPLERCUBE) && 
                         p->current.type == TOKEN_IDENTIFIER) {
                    add_space_before = 1;
                }
                // Space after commas
                else if (prev_token == TOKEN_COMMA) {
                    add_space_before = 1;
                }
                // Space before commas (but not always needed)
                else if (p->current.type == TOKEN_COMMA && prev_token != TOKEN_RPAREN) {
                    // Don't add space before comma after closing paren
                }
            }
            
            if (add_space_before) {
                body[body_pos++] = ' ';
            }
            
            // Copy current token
            if (p->current.type == TOKEN_IDENTIFIER || p->current.type == TOKEN_NUMBER) {
                memcpy(body + body_pos, p->current.text, p->current.length);
                body_pos += p->current.length;
            } else {
                // Handle operators and symbols
                const char* token_str = token_type_str(p->current.type);
                if (token_str && strlen(token_str) == 1) {
                    body[body_pos++] = token_str[0];
                } else {
                    // Copy the actual text for multi-character tokens
                    memcpy(body + body_pos, p->current.text, p->current.length);
                    body_pos += p->current.length;
                }
            }
            
            // Add newline after semicolons for better formatting
            if (p->current.type == TOKEN_SEMICOLON) {
                body[body_pos++] = '\n';
                body[body_pos++] = ' ';
                body[body_pos++] = ' ';
                body[body_pos++] = ' ';
                body[body_pos++] = ' ';
            }
            
            prev_token = p->current.type;
            parser_advance(p);
        }
        
        // Prevent buffer overflow
        if (body_pos >= 4090) {
            LOG_ERROR("Function body too large");
            exit(1);
        }
    }
    
    body[body_pos] = '\0';
    return body;
}

// Helper function to copy uniform list
static FXUniform* copy_uniform_list(FXUniform* src) {
    if (!src) return NULL;
    
    FXUniform* dst_head = NULL;
    FXUniform** dst_ptr = &dst_head;
    
    for (FXUniform* u = src; u; u = u->next) {
        FXUniform* copy = (FXUniform*)calloc(1, sizeof(FXUniform));
        copy->type = u->type;
        copy->name = u->name;
        *dst_ptr = copy;
        dst_ptr = &copy->next;
    }
    
    return dst_head;
}

// Helper function to copy input list
static FXInput* copy_input_list(FXInput* src) {
    if (!src) return NULL;
    
    FXInput* dst_head = NULL;
    FXInput** dst_ptr = &dst_head;
    
    for (FXInput* in = src; in; in = in->next) {
        FXInput* copy = (FXInput*)calloc(1, sizeof(FXInput));
        copy->type = in->type;
        copy->name = in->name;
        *dst_ptr = copy;
        dst_ptr = &copy->next;
    }
    
    return dst_head;
}

static void cleanup_shader(FXShader* shader) {
    // Free shader name
    free((void*)shader->name);

    // Free uniforms
    FXUniform* u = shader->uniforms;
    while (u) {
        FXUniform* next = u->next;
        free((void*)u->type);
        free((void*)u->name);
        free(u);
        u = next;
    }

    // Free inputs
    FXInput* in = shader->inputs;
    while (in) {
        FXInput* next = in->next;
        free((void*)in->type);
        free((void*)in->name);
        free(in);
        in = next;
    }

    // Free functions
    FXFunction* fn = shader->functions;
    while (fn) {
        FXFunction* next = fn->next;
        free((void*)fn->name);
        free((void*)fn->out_type);
        free((void*)fn->out_name);
        FXStatement* stmt = fn->statements;
        while (stmt) {
            FXStatement* next_stmt = stmt->next;
            free((void*)stmt->text);
            free(stmt);
            stmt = next_stmt;
        }
        free(fn);
        fn = next;
    }

    // Free shader itself
    free(shader);
}

static void cleanup_uniform_list(FXUniform* uniforms) {
    FXUniform* u = uniforms;
    while (u) {
        FXUniform* next = u->next;
        free((void*)u->type);
        free((void*)u->name);
        free(u);
        u = next;
    }
}

static void cleanup_input_list(FXInput* inputs) {
    FXInput* in = inputs;
    while (in) {
        FXInput* next = in->next;
        free((void*)in->type);
        free((void*)in->name);
        free(in);
        in = next;
    }
}

static void cleanup_function_list(FXFunction* functions) {
    FXFunction* fn = functions;
    while (fn) {
        FXFunction* next = fn->next;
        free((void*)fn->name);
        free((void*)fn->out_type);
        free((void*)fn->out_name);
        FXStatement* stmt = fn->statements;
        while (stmt) {
            FXStatement* next_stmt = stmt->next;
            free((void*)stmt->text);
            free(stmt);
            stmt = next_stmt;
        }
        free(fn);
        fn = next;
    }
} 