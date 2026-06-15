#pragma once
#include "Item.h"

// ============================================================
// PowerItem：火力道具
// 拾取后增加玩家火力等级（通过 Player::CollectPowerUp 接口）
// 这是游戏中最常见的道具类型
// ============================================================
class PowerItem : public Item
{
public:
    PowerItem(float x, float y, SDL_Texture* texture);

    // 拾取效果：调用 Player::CollectPowerUp() 增加火力
    // 返回值 0=已满/1=升级/2=仅增加进度，由 Game 决定是否播放音效
    int ApplyReturn(Player& player);

    // 纯虚函数实现：内部调用 CollectPowerUp
    void Apply(Player& player) override;

    const char* GetTypeName() const override { return "PowerItem"; }
};
