#include "Game.h"

int main(int argc, char* argv[])
{
    // 程序入口只负责串联 Game 的初始化、主循环和资源释放。
    Game game;
    if (game.Init()) {
        game.Run();
    }
    game.Clean();
    return 0;
}
