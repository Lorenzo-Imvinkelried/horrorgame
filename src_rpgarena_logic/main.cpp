#include "core/engine/Game.h"
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#include <timeapi.h>
#endif

int main() {
#ifdef _WIN32
    timeBeginPeriod(1); // Configurar resolución del timer de Windows a 1ms (arregla el cap a 30 FPS)
#endif

    try {
        Game game;
        game.run();
    } catch (const std::exception& e) {
        std::cerr << "EXCEPTION: " << e.what() << std::endl;
        std::cin.get(); 
#ifdef _WIN32
        timeEndPeriod(1);
#endif
        return 1;
    } catch (...) {
        std::cerr << "UNKNOWN EXCEPTION" << std::endl;
        std::cin.get();
#ifdef _WIN32
        timeEndPeriod(1);
#endif
        return 1;
    }

#ifdef _WIN32
    timeEndPeriod(1);
#endif
    return 0;
}
