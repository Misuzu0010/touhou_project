#pragma once
#include "Entity.h"
#include <SDL.h>

// Boss 敌人实体。
// 当前项目每局主要使用一个 Boss，血量阶段和弹幕行为由 Game 统一调度。
class Enemy : public Entity
{
private:
    float Blood;            // 当前血量，阶段切换和胜利判定都依赖该值。
    float attack_point = 5; // 预留攻击力字段，当前敌弹伤害逻辑未直接使用。
    SDL_Texture* texture;   // 非拥有指针：Boss 贴图由 Game 加载和释放。

public:
    Enemy(float x, float y, SDL_Texture* tex)
        : Entity(x, y, 40), Blood(6000), texture(tex) {
        active = true;
    }

    ~Enemy() {
        // texture 由 Game 统一管理生命周期，Enemy 只保存非拥有指针。
    }

    void Render(SDL_Renderer* r) override {
        if (texture) {
            // Boss 贴图统一按高度缩放，避免不同素材原始尺寸影响屏幕占比。
            int w, h;
            SDL_QueryTexture(texture, NULL, NULL, &w, &h);

            // Boss 显示高度固定为 250 像素，宽度按原图比例计算。
            float aspect = (float)w / h;
            int displayH = 250;
            int displayW = (int)(displayH * aspect);

            SDL_Rect dest = {
                (int)Position.x - displayW / 2,
                (int)Position.y - displayH / 2,
                displayW,
                displayH
            };
            SDL_RenderCopy(r, texture, NULL, &dest);
        }
        else {
            // 贴图加载失败时使用红色方块兜底，方便定位资源问题。
            SDL_SetRenderDrawColor(r, 255, 0, 0, 255);
            SDL_Rect rect = { (int)Position.x - 50, (int)Position.y - 50, 100, 100 };
            SDL_RenderFillRect(r, &rect);
        }
    }
    

    // 被玩家子弹或符卡命中时扣血。
    void hit(float attack) { Blood -= attack; }

    // 返回当前血量，用于阶段切换、血条显示和胜利判定。
    float GetBlood() const { return Blood; }
    bool IsAlive() const { return Blood > 0.0f; }
};
