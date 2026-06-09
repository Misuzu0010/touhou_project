#pragma once
#include "Entity.h"
#include <SDL.h>
#include <cstdlib> // 用于随机数

// P 点道具实体。
// 生成后会先向上弹起，再受重力下落；玩家拾取后提升火力。
class PowerUp : public Entity
{
private:
    SDL_Texture* texture; // 非拥有指针：当前复用玩家子弹图集，由 Game 统一释放。
    float gravity;        // 下落重力加速度，单位为像素/秒^2。

public:
    PowerUp(float x, float y, SDL_Texture* tex)
        : Entity(x, y, 20), texture(tex), gravity(800.0f) // 半径略大，方便拾取。
    {
        active = true;

        // 初始给一个向上的速度，让 P 点出现时先弹起再下落。
        Velocity.y = -400.0f;

        // 横向随机漂移，避免同一批 P 点完全重叠。
        float randomX = (float)(rand() % 300 - 150);
        Velocity.x = randomX;
    }

    void Update(float deltaTime) override
    {
        // 持续施加重力，让道具从弹起状态逐渐转为下落。
        Velocity.y += gravity * deltaTime;

        // 限制最大下落速度，避免玩家几乎没有反应时间。
        if (Velocity.y > 300.0f) Velocity.y = 300.0f;

        // 复用 Entity 的基础位移逻辑。
        Entity::Update(deltaTime);

        // 到达屏幕左右边缘时反弹，保证道具不会飘出可拾取区域。
        if (Position.x < 10) {
            Position.x = 10;
            Velocity.x = -Velocity.x; // 反弹
        }
        else if (Position.x > 1910) {
            Position.x = 1910;
            Velocity.x = -Velocity.x; // 反弹
        }

        // 掉出屏幕底部一段缓冲距离后标记失效，随后由 Game 清理。
        if (Position.y > 1150.0f) {
            active = false;
        }
    }

    void Render(SDL_Renderer* renderer)
    {
        if (!texture) return;

        // P 点使用 PlayerBullet.png 图集第一行第一列。
        int w, h;
        SDL_QueryTexture(texture, NULL, NULL, &w, &h);

        SDL_Rect srcRect;
        srcRect.w = w / 5;
        srcRect.h = h / 3;
        srcRect.x = 0;
        srcRect.y = 0;

        // 显示大小固定为 40x40，拾取判定由 Game 中的矩形范围控制。
        SDL_Rect destRect = {
            (int)(Position.x - 20),
            (int)(Position.y - 20),
            40, 40
        };

        SDL_RenderCopy(renderer, texture, &srcRect, &destRect);
    }
};
