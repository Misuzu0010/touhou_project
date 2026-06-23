#pragma execution_character_set("utf-8")
#include "Item.h"
#include <cmath>

// ----------------------------------------------------------------
// 构造函数：设定初始位置、贴图和碰撞半径
// 初始速度模拟上抛效果（与 PowerUp 一致），并添加随机横向漂移
// ----------------------------------------------------------------
Item::Item(float x, float y, SDL_Texture* texture, float r)
    : Entity(x, y, r), texture(texture), gravity(GRAVITY)
{
    active = true;

    // 初始给一个向上的速度，让道具出现时先弹起再下落（与 PowerUp 一致）
    Velocity.y = -400.0f;

    // 横向随机漂移，避免同一批道具完全重叠
    float randomX = (float)(rand() % 300 - 150);
    Velocity.x = randomX;
}

// ----------------------------------------------------------------
// Update：道具受重力下落，左右边界反弹，超出底部标记失效
// ----------------------------------------------------------------
void Item::Update(float deltaTime)
{
    // 持续施加重力，让道具从弹起状态逐渐转为下落
    Velocity.y += gravity * deltaTime;

    // 限制最大下落速度
    if (Velocity.y > MAX_FALL_SPEED) Velocity.y = MAX_FALL_SPEED;

    // 复用 Entity 的基础位移逻辑
    Entity::Update(deltaTime);

    // 到达屏幕左右边缘时反弹，保证道具不会飘出可拾取区域
    if (Position.x < 10) {
        Position.x = 10;
        Velocity.x = -Velocity.x;
    }
    else if (Position.x > 1910) {
        Position.x = 1910;
        Velocity.x = -Velocity.x;
    }

    // 掉出屏幕底部一段缓冲距离后标记失效，随后由 Game 清理
    if (Position.y > 1150.0f) {
        active = false;
    }

    // 推进光晕动画计时器
    animTimer_ += deltaTime;
}

// ----------------------------------------------------------------
// Render：绘制道具，并叠加一圈随时间缩放的半透明光晕
// ----------------------------------------------------------------
void Item::Render(SDL_Renderer* renderer)
{
    if (!texture) return;

    // 绘制道具主体（复用 PlayerBullet 图集的第一行第一列，与 PowerUp 一致）
    int w, h;
    SDL_QueryTexture(texture, NULL, NULL, &w, &h);

    SDL_Rect srcRect;
    srcRect.w = w / 5;
    srcRect.h = h / 3;
    srcRect.x = 0;
    srcRect.y = 0;

    // 显示大小固定为 40x40
    SDL_Rect destRect = {
        (int)(Position.x - 20),
        (int)(Position.y - 20),
        40, 40
    };

    SDL_RenderCopy(renderer, texture, &srcRect, &destRect);

    // 呼吸光晕：半透明白圈，随 sin 波动大小
    float pulse = 1.0f + 0.15f * std::sin(animTimer_ * 4.0f);
    int glowR = (int)(40 * pulse);
    int glowH = (int)(40 * pulse);
    SDL_Rect glowDst = {
        (int)Position.x - glowR / 2,
        (int)Position.y - glowH / 2,
        glowR, glowH
    };
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 40);
    SDL_RenderDrawRect(renderer, &glowDst);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}
