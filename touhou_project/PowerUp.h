#pragma once
#include "Entity.h"
#include <SDL.h>
#include <cstdlib> // 用于随机数

class PowerUp : public Entity
{
public:
    SDL_Texture* texture;
    float gravity; // 重力加速度

    PowerUp(float x, float y, SDL_Texture* tex)
        : Entity(x, y, 20), texture(tex), gravity(800.0f) // 半径稍微大点方便吃
    {
        active = true;

        // ★优化1：物理弹跳效果
        // 不再是死板的 Velocity.y = 100
        // 而是给一个向上的初速度 (弹起来)
        Velocity.y = -400.0f;

        // 给一个随机的左右漂移速度 (-150 到 150)
        // 这样道具会散开，不会叠在一起
        float randomX = (float)(rand() % 300 - 150);
        Velocity.x = randomX;
    }

    void Update(float deltaTime) override
    {
        // ★优化2：施加重力
        Velocity.y += gravity * deltaTime;

        // 限制最大下落速度 (防止掉太快错过)
        if (Velocity.y > 300.0f) Velocity.y = 300.0f;

        // 应用移动
        Entity::Update(deltaTime);

        // ★优化3：左右墙壁反弹
        // 1920宽度的边界保护
        if (Position.x < 10) {
            Position.x = 10;
            Velocity.x = -Velocity.x; // 反弹
        }
        else if (Position.x > 1910) {
            Position.x = 1910;
            Velocity.x = -Velocity.x; // 反弹
        }

        // ★修复BUG：销毁边界适配 1080P
        // 只有掉出屏幕最下方 (1080 + 缓冲) 才销毁
        if (Position.y > 1150.0f) {
            active = false;
        }
    }

    void Render(SDL_Renderer* renderer)
    {
        if (!texture) return;

        // 1. 获取原图大小
        int w, h;
        SDL_QueryTexture(texture, NULL, NULL, &w, &h);

        // 2. 切割第一行第一列 (红色P点)
        SDL_Rect srcRect;
        srcRect.w = w / 5;
        srcRect.h = h / 3;
        srcRect.x = 0;
        srcRect.y = 0;

        // 3. 强制缩放显示
        // 稍微做个呼吸效果或者旋转效果太复杂，先保持简单
        SDL_Rect destRect = {
            (int)(Position.x - 20),
            (int)(Position.y - 20),
            40, 40
        };

        SDL_RenderCopy(renderer, texture, &srcRect, &destRect);
    }
};