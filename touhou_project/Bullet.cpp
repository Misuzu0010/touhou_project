#include "Bullet.h"
#include "Player.h"
#include <cmath>
#include <SDL_image.h>

// 普通直线子弹：创建后立即按给定速度移动。
Bullet::Bullet(float x, float y, float speed_x, float speed_y, BulletType t)
    : Entity(x, y, 10), state(BulletState::NORMAL), type(t), targetPlayer(nullptr), waitTimer(0), finalSpeed(0)
{
    Velocity.x = speed_x;
    Velocity.y = speed_y;
    // 根据弹种调整判定半径；贴图尺寸通常大于实际判定范围。
    if (type == BulletType::VIRUS) radius = 15; // 大型敌弹使用较大的圆形判定。
    else radius = 8; // 其他子弹保留较小判定，方便玩家擦弹式移动。
}

// 延迟瞄准子弹：先等待指定时间，再朝玩家当前位置发射。
Bullet::Bullet(float x, float y, float waitTime, float speed, Player* player, BulletType t)
    : Entity(x, y, 10), state(BulletState::WAITING), type(t), waitTimer(waitTime), finalSpeed(speed), targetPlayer(player)
{
    Velocity.x = 0;
    Velocity.y = 0;
    radius = 8;
}

void Bullet::Update(float deltaTime)
{
    // UNFOLDING 用于“先展开再停顿”的弹幕，结束后转入 WAITING。
    if (state == BulletState::UNFOLDING) 
    {
        unfoldTimer -= deltaTime;

        if (unfoldTimer <= 0) {
            Velocity.x = 0;
            Velocity.y = 0;
            state = BulletState::WAITING;
        }
    }
    else if (state == BulletState::WAITING) 
    {
        // WAITING 倒计时结束时，根据玩家当前坐标计算最终速度。
        waitTimer -= deltaTime;
        if (waitTimer <= 0) {
            if (targetPlayer) {
                const Vector2D& targetPosition = targetPlayer->GetPosition();
                float dx = targetPosition.x - Position.x;
                float dy = targetPosition.y - Position.y;
                float angle = std::atan2(dy, dx);
                Velocity.x = finalSpeed * std::cos(angle);
                Velocity.y = finalSpeed * std::sin(angle);
            }
            state = BulletState::NORMAL;
        }
    }

    Entity::Update(deltaTime);

    // 子弹离开屏幕外缓冲区后标记为失效，随后由 Game 统一清理。
    if (Position.x > 2000 || Position.x < -100 || Position.y > 1150 || Position.y < -100) 
    {
        Deactivate();
    }
}

void Bullet::Render(SDL_Renderer* renderer, SDL_Texture* texture)
{
    if (!texture) return;

    // 读取图集原始尺寸，用于计算不同弹种的源矩形。
    int texW, texH;
    SDL_QueryTexture(texture, NULL, NULL, &texW, &texH);

    SDL_Rect srcRect; // 图集裁剪区域。
    int displaySize;  // 屏幕上的基础显示尺寸。

    switch (type) {
    case BulletType::PLAYER_TALISMAN:
        // 玩家符札位于 PlayerBullet 图集的 5 列 x 3 行网格中。
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
        // 敌方图集上半部分是实际弹体，下半部分为素材附带文字。
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
        // 未识别弹种时使用整张贴图，保证调试时仍有可见输出。
        srcRect = { 0, 0, texW, texH };
        displaySize = 32;
        break;
    }

    // 根据弹种显示尺寸生成目标矩形；多数弹体按正方形渲染。
    SDL_Rect destRect = {
        (int)(Position.x - displaySize / 2),
        (int)(Position.y - displaySize / 2),
        displaySize,
        displaySize
    };

    // 符札是纵向长条，单独修正宽高以匹配视觉外形。
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

    // 按速度方向旋转绘制，使弹体朝向移动方向。
    SDL_RenderCopyEx(renderer, texture, &srcRect, &destRect, angle, NULL, SDL_FLIP_NONE);
}
