#pragma once
#include "Entity.h"
#include <SDL.h>
#include <iostream>
#include <cmath>

class Player : public Entity
{
private:
    // 玩家当前使用残机数。为 0 时进入 Game Over。
    int lives;

    // 火力由整数等级和小数进度组成，例如 powerLevel=2、powerValue=0.35 表示 2.35。
    int powerLevel;
    float powerValue;

    // 可用符卡/Bomb 数量，当前每局默认 1 个。
    int bombs;

    // 玩家子弹命中 Boss 时造成的单发伤害。
    float attack_point;

    // 非拥有指针：贴图由 Game 统一加载和释放。
    SDL_Texture* texture;

    float invincTimer;   // 无敌剩余时间
    bool isInvincible;   // 是否处于无敌状态
    bool visible=true;   // 闪烁控制：当前是否显示
    float flashTimer;    // 闪烁频率计时器

    // 视觉动画控制：呼吸缩放、上下漂浮和左右倾斜都依赖这两个值。
    float visualTime;    // 视觉计时器
    double currentAngle; // 当前倾斜角度

public:
    Player(float x, float y, SDL_Texture* tex)
        : Entity(x, y, 3.0f), lives(3), powerLevel(0),bombs(1), powerValue(0.0f), attack_point(20), texture(tex),
        invincTimer(0.0f), isInvincible(false), flashTimer(0.0f), visualTime(0.0f), currentAngle(0.0)
    {
    }

    ~Player() {}

    // Miss 或续关后重置玩家位置，并给予短暂无敌避免立刻二次受击。
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

        // 处理无敌时间和闪烁显示。Render 会根据 visible 决定是否绘制玩家。
        if (invincTimer > 0) {
            invincTimer -= deltaTime;
            flashTimer -= deltaTime;

            // 通过周期性切换 visible 实现受击无敌闪烁。
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

        // 读取键盘输入。按住左 Shift 时进入低速移动，方便精确避弹。
        float baseSpeed = 800.0f;
        if (key[SDL_SCANCODE_LSHIFT]) baseSpeed = 400.0f;

        Vector2D input(0, 0);
        if (key[SDL_SCANCODE_UP]) input.y -= 1;
        if (key[SDL_SCANCODE_DOWN]) input.y += 1;
        if (key[SDL_SCANCODE_LEFT]) input.x -= 1;
        if (key[SDL_SCANCODE_RIGHT]) input.x += 1;

        Position.x += input.x * baseSpeed * deltaTime;
        Position.y += input.y * baseSpeed * deltaTime;

        // 玩家中心点边界限制，防止角色移动出 1920x1080 画面。
        if (Position.x < 30) Position.x = 30;
        if (Position.x > 1890) Position.x = 1890;
        if (Position.y < 30) Position.y = 30;
        if (Position.y > 1050) Position.y = 1050;

        // 更新视觉计时器，用于呼吸和漂浮动画。
        visualTime += deltaTime * 3.0f; // 稍微快一点的呼吸节奏

        // 根据水平移动方向计算目标倾斜角，再平滑插值到当前角度。
        double targetAngle = 0.0;
        // 移动时倾斜角度大一点 (15度)
        if (input.x < 0) targetAngle = -15.0;
        else if (input.x > 0) targetAngle = 15.0;

        // 简单线性插值，避免角色角度突然跳变。
        currentAngle += (targetAngle - currentAngle) * 12.0f * deltaTime;
    }

    void Render(SDL_Renderer* renderer) override
    {
        // 无敌闪烁期间 visible=false 的帧不绘制玩家。
        if (texture && visible) {
            int texW, texH;
            SDL_QueryTexture(texture, NULL, NULL, &texW, &texH);

            // 根据贴图原始宽高比计算基础显示大小。
            float aspect = (float)texW / texH;
            float baseH = 100.0f;
            float baseW = baseH * aspect;

            // 呼吸缩放：scaleFactor 在 0.95 到 1.05 之间波动。
            float scaleFactor = 1.0f + std::sin(visualTime) * 0.05f;

            // 宽度变大时高度变小，形成轻微的 squash/stretch 视觉效果。
            float currentW = baseW * scaleFactor;        // 变宽
            float currentH = baseH * (2.0f - scaleFactor); // 变矮

            // 漂浮偏移与呼吸节奏错开，避免动画显得机械。
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

            // 低速模式下显示真实判定点，贴图大小不代表受击范围。
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
        // 外部命中逻辑会先判断 isInvincible，这里保留一层兜底保护。
        if (!isInvincible) lives--;
    }

    // lives 代表剩余可承受命中次数，因此小于等于 0 即进入失败状态。
    bool Dead_judge() { return lives <= 0; }

    int GetLives() const { return lives; }
    int GetPowerLevel() const { return powerLevel; }
    float GetPowerValue() const { return powerValue; }
    float GetTotalPower() const { return static_cast<float>(powerLevel) + powerValue; }
    int GetBombs() const { return bombs; }
    float GetAttackPoint() const { return attack_point; }
    bool IsInvincible() const { return isInvincible; }
    void SetInvincible(float seconds)
    {
        isInvincible = true;
        invincTimer = seconds;
        flashTimer = 0.1f;
        visible = true;
    }
    bool CanUseBomb() const { return bombs > 0; }
    void ConsumeBomb() { if (bombs > 0) bombs--; }
    void ResetForContinue()
    {
        lives = 3;
        powerLevel = 0;
        powerValue = 0.0f;
        ResetPosition();
    }

    int CollectPowerUp() {
        if (powerLevel >= 4) return 0; // 最高火力为 4.00

        powerValue += 0.05f; // 每个 P 点增加 0.05

        if (powerValue >= 1.0f) {
            powerValue -= 1.0f; // 溢出转入下一级
            powerLevel++;
            return 1; // 拾取并升级
        }
        return 2; // 拾取但未升级
    }
};
