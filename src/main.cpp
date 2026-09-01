#include "core/GameApp.h"
#include <iostream>
#include <fstream>

#ifndef __EMSCRIPTEN__
struct CoutRedirector {
    std::streambuf* oldBuf;
    std::ofstream file;
    CoutRedirector(const std::string& filename) : file(filename) {
        oldBuf = std::cout.rdbuf();
        std::cout.rdbuf(file.rdbuf());
        std::cout << std::unitbuf;
    }
    ~CoutRedirector() {
        std::cout.rdbuf(oldBuf);
    }
};
#endif

int main(int argc, char** argv) {
#ifndef __EMSCRIPTEN__
    CoutRedirector redirect("log.txt");
#endif

    GameApp app;
    if (!app.Init()) {
        std::cerr << "[VRAM DUNGEON] Fatal: Failed to initialize Game Application." << std::endl;
        return -1;
    }

    app.Run();
    return 0;
}
