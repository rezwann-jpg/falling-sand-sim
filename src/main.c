#include "game.h"

int main(int argc, char* argv[]) {
    if (!init()) {
        return 1;
    }

    run();

    return 0;
}
