#include "Renderer.h"
#include <iostream>
#include <cstdlib>
#include <exception>

int main (int argc, char *argv[]) {
    Renderer renderer;

    try {
        renderer.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}
