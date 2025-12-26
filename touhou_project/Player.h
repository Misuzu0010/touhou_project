#pragma once
#include "Entity.h"
#include <SDL.h>
#include <iostream>

class Player : public Entity
{
public:
    float hp;
    float max_hp;
    int powerLevel;
    int powerCount;
    float attack_point;
    SDL_Texture* texture;

    // ★修改点1：构造函数中，radius 设为 3.0f (判定点极小)
    // 即使图片很大，只有这 3 像素的半径会触发碰撞
    Player(float x, float y, SDL_Texture* tex)
        : Entity(x, y, 3.0f), hp(100), max_hp(100), powerLevel(0), powerCount(0), attack_point(20), texture(tex)
    {
    }

    ~Player() {}

    void Update(float deltaTime) override
    {
        const Uint8* key = SDL_GetKeyboardState(NULL);

        // ★修改点2：1080P下的速度调整
        // 普通速度 800，按住 Shift 减半
        float baseSpeed = 800.0f;
        if (key[SDL_SCANCODE_LSHIFT]) baseSpeed = 400.0f;

        if (key[SDL_SCANCODE_UP]) Position.y -= baseSpeed * deltaTime;
        if (key[SDL_SCANCODE_DOWN]) Position.y += baseSpeed * deltaTime;
        if (key[SDL_SCANCODE_LEFT]) Position.x -= baseSpeed * deltaTime;
        if (key[SDL_SCANCODE_RIGHT]) Position.x += baseSpeed * deltaTime;

        // ★修改点3：1920x1080 的边界限制
        // 留出一点边距，别让角色完全跑出屏幕
        if (Position.x < 30) Position.x = 30;
        if (Position.x > 1890) Position.x = 1890;
        if (Position.y < 30) Position.y = 30;
        if (Position.y > 1050) Position.y = 1050;
    }

    void Render(SDL_Renderer* renderer) override
    {
        if (texture) {
            // 1. 获取原图大小 (比如 1280x1706)
            int texW, texH;
            SDL_QueryTexture(texture, NULL, NULL, &texW, &texH);

            // 2. 计算显示大小 (强制缩放)
            // 假设我们希望角色在 1080P 屏幕上高约 100 像素，保持比例
            // 宽高比 = texW / texH
            float aspect = (float)texW / texH;
            int displayH = 100; // 设定高度
            int displayW = (int)(displayH * aspect); // 自动算宽度

            // 3. 绘制角色立绘
            // destRect 的中心点应该是 Position
            SDL_Rect dest = {
                (int)Position.x - displayW / 2,
                (int)Position.y - displayH / 2,
                displayW,
                displayH
            };

            // srcRect 为 NULL 表示画整张原图
            SDL_RenderCopy(renderer, texture, NULL, &dest);

            // ★修改点4：绘制判定点 (Hitbox)
            // 只有按下 Shift (低速模式) 时才显示，这是 STG 的传统！
            const Uint8* key = SDL_GetKeyboardState(NULL);
            if (key[SDL_SCANCODE_LSHIFT]) {
                // 画一个鲜艳的实心方块/圆点
                SDL_SetRenderDrawColor(renderer, 255, 50, 50, 255); // 亮红色
                int hitboxSize = 8; // 视觉上的判定点大小 (比实际碰撞半径稍大一点点好辨认)
                SDL_Rect hitRect = {
                    (int)Position.x - hitboxSize / 2,
                    (int)Position.y - hitboxSize / 2,
                    hitboxSize,
                    hitboxSize
                };
                SDL_RenderFillRect(renderer, &hitRect);

                // 再画个白框框住红点，更显眼
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                SDL_RenderDrawRect(renderer, &hitRect);
            }
        }
    }

    void hit(float damage) { hp -= damage; }
    bool Dead_judge() { return hp <= 0; }
    float get_hp() { return hp; }

    void CollectPowerUp() {
        powerCount++;
        if (powerCount % 10 == 0 && powerLevel < 3) powerLevel++;
        hp += 5;
        if (hp > max_hp) hp = max_hp;
    }
};