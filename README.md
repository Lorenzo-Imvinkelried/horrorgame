# VRAM DUNGEON (Retro 3D ARPG)

Un juego ARPG y supervivencia 3D con estética retro PS1 y sistemas procedurales, desarrollado en C++ puro con OpenGL y compilable tanto para Windows como para WebAssembly.

## Requisitos Previos

Para compilar y correr este proyecto en otra PC (Windows), necesitarás instalar:

1.  **Git**: Para clonar el repositorio.
    *   [Descargar Git](https://git-scm.com/downloads)
2.  **CMake** (Versión 3.20 o superior): Para generar los archivos del proyecto.
    *   [Descargar CMake](https://cmake.org/download/)
3.  **Compilador de C++ (C++20)**:
    *   Recomendado: **Visual Studio 2019/2022** (Select "Desktop development with C++").
    *   Alternativa: **MinGW-w64** (asegúrate de que soporte C++20).

## Instrucciones de Instalación

1.  **Clonar el repositorio:**

    ```bash
    git clone https://github.com/Lorenzo-Imvinkelried/horrorgame.git
    cd horrorgame
    ```

2.  **Descargar dependencias:**
    Ejecuta el script de PowerShell para descargar las librerías necesarias (SFML, GLM, etc.) en la carpeta `libs/`.

    ```powershell
    ./download_deps.ps1
    ```

3.  **Compilar con CMake:**

    ```bash
    mkdir build
    cd build
    cmake ..
    cmake --build . --config Release
    ```

4.  **Ejecutar el juego:**
    El ejecutable y los assets se encontrarán en `build/bin/Release` (o `build/bin` dependiendo de tu generador).

    ```bash
    ./bin/Release/GamePS1Horror.exe
    ```

## Estructura del Proyecto

*   `src/`: Código fuente C++.
*   `assets/`: Modelos, texturas y shaders.
*   `libs/`: Archivos .zip de dependencias (generados por `download_deps.ps1`).
*   `cmake/`: Scripts adicionales de CMake (si los hubiera).

## Notas

*   Este proyecto usa **FetchContent** de CMake apuntando a archivos locales en `libs/`, por lo que es **obligatorio** correr `download_deps.ps1` al menos una vez antes de configurar CMake.
