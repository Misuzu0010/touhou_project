#pragma once
#include "Entity.h"
#include <SDL.h>
#include <iostream>
#include <cmath>

class Player : public Entity
{
public:
    //float hp;
    //float max_hp;
    int lives;
    int powerLevel;
    int bombs;
    float powerValue;
    float attack_point;
    SDL_Texture* texture;

    float invincTimer;   // 无敌剩余时间
    bool isInvincible;   // 是否处于无敌状态
    bool visible=true;        // 闪烁控制：当前是否显示
    float flashTimer;    // 闪烁频率计时器

    // 动画控制变量
    float visualTime;    // 视觉计时器
    double currentAngle; // 当前倾斜角度

    Player(float x, float y, SDL_Texture* tex)
        : Entity(x, y, 3.0f), lives(3), powerLevel(0),bombs(1), powerValue(0.0f), attack_point(20), texture(tex),
        invincTimer(0.0f), isInvincible(false), flashTimer(0.0f)
    {
    }

    ~Player() {}

    // 重置玩家到初始位置并给予无敌
    void ResetPosition() {
        Position.x = 1920 / 2; // 屏幕底部中央
        Position.y = 900;
        isInvincible = true;
        invincTimer = 3.0f;    // 3秒无敌
        flashTimer = 0.1f;
        visible = true;
    }

    void Update(float deltaTime) override
    {
        const Uint8* key = SDL_GetKeyboardState(NULL);

        // --- 0. 无敌与闪烁逻辑处理 ---
        if (invincTimer > 0) {
            invincTimer -= deltaTime;
            flashTimer -= deltaTime;

            // 实现闪烁特效：每 0.1 秒切换一次可见性
            if (flashTimer <= 0) {
                visible = !visible;
                flashTimer = 0.15f;
            }

            if (invincTimer <= 0) {
                isInvincible = false;
                visible = true; // 确保无敌结束后是显示的
            }
        }
        else {
            visible = true; // 确保非无敌状态不消失
        }

        // --- 1. 基础移动 (保持不变) ---
        float baseSpeed = 800.0f;
        if (key[SDL_SCANCODE_LSHIFT]) baseSpeed = 400.0f;

        Vector2D input(0, 0);
        if (key[SDL_SCANCODE_UP]) input.y -= 1;
        if (key[SDL_SCANCODE_DOWN]) input.y += 1;
        if (key[SDL_SCANCODE_LEFT]) input.x -= 1;
        if (key[SDL_SCANCODE_RIGHT]) input.x += 1;

        Position.x += input.x * baseSpeed * deltaTime;
        Position.y += input.y * baseSpeed * deltaTime;

        // 边界限制
        if (Position.x < 30) Position.x = 30;
        if (Position.x > 1890) Position.x = 1890;
        if (Position.y < 30) Position.y = 30;
        if (Position.y > 1050) Position.y = 1050;

        // --- 2. 动态效果计算 ---

        // (A) 计时器：控制呼吸频率
        visualTime += deltaTime * 3.0f; // 稍微快一点的呼吸节奏

        // (B) 倾斜逻辑 (Lerp)
        double targetAngle = 0.0;
        // 移动时倾斜角度大一点 (15度)
        if (input.x < 0) targetAngle = -15.0;
        else if (input.x > 0) targetAngle = 15.0;

        // 简单的平滑插值公式
        currentAngle += (targetAngle - currentAngle) * 12.0f * deltaTime;
    }

    void Render(SDL_Renderer* renderer) override
    {
        // ★★★ 只有当 visible 为 true 时才渲染人物 (实现闪烁) ★★★
        if (texture && visible) {
            int texW, texH;
            SDL_QueryTexture(texture, NULL, NULL, &texW, &texH);

            // 基础显示大小
            float aspect = (float)texW / texH;
            float baseH = 100.0f;
            float baseW = baseH * aspect;

            // ★★★ 3. 注入灵魂：呼吸缩放算法 (Squash & Stretch) ★★★
            // scaleFactor 在 0.95 到 1.05 之间波动
            float scaleFactor = 1.0f + std::sin(visualTime) * 0.05f;

            // 宽度变大时，高度变小（保持体积感）
            // 这种反向变化是动画的精髓！
            float currentW = baseW * scaleFactor;        // 变宽
            float currentH = baseH * (2.0f - scaleFactor); // 变矮

            // ★★★ 4. 漂浮偏移 ★★★
            // 让漂浮的节奏和呼吸错开一点 (visualTime * 0.5)，看起来更自然
            float floatOffset = std::sin(visualTime * 0.5f) * 8.0f;

            // 计算最终矩形 (居中绘制)
            SDL_Rect dest = {
                (int)(Position.x - currentW / 2),
                (int)(Position.y - currentH / 2 + floatOffset),
                (int)currentW,
                (int)currentH
            };

            // 绘制 (带旋转)
            SDL_RenderCopyEx(renderer, texture, NULL, &dest, currentAngle, NULL, SDL_FLIP_NONE);

            // 判定点 (Shift模式)
            const Uint8* key = SDL_GetKeyboardState(NULL);
            if (key[SDL_SCANCODE_LSHIFT]) {
                SDL_SetRenderDrawColor(renderer, 255, 50, 50, 255);
                int hitboxSize = 8;
                SDL_Rect hitRect = {
                    (int)Position.x - hitboxSize / 2,
                    (int)Position.y - hitboxSize / 2,
                    hitboxSize,
                    hitboxSize
                };
                SDL_RenderFillRect(renderer, &hitRect);
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                SDL_RenderDrawRect(renderer, &hitRect);
            }
        }
    }

    void hit(float damage) {
        // 外部逻辑会检查 isInvincible，这里保留作为兜底
        if (!isInvincible) lives--;
    }

    bool Dead_judge() { return lives < 0; }

    int CollectPowerUp() {

        if (powerLevel >= 4) return 0; // 假设最高 4.00

        powerValue += 0.05f; // 每个 P 点增加 0.05

        if (powerValue >= 1.0f) {
            powerValue -= 1.0f; // 溢出转入下一级
            powerLevel++;
            return 1; // 升级了
        }
        return 2; // 捡到了但未升级
    }
};