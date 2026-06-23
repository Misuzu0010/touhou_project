#pragma once
#include "Item.h"

// ============================================================
// LifeItem：残机道具
// 拾取后增加玩家剩余残机数（通过 Player::AddLife 接口）
// 最稀有，出现概率约 20%
// ============================================================
class LifeItem : public Item
{
public:
    LifeItem(float x, float y, SDL_Texture* texture);

    // 拾取效果：玩家残机 +1
    void Apply(Player& player) override;

    const char* GetTypeName() const override { return "LifeItem"; }
};
