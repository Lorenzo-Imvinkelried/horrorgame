#pragma once // aca evita que el archivo se incluya varias veces y cause errores
#include <SFML/Graphics.hpp> // aca importa las herramientas gráficas de SFML (ventanas, texturas, etc.)
#include <memory> // aca permite usar punteros inteligentes como unique_ptr
#include <stack> // aca importa la estructura de datos tipo pila (aunque usamos vector para los estados)
#include "ResourceManager.h" // aca incluye el gestor que carga y guarda texturas y sonidos
#include "InputManager.h" // aca incluye el sistema que detecta las teclas y clics del mouse
#include "../items/ItemManager.h" // aca incluye el gestor que maneja la base de datos de ítems
#include "../items/LootManager.h"
#include "../systems/GoldSystem.h"
#include "states/GameState.h" // aca incluye la clase base para los diferentes estados del juego
#include "core/systems/PostFXSystem.h" // [POST FX] Post-processing visual polish

class Game { // aca define la clase principal que controla todo el motor del juego
public: // aca indica que lo que sigue es accesible desde fuera de esta clase
//funciones expuestas al mundo, por ejemplo donde creo una instancia de game puedo usar sus funciones publicas
    Game(); // aca es el Constructor: inicializa el juego, la ventana y los recursos
    ~Game(); // aca es el Destructor: limpia la memoria cuando el juego se cierra
    void run(); // aca es el bucle principal que mantiene el juego corriendo a 60 FPS

    // --- ACCESOS GLOBALES (Para que los States los usen) ---
    sf::RenderWindow& getWindow() { return mWindow; } // aca devuelve una referencia a la ventana real de Windows
    //sf es el nombre de la libreria sfml, RenderWindow es una clase de esa libreria que representa la ventana
    //paso por referencia la ventana original, son & seria por copia
    // Los estados ahora dibujan en un RenderTarget (puede ser la ventana O la textura)
    sf::RenderTarget& getRenderTarget() { // aca decide dónde dibujar (en la ventana o en una textura de baja resolución)
        if (mUseVirtualResolution) return mRenderTexture; // aca si usamos resolución virtual, dibuja en la textura pequeña
        return mWindow; // aca si no, dibuja directamente en la ventana
    }
    const sf::RenderTexture& getRenderTexture() const { return mRenderTexture; }
    bool isUsingVirtualResolution() const { return mUseVirtualResolution && (mRenderTexture.getSize() != mWindow.getSize()); }

    const InputManager& getInput() const { return mInputManager; } // aca permite leer las entradas del teclado desde otros lugares
    ResourceManager& getResources() { return mResourceManager; } // aca permite acceder a las texturas cargadas desde otros sitios
    ItemManager& getItemManager() { return mItemManager; } // aca permite acceder a la lista de ítems desde el inventario o tiendas
    LootManager& getLootManager() { return mLootManager; }
    GoldSystem& getGoldSystem() { return mGoldSystem; }
    PostFXSystem& getPostFX() { return mPostFX; } // [POST FX] Getter for post-processing system
    
    // --- GESTIÓN DE ESTADOS ---
    void changeState(std::unique_ptr<GameState> state); // aca borra el estado actual y pone uno nuevo (ej: de Menú a Juego)
    void pushState(std::unique_ptr<GameState> state); // aca pone un estado encima de otro (ej: abrir Inventario sobre el Juego)
    void popState(); // aca quita el estado de arriba (ej: cerrar el Inventario)
    
    // [VIRTUAL RESOLUTION] Sub-pixel offset for smooth movement on low res
    void setVirtualOffset(sf::Vector2f offset) { mVirtualOffset = offset; }

private: // aca indica que lo que sigue solo puede ser usado por esta clase internamente
    void processEvents(); // aca maneja eventos del sistema como cerrar la ventana o cambiar su tamaño
    void update(sf::Time dt); // aca actualiza la lógica (movimiento, colisiones) usando el tiempo transcurrido
    void render(); // aca se encarga de dibujar todo en pantalla cada frame

private: // aca define las variables (propiedades) privadas de la clase
    sf::RenderWindow mWindow; // aca es el objeto que representa la ventana física en tu monitor
    
    // [VIRTUAL RESOLUTION]
    bool mUseVirtualResolution = false; // aca indica si el juego debe verse pixelado (baja resolución) o nítido
    sf::RenderTexture mRenderTexture; // aca es un lienzo invisible donde dibujamos si la resolución es virtual
    sf::Sprite mRenderSprite; // aca es el sprite que estira ese lienzo invisible para que ocupe toda la ventana

    InputManager     mInputManager; // aca guarda el estado actual de todas las teclas y el mouse
    ResourceManager  mResourceManager; // aca guarda todas las imágenes y sonidos en memoria para no recargarlos
    ItemManager      mItemManager; // aca guarda la lógica y base de datos de todos los objetos del juego
    LootManager      mLootManager;
    GoldSystem       mGoldSystem;
    sf::Vector2f mVirtualOffset = {0.f, 0.f}; // aca guarda el pequeño desplazamiento para suavizar el movimiento en baja resolución
    PostFXSystem mPostFX; // [POST FX] Post-processing visual polish system
    sf::RenderTexture mCombinedTexture; // [DEATH GRayscale] Temporary texture to draw everything (including UI) for fullscreen desaturation
    
    // Pila de estados (El de arriba es el activo)
    std::vector<std::unique_ptr<GameState>> mStates; // aca es la lista (pila) de pantallas activas en el juego
};