#pragma execution_character_set("utf-8")
#include "PowerItem.h"
#include "BombItem.h"
#include "LifeItem.h"

// ================================================================
// PowerItem 实现
// ================================================================
PowerItem::PowerItem(float x, float y, SDL_Texture* texture)
    : Item(x, y, texture, 20.0f)   // 碰撞半径 20，与原 PowerUp 一致
{}

int PowerItem::ApplyReturn(Player& player)
{
    return player.CollectPowerUp();   // 委托给 Player 的火力增加接口
}

void PowerItem::Apply(Player& player)
{
    ApplyReturn(player);              // 统一通过 Apply 接口调用
}

// ================================================================
// BombItem 实现
// ================================================================
BombItem::BombItem(float x, float y, SDL_Texture* texture)
    : Item(x, y, texture, 18.0f)   // 碰撞半径稍小
{}

void BombItem::Apply(Player& player)
{
    player.AddBomb(1);    // 调用 Player 接口增加 Bomb
}

// ================================================================
// LifeItem 实现
// ================================================================
LifeItem::LifeItem(float x, float y, SDL_Texture* texture)
    : Item(x, y, texture, 22.0f)   // 碰撞半径最大，方便拾取
{}

void LifeItem::Apply(Player& player)
{
    player.AddLife(1);    // 调用 Player 接口增加残机
}
