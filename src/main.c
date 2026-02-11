#include "game.h"

int main(int argc, char* argv[]) {
    if (!init()) {
        return 1;
    }

    run();

    cleanup();

    return 0;
}
