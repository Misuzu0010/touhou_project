#include "Bullet.h"
#include "Player.h"
#include <cmath>
#include <SDL_image.h>

// 构造函数保持不变...
Bullet::Bullet(float x, float y, float speed_x, float speed_y, BulletType t)
    : Entity(x, y, 10), state(BulletState::NORMAL), type(t), targetPlayer(nullptr), waitTimer(0), finalSpeed(0)
{
    Velocity.x = speed_x;
    Velocity.y = speed_y;
    // ★判定半径微调：虽然贴图大，但判定点要小
    if (type == BulletType::VIRUS) radius = 15; // 大玉判定稍微大点
    else radius = 8; // 小玉判定小点
}

Bullet::Bullet(float x, float y, float waitTime, float speed, Player* player, BulletType t)
    : Entity(x, y, 10), state(BulletState::WAITING), type(t), waitTimer(waitTime), finalSpeed(speed), targetPlayer(player)
{
    Velocity.x = 0;
    Velocity.y = 0;
    radius = 8;
}

void Bullet::Update(float deltaTime)
{
    // ... (追踪逻辑不变) ...
    if (state == BulletState::WAITING) {
        waitTimer -= deltaTime;
        if (waitTimer <= 0) {
            if (targetPlayer) {
                float dx = targetPlayer->Position.x - Position.x;
                float dy = targetPlayer->Position.y - Position.y;
                float angle = std::atan2(dy, dx);
                Velocity.x = finalSpeed * std::cos(angle);
                Velocity.y = finalSpeed * std::sin(angle);
            }
            state = BulletState::NORMAL;
        }
    }

    Entity::Update(deltaTime);

    // ★边界判定：适配 1920x1080
    // 稍微放宽一点销毁边界，防止子弹刚出屏幕就消失显得突兀
    if (Position.x > 2000 || Position.x < -100 || Position.y > 1150 || Position.y < -100) {
        active = false;
    }
}

void Bullet::Render(SDL_Renderer* renderer, SDL_Texture* texture)
{
    if (!texture) return;

    // 1. 获取原图的巨大尺寸 (Force Read Source Size)
    int texW, texH;
    SDL_QueryTexture(texture, NULL, NULL, &texW, &texH);

    SDL_Rect srcRect; // 切哪里？(基于原图巨大的坐标)
    int displaySize;  // 画多大？(强制缩放后的屏幕像素大小)

    switch (type) {
    case BulletType::PLAYER_TALISMAN:
        // 玩家符札：PlayerBullet .png (5列 x 3行)
        // 逻辑：不管图多大，宽除以5，高除以3
        srcRect.w = texW / 5;
        srcRect.h = texH / 3;
        srcRect.x = srcRect.w * 1; // 第2列
        srcRect.y = srcRect.h * 1; // 第2行

        displaySize = 48; // 屏幕上显示的长边大小
        break;

    case BulletType::PLAYER_STAR:
        // 玩家星星：(5列 x 3行)
        srcRect.w = texW / 5;
        srcRect.h = texH / 3;
        srcRect.x = srcRect.w * 2; // 第3列
        srcRect.y = srcRect.h * 2; // 第3行

        displaySize = 40;
        break;

    case BulletType::VIRUS:
        // 敌人：病毒球 (EnemyBullet.png)
        // 这图有文字，我们只切上半部分 (h * 0.65)
        srcRect.w = texW / 3; // 3列
        srcRect.h = (int)(texH * 0.65);
        srcRect.x = 0;        // 第1列
        srcRect.y = 0;

        displaySize = 64; // 病毒球大一点
        break;

    case BulletType::LOCK:
        // 敌人：锁
        srcRect.w = texW / 3;
        srcRect.h = (int)(texH * 0.65);
        srcRect.x = srcRect.w * 1; // 第2列
        srcRect.y = 0;

        displaySize = 48;
        break;

    case BulletType::SHURIKEN:
        // 敌人：手里剑
        srcRect.w = texW / 3;
        srcRect.h = (int)(texH * 0.65);
        srcRect.x = srcRect.w * 2; // 第3列
        srcRect.y = 0;

        displaySize = 48;
        break;

    default:
        // 默认保底
        srcRect = { 0, 0, texW, texH };
        displaySize = 32;
        break;
    }

    // 2. 强制缩放 (Force Scaling)
    // 根据上面设定的 displaySize 生成目标矩形
    // 简单的正方形判定，如果长宽不一致可以在 case 里细分
    SDL_Rect destRect = {
        (int)(Position.x - displaySize / 2),
        (int)(Position.y - displaySize / 2),
        displaySize,
        displaySize
    };

    // 如果是符札这种长条形的，单独修正一下 destRect
    if (type == BulletType::PLAYER_TALISMAN) {
        destRect.w = 32; // 窄一点
        destRect.h = 48; // 长一点
        destRect.x = (int)(Position.x - destRect.w / 2);
        destRect.y = (int)(Position.y - destRect.h / 2);
    }

    // 计算旋转角度
    double angle = 0.0;
    if (Velocity.x != 0 || Velocity.y != 0) {
        angle = std::atan2(Velocity.y, Velocity.x) * (180.0 / 3.14159) + 90;
    }

    // 渲染！
    SDL_RenderCopyEx(renderer, texture, &srcRect, &destRect, angle, NULL, SDL_FLIP_NONE);
}