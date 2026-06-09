#pragma once
#include "Entity.h"
#include <SDL.h>

class Player; // 前向声明

// 子弹贴图和逻辑类型。敌弹和自机弹共用 Bullet 类，通过类型决定贴图切片和判定大小。
enum class BulletType {
    VIRUS,          // 敌人：病毒球
    LOCK,           // 敌人：锁
    SHURIKEN,       // 敌人：手里剑
    PLAYER_TALISMAN,// 玩家：灵梦符札
    PLAYER_STAR     // 玩家：魔理沙星星
};

// 子弹运动状态：
// NORMAL    直接按 Velocity 移动。
// WAITING   原地等待，倒计时结束后朝 targetPlayer 发射。
// AIMING    预留状态，当前未被使用。
// UNFOLDING 从 Boss 中心向外展开，展开结束后进入 WAITING。
enum class BulletState { NORMAL, WAITING, AIMING, UNFOLDING };

// 单颗子弹实体。既负责运动状态更新，也负责根据 BulletType 从图集中取对应贴图。
class Bullet : public Entity
{
private:
    BulletState state; // 当前运动阶段。
    BulletType type;   // 决定判定半径和渲染切片。

    float waitTimer;       // WAITING 阶段剩余等待时间。
    float finalSpeed;      // 等待结束后朝玩家飞行的速度。
    float unfoldTimer;     // UNFOLDING 阶段剩余展开时间。
    Player* targetPlayer;  // 非拥有指针，仅用于追踪弹计算玩家当前位置。

public:
    // 普通直线子弹：创建后立即按 speed_x/speed_y 移动。
    Bullet(float x, float y, float speed_x, float speed_y, BulletType type = BulletType::VIRUS);

    // 延迟追踪子弹：先等待 waitTime，再以 speed 朝 player 当前方向飞出。
    Bullet(float x, float y, float waitTime, float speed, Player* player, BulletType type = BulletType::VIRUS);

    void Update(float deltaTime) override;

    BulletType GetType() const { return type; }
    BulletState GetState() const { return state; }
    void StartUnfolding(float timer, float velocityX, float velocityY)
    {
        state = BulletState::UNFOLDING;
        unfoldTimer = timer;
        SetVelocity(velocityX, velocityY);
    }

    // 子弹渲染需要外部传入图集纹理，因此签名不同于 Entity::Render，不作为 override。
    void Render(SDL_Renderer* renderer, SDL_Texture* texture);
};
