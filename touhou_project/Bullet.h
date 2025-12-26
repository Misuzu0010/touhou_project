#pragma once
#include "Entity.h"
#include <SDL.h>

class Player; // 前向声明

enum class BulletType {
    VIRUS,          // 敌人：病毒球
    LOCK,           // 敌人：锁
    SHURIKEN,       // 敌人：手里剑
    PLAYER_TALISMAN,// 玩家：灵梦符札
    PLAYER_STAR     // 玩家：魔理沙星星
};

enum class BulletState { NORMAL, WAITING, AIMING };

class Bullet : public Entity
{
public:
    BulletState state;
    BulletType type;

    float waitTimer;
    float finalSpeed;
    Player* targetPlayer;

    Bullet(float x, float y, float speed_x, float speed_y, BulletType type = BulletType::VIRUS);
    Bullet(float x, float y, float waitTime, float speed, Player* player, BulletType type = BulletType::VIRUS);

    void Update(float deltaTime) override;
    void Render(SDL_Renderer* renderer, SDL_Texture* texture); // 注意这里签名变了，不override基类
};