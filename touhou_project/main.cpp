#include "Game.h"

int main(int argc, char* argv[])
{
    // 首先，创造一个游戏“大盒子”的实例喵
    Game game;

    // 然后，我们调用 Init() 来初始化游戏，如果成功了……
    if (game.Init())
    {
        // ……就调用 Run() 来开始真正的游戏循环！
        // Run() 会一直运行，直到游戏结束为止哦
        game.Run();
    }

    // 当游戏循环结束（比如玩家关闭了窗口），我们就调用 Clean() 来打扫战场，释放所有资源
    game.Clean();

    // 最后，程序开开心心地结束啦~
    return 0;
}