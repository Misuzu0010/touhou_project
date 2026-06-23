#pragma once
#include "Entity.h"
#include "Player.h"
#include <SDL.h>

// ============================================================
// Item：道具抽象基类，继承自 Entity
// 所有具体道具（PowerItem/BombItem/LifeItem）都继承此类，
// 并重写纯虚函数 Apply() 实现各自拾取效果。
// Game 只需持有 Item 基类指针，通过多态调用 Apply()，
// 无需关心具体道具类型——这正是多态的核心价值。
// ============================================================
class Item : public Entity
{
public:
    // 道具下落重力加速度（像素/秒^2），与 PowerUp 保持一致。
    static constexpr float GRAVITY = 800.0f;

    // 道具最大下落速度，避免道具几乎无法拾取。
    static constexpr float MAX_FALL_SPEED = 300.0f;

    // 构造：位置 + 贴图 + 碰撞半径
    Item(float x, float y, SDL_Texture* texture, float r = 20.0f);

    virtual ~Item() = default;

    // 纯虚函数：派生类必须实现，定义拾取后对玩家的具体效果。
    // 玩家通过引用传入，Item 不直接访问 Player 的 private 数据成员。
    virtual void Apply(Player& player) = 0;

    // 每帧更新：道具受重力下落，左右边界反弹，超出底部标记失效。
    void Update(float deltaTime) override;

    // 渲染道具贴图，带简单的呼吸光晕效果。
    void Render(SDL_Renderer* renderer) override;

    // 返回道具类型名称（用于调试/日志输出）
    virtual const char* GetTypeName() const = 0;

private:
    // 非拥有指针：贴图由 Game 统一加载和释放。
    SDL_Texture* texture;

    // 重力加速度，与 PowerUp 的 gravity 字段保持一致。
    float gravity;

    // 呼吸光晕动画计时器
    float animTimer_ = 0.0f;
};
