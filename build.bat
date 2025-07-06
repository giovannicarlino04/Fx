@echo off
gcc -std=c99 -Wall -Wextra -O2 -c src\fx_gl.c -o bin\fx_gl.o
gcc -std=c99 -Wall -Wextra -O2 -c src\fx_runtime.c -o bin\fx_runtime.o
gcc -std=c99 -Wall -Wextra -O2 -c src\fxc.c -o bin\fxc.o
gcc -std=c99 -Wall -Wextra -O2 -o bin\fxc bin\fxc.o bin\fx_gl.o bin\fx_runtime.o -lgdi32 -lopengl32
echo Build complete.