#pragma once
#include <cmath>
#include <SDL.h>

class Vector2D
{
public:
    float x, y;
    Vector2D(float x = 0, float y = 0) : x(x), y(y) {}
};

class Entity
{
public:
    Vector2D Position;
    Vector2D Velocity;
    float radius;
    bool active;

    Entity(float x, float y, float r)
        : Position(x, y), Velocity(0, 0), radius(r), active(true) {
    }

    // ★一定要加上这个虚析构函数！
    virtual ~Entity() {}

    virtual void Update(float deltaTime)
    {
        Position.x += Velocity.x * deltaTime;
        Position.y += Velocity.y * deltaTime;
    }

    // 必须在基类里声明这个虚函数，子类才能 override
    virtual void Render(SDL_Renderer* renderer) {}

    bool CheckCollision(Entity* Other)
    {
        float distX = Position.x - Other->Position.x;
        float distY = Position.y - Other->Position.y;
        float distance = sqrt((distX * distX) + (distY * distY));
        return distance < (radius + Other->radius);
    }
};