#include "Game.h"

int main(int argc, char* argv[])
{
    Game game;
    if (game.Init()) {
        game.Run();
    }
    game.Clean();
    return 0;
}