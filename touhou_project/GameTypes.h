#pragma once
#include <SDL.h>
#include <string>
#include <vector>

// 单句剧情文本。name 和 Content 使用 UTF-8 字符串，color 用于说话者名称颜色。
// 从 Game.h 提取到独立头文件，避免 Strategy/Enemy/Game 之间的循环依赖。
struct DialogueLine {
    std::string name;
    std::string Content;
    SDL_Color color;
};

// 可选角色 ID，用于对话生成和 Boss 阶段配置。
enum class CharacterID { REIMU = 0, MARISA = 1 };
