// main.cpp
#include <iostream>
#include "system.h"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <rom_file>" << std::endl;
        return 1;
    }

    GBASystem gba;
    gba.init();

    if (!gba.load_rom(argv[1])) {
        return 1;
    }

    gba.running = true;

    while (gba.running) {
        gba.run_frame(); // Try and hope to maybe run a frame
        //TODO: A lot
        break;
    }

    return 0;
}