# FX Shader System

A minimal, handmade shader system inspired by RAD Game Tools, Casey Muratori, and Jonathan Blow. This project provides a custom .fx shader language (HLSL-lite style), a compiler CLI tool, and a C runtime loader for OpenGL 3.3 core.

## Philosophy

- **Minimal Dependencies**: Pure C implementation with no external libraries except system OpenGL
- **Full Understanding**: Every line of code is written by hand, no black boxes
- **Production Quality**: Real, working code, not boilerplate or stubs
- **Handmade OpenGL Loader**: Custom function pointer loader, no gl3w or similar

## Project Structure

```
├── src/           # Source code
│   ├── fxc.c      # FX compiler (lexer, parser, codegen)
│   ├── fx_gl.h    # OpenGL loader header
│   ├── fx_gl.c    # OpenGL loader implementation
│   ├── fx_runtime.h # Runtime API header
│   └── fx_runtime.c # Runtime implementation
├── tests/         # Test shaders and test code
│   └── test.fx    # Test shader file
├── examples/      # Example usage
│   ├── example.c  # Runtime usage example
│   └── basic_lit.fx # Example shader
├── bin/           # Build outputs
│   └── fxc.exe    # Compiled compiler tool
└── build.bat      # Build script
```

## Features

### FX Shader Language (.fx)
- HLSL-lite syntax with custom keywords
- Vertex and fragment shader definitions
- Attribute and uniform declarations
- Type system: float, float2, float3, float4, float4x4
- Built-in variables: gl_Position, SV_Target

### Compiler (fxc)
- Handwritten lexer and recursive descent parser
- Generates separate vertex and fragment GLSL files
- Outputs metadata for runtime binding
- Command-line interface: `fxc input.fx`

### Runtime Loader
- Pure C OpenGL 3.3 core loader (no external dependencies)
- Loads and compiles vertex/fragment shaders
- Binds uniforms and attributes
- Resource management and cleanup
- Optional live reloading support

## Building

### Prerequisites
- GCC or compatible C compiler
- Windows (for OpenGL loader)
- OpenGL 3.3+ capable graphics driver

### Build Commands
```bash
# Build the compiler and runtime
.\build.bat

# Or manually:
gcc -std=c99 -Wall -Wextra -O2 -c src\fx_gl.c -o bin\fx_gl.o
gcc -std=c99 -Wall -Wextra -O2 -c src\fx_runtime.c -o bin\fx_runtime.o
gcc -std=c99 -Wall -Wextra -O2 -c src\fxc.c -o bin\fxc.o
gcc -std=c99 -Wall -Wextra -O2 -o bin\fxc bin\fxc.o bin\fx_gl.o bin\fx_runtime.o -lgdi32 -lopengl32
```

## Usage

### Compiling Shaders
```bash
# Compile a .fx file to GLSL
bin\fxc tests\test.fx

# This generates:
# - test.vert.glsl (vertex shader)
# - test.frag.glsl (fragment shader)
# - test.meta (metadata for runtime)
```

### Runtime Usage
```c
#include "src/fx_runtime.h"

// Initialize OpenGL loader
fxgl_init();

// Load shader
FXShader* shader = fx_load("test.vert.glsl", "test.frag.glsl", "test.meta");

// Use shader
fx_use(shader);

// Set uniforms
fx_set_uniform_float(shader, "time", 1.0f);
fx_set_uniform_vec3(shader, "color", 1.0f, 0.0f, 0.0f);

// Cleanup
fx_cleanup(shader);
```

## Example Shader

```hlsl
// tests/test.fx
vertex_shader main_vs(
    float3 position : POSITION,
    float3 color : COLOR
) {
    out float3 v_color : COLOR;
    v_color = color;
    gl_Position = float4(position, 1.0);
}

fragment_shader main_fs(
    float3 v_color : COLOR
) {
    out float4 fragColor : SV_Target;
    fragColor = float4(v_color, 1.0);
}
```

## Technical Details

### OpenGL Loader
- Handwritten function pointer declarations
- Runtime loading via wglGetProcAddress
- Fallback to GetProcAddress for core functions
- No external loader libraries

### Compiler Pipeline
1. **Lexer**: Tokenizes .fx source into tokens
2. **Parser**: Recursive descent parser builds AST
3. **Codegen**: Generates GLSL from AST
4. **Metadata**: Outputs binding information

### Runtime Features
- Shader compilation and linking
- Uniform and attribute binding
- Resource management
- Error handling and logging

## License

This project is provided as-is for educational and development purposes.

## Contributing

This is a learning project focused on understanding graphics programming fundamentals. Feel free to study, modify, and experiment with the code. 