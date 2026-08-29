#include "WorldMap.h"
#include <algorithm>

WorldMap::WorldMap() {
    // Configurar marcador del jugador
    mPlayerMarker.setRadius(10.f);
    mPlayerMarker.setOrigin({10.f, 10.f});
    mPlayerMarker.setFillColor(sf::Color::Red);
    mPlayerMarker.setOutlineColor(sf::Color::White);
    mPlayerMarker.setOutlineThickness(2.f);
}

void WorldMap::draw(sf::RenderWindow& window, 
                    const ChunkedTileMap& map, 
                    const DecorSystem& decor, 
                    sf::Vector2f playerPos) 
{
    // 1. Configurar Vista Global
    // Queremos ver TODO el mapa si es posible, o una gran parte.
    // Asumiremos que centramos en el jugador pero con mucho zoom out.
    
    sf::Vector2u mapSize = map.mapSizePx();
    sf::Vector2f currentSize = mView.getSize();
    
    // Si la vista no está inicializada o cambió el tamaño de ventana, reajustamos
    if (currentSize.x == 0.f) {
        // Inicializar con tamaño ventana
        mView = window.getDefaultView();
        // Zoom out masivo: mostrar ~4 pantallas de ancho
        mView.zoom(4.0f); 
    }
    
    // Centrar en jugador pero clmpeado el mundo
    // (Similar a PlayingState pero con vista mas grande)
    sf::Vector2f viewSize = mView.getSize();
    float halfW = viewSize.x * 0.5f;
    float halfH = viewSize.y * 0.5f;
    
    float cx = std::clamp(playerPos.x, halfW, (float)mapSize.x - halfW);
    float cy = std::clamp(playerPos.y, halfH, (float)mapSize.y - halfH);
    
    // Si el mapa es mas chico que la vista, centrar en el medio del mapa
    if (mapSize.x < viewSize.x) cx = mapSize.x * 0.5f;
    if (mapSize.y < viewSize.y) cy = mapSize.y * 0.5f;

    mView.setCenter({cx, cy});

    // 2. Dibujar
    window.setView(mView);

    // Fondo "vacío" (mar fuera del mapa)
    window.clear(sf::Color(10, 10, 20)); 

    // Mapa
    map.drawVisible(window, mView);
    
    // Decoración (Solo arboles/rocas, lo que sea estático)
    // Nota: DecorSystem::drawStaticLayer dibuja TODO. Si es muy pesado, 
    // al estar zoomeado, SFML tendrá que procesar muchos vértices.
    // Dado que tu DecorSystem usa batches, debería aguantar bien.
    decor.drawStaticLayer(window);
    
    // También dibujamos las instancias (árboles sólidos) si queremos verlos
    // El DecorSystem actual tiene drawStaticLayer (pasto) y sprites separados.
    // Deberíamos dibujar los sprites también.
    // Como drawStaticLayer solo dibuja "pasto", nos faltan los árboles.
    // Vamos a iterar las instancias del decorSystem y dibujar las que entren.
    // (Copiado lógica de PlayingState::draw simplificada)
    sf::FloatRect viewRect({cx - halfW, cy - halfH}, viewSize);
    
    for (const auto& inst : decor.getInstances()) {
        if (viewRect.contains(inst.sprite.getPosition())) {
            window.draw(inst.sprite);
        }
    }

    // 3. Marcador Jugador
    mPlayerMarker.setPosition(playerPos);
    window.draw(mPlayerMarker);

    // 4. Restaurar vista (Opcional, pero buena práctica)
    window.setView(window.getDefaultView());
}
