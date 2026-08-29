#pragma once
#include <SFML/Graphics.hpp>
#include "map/ChunkedTileMap.h"
#include "../systems/DecorSystem.h"

class WorldMap {
public:
    WorldMap();

    // Dibuja el mapa completo
    // map: El mapa de tiles
    // decor: El sistema de decoración (árboles)
    // playerPos: La posición del jugador para marcarla
    // window: La ventana, para saber el tamaño y setear la vista
    void draw(sf::RenderWindow& window, 
              const ChunkedTileMap& map, 
              const DecorSystem& decor, 
              sf::Vector2f playerPos);

private:
    sf::View mView;
    sf::CircleShape mPlayerMarker;
    
    // Zoom control? (Tal vez futuro)
    float mZoomLevel = 1.0f;
};
