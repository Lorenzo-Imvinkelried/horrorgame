#include "core/GameApp.h"
#include <iostream>
#include <fstream>

#ifndef __EMSCRIPTEN__
struct CoutRedirector {
    std::streambuf* oldOut;
    std::streambuf* oldErr;
    std::ofstream file;
    CoutRedirector(const std::string& filename) : file(filename, std::ios::out | std::ios::trunc) {
        oldOut = std::cout.rdbuf();
        oldErr = std::cerr.rdbuf();
        std::cout.rdbuf(file.rdbuf());
        std::cerr.rdbuf(file.rdbuf());
        std::cout << std::unitbuf;
        std::cerr << std::unitbuf;
    }
    ~CoutRedirector() {
        std::cout.rdbuf(oldOut);
        std::cerr.rdbuf(oldErr);
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
