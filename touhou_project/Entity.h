#pragma once
#include <cmath>
#include <SDL.h>

class Vector2D
{
public:
    // 简单二维向量，用于保存实体的位置和速度。
    float x, y;
    Vector2D(float x = 0, float y = 0) : x(x), y(y) {}
};

// 所有可更新、可渲染、可碰撞对象的公共基类。
// 约定：Position 表示中心点坐标，radius 表示圆形判定半径，active=false 表示等待清理。
class Entity
{
protected:
    Vector2D Position; // 中心点位置，屏幕坐标系：右为 +x，下为 +y。
    Vector2D Velocity; // 每秒移动速度，单位为像素/秒。
    float radius;      // 圆形碰撞半径，不一定等于贴图显示大小。
    bool active;       // 对象是否仍参与更新、渲染和碰撞。

public:
    Entity(float x, float y, float r)
        : Position(x, y), Velocity(0, 0), radius(r), active(true) {
    }

    // 通过 Entity 指针/引用操作派生类时，需要虚析构保证派生类析构链正确。
    virtual ~Entity() {}

    // 默认移动逻辑：按速度和 deltaTime 推进位置。
    // 子类可以覆盖，但仍可调用 Entity::Update 复用基础位移。
    virtual void Update(float deltaTime)
    {
        Position.x += Velocity.x * deltaTime;
        Position.y += Velocity.y * deltaTime;
    }

    // 默认不绘制；需要显示的派生类覆盖该函数。
    virtual void Render(SDL_Renderer* renderer) {}

    const Vector2D& GetPosition() const { return Position; }
    void SetPosition(float x, float y) { Position.x = x; Position.y = y; }

    const Vector2D& GetVelocity() const { return Velocity; }
    void SetVelocity(float x, float y) { Velocity.x = x; Velocity.y = y; }

    float GetRadius() const { return radius; }
    void SetRadius(float newRadius) { radius = newRadius; }

    bool IsActive() const { return active; }
    void Deactivate() { active = false; }
    void Activate() { active = true; }

    // 圆形碰撞检测。注意 Other 必须非空，调用方负责保证对象有效。
    bool CheckCollision(const Entity* Other) const
    {
        float distX = Position.x - Other->Position.x;
        float distY = Position.y - Other->Position.y;
        float distance = sqrt((distX * distX) + (distY * distY));
        return distance < (radius + Other->radius);
    }

};
