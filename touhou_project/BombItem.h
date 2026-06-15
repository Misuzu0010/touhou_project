#pragma once
#include "Item.h"

// ============================================================
// BombItem：符卡道具
// 拾取后增加玩家 Bomb 数量（通过 Player::AddBomb 接口）
// 较为稀有，出现概率约 30%
// ============================================================
class BombItem : public Item
{
public:
    BombItem(float x, float y, SDL_Texture* texture);

    // 拾取效果：玩家 Bomb +1
    void Apply(Player& player) override;

    const char* GetTypeName() const override { return "BombItem"; }
};
