#pragma once
#include "Bullet.h"
#include "BulletPattern.h"
#include "Player.h"
#include <cmath>
#include <cstdlib>
#include <memory>
#include <vector>

// ============================================================
// 弹幕策略抽象接口（策略模式）
//
// 每一种 Boss 阶段对应一个具体策略子类，封装该阶段的弹幕生成算法。
// Game / Enemy 只需持有 BulletStrategy 指针并调用 Execute，
// 无需了解弹幕的具体构造方式，实现"算法与使用方解耦"。
// ============================================================
class BulletStrategy {
public:
    virtual ~BulletStrategy() = default;

    // 执行一次弹幕波次。
    // bossX/bossY   — Boss 当前坐标。
    // angleOffset   — 旋转偏移引用，策略内部可累加实现持续旋转。
    // bullets       — 敌弹容器，策略向其中追加新子弹。
    // player        — 玩家指针，追踪弹需要瞄准玩家位置。
    virtual void Execute(float bossX, float bossY, float& angleOffset,
        std::vector<std::unique_ptr<Bullet>>& bullets,
        Player* player) = 0;

    // 返回本阶段相邻两波弹幕之间的冷却时间（秒）。
    virtual float GetShootInterval() const = 0;

    // 返回每次 Execute 后累加到 angleOffset 的角度增量。
    // 正值为顺时针旋转，负值为逆时针。
    virtual float GetAngleIncrement() const = 0;

    // 返回策略名称，用于调试日志。
    virtual const char* GetDescription() const = 0;
};


// ============================================================
// 阶段 0：病毒圆环弹幕
// 生成一圈均匀分布的子弹，每帧旋转 angleOffset 制造旋转视觉效果。
// ============================================================
class RotatingRingStrategy : public BulletStrategy {
private:
    int  bulletCount;   // 每圈的子弹数量
    float bulletSpeed;  // 子弹移动速度（像素/秒）
    BulletType bulletType;
    BulletPattern bp;   // 复用已有弹幕生成工具

public:
    RotatingRingStrategy(int count = 36, float speed = 300,
                         BulletType type = BulletType::VIRUS)
        : bulletCount(count), bulletSpeed(speed), bulletType(type) {}

    void Execute(float bossX, float bossY, float& angleOffset,
        std::vector<std::unique_ptr<Bullet>>& bullets,
        Player* /*player*/) override
    {
        bp.ShootRotatingRing(bossX, bossY, bulletSpeed,
                             bulletCount, angleOffset, bullets, bulletType);
        angleOffset += GetAngleIncrement();
    }

    float GetShootInterval()  const override { return 0.5f; }
    float GetAngleIncrement() const override { return 0.1f; }
    const char* GetDescription() const override { return "RotatingRing (病毒圆环弹幕)"; }
};


// ============================================================
// 阶段 1：锁链追踪弹幕
// 子弹先在 Boss 周围展开成环形，停顿后统一瞄准玩家当前位置射出。
// ============================================================
class StopAndGoStrategy : public BulletStrategy {
private:
    float waitTime;       // 展开后等待时间
    float attackSpeed;    // 瞄准玩家后的飞行速度
    int   bulletCount;    // 每波锁链数量
    BulletType bulletType;
    BulletPattern bp;

public:
    StopAndGoStrategy(float wait = 1.0f, float speed = 600,
                      int count = 24, BulletType type = BulletType::LOCK)
        : waitTime(wait), attackSpeed(speed), bulletCount(count), bulletType(type) {}

    void Execute(float bossX, float bossY, float& angleOffset,
        std::vector<std::unique_ptr<Bullet>>& bullets,
        Player* player) override
    {
        bp.ShootStopAndGo(bossX, bossY, waitTime, attackSpeed,
                          player, bulletCount, bullets, bulletType);
        // 锁链弹每波轻微反向旋转，视觉上比圆环弹更有压迫感。
        angleOffset -= 0.15f;
    }

    float GetShootInterval()  const override { return 0.8f; }
    float GetAngleIncrement() const override { return -0.15f; }
    const char* GetDescription() const override { return "StopAndGo (锁链追踪弹幕)"; }
};


// ============================================================
// 阶段 2：高速手里剑螺旋弹幕
// 高频低间隔发射多枚螺旋弹，弹速快、弹数多，形成密集压制。
// ============================================================
class SpiralStrategy : public BulletStrategy {
private:
    float bulletSpeed;   // 子弹速度
    int   bulletCount;   // 每波子弹数
    float angleStep;     // 同一波内子弹之间的角度差
    BulletType bulletType;
    BulletPattern bp;

public:
    SpiralStrategy(float speed = 500, int count = 5, float step = 0.2f,
                   BulletType type = BulletType::SHURIKEN)
        : bulletSpeed(speed), bulletCount(count), angleStep(step), bulletType(type) {}

    void Execute(float bossX, float bossY, float& angleOffset,
        std::vector<std::unique_ptr<Bullet>>& bullets,
        Player* /*player*/) override
    {
        bp.ShootSpiral(bossX, bossY, bulletSpeed, angleOffset,
                       bulletCount, angleStep, bullets, bulletType);
        angleOffset += 0.5f;
    }

    float GetShootInterval()  const override { return 0.05f; }
    float GetAngleIncrement() const override { return 0.5f; }
    const char* GetDescription() const override { return "Spiral (高速手里剑螺旋)"; }
};


// ============================================================
// 新增：扇形散射弹幕（WideShotStrategy）
// 追踪玩家方向，以玩家角度为中心展开扇形弹幕，模拟"霰弹"效果。
// ============================================================
class WideShotStrategy : public BulletStrategy {
private:
    int   bulletCount;    // 每波子弹数
    float bulletSpeed;    // 弹速
    float spreadAngle;    // 总散射角度（弧度），如 Pi/4 = 45°
    BulletType bulletType;

public:
    WideShotStrategy(int count = 9, float speed = 350,
                     float spread = 0.785f, // Pi/4 ≈ 45°
                     BulletType type = BulletType::VIRUS)
        : bulletCount(count), bulletSpeed(speed),
          spreadAngle(spread), bulletType(type) {}

    void Execute(float bossX, float bossY, float& angleOffset,
        std::vector<std::unique_ptr<Bullet>>& bullets,
        Player* player) override
    {
        // 计算 Boss → 玩家的角度作为散射中心
        float baseAngle = 0.0f;
        if (player) {
            float dx = player->GetPosition().x - bossX;
            float dy = player->GetPosition().y - bossY;
            baseAngle = std::atan2(dy, dx);
        }

        // 在 baseAngle ± spreadAngle/2 范围内均匀分布子弹
        float halfSpread = spreadAngle * 0.5f;
        for (int i = 0; i < bulletCount; ++i) {
            float t = (bulletCount == 1) ? 0.0f
                       : (static_cast<float>(i) / (bulletCount - 1) * 2.0f - 1.0f);
            float angle = baseAngle + t * halfSpread;
            float vx = bulletSpeed * std::cos(angle);
            float vy = bulletSpeed * std::sin(angle);
            bullets.push_back(std::make_unique<Bullet>(
                bossX, bossY, vx, vy, bulletType));
        }

        // 散射弹每波微调角度，让连续波次不重叠在同一位置
        angleOffset += 0.08f;
    }

    float GetShootInterval()  const override { return 0.35f; }
    float GetAngleIncrement() const override { return 0.08f; }
    const char* GetDescription() const override { return "WideShot (扇形追踪散射)"; }
};


// ============================================================
// 新增：十字交叉弹幕（CrossPatternStrategy）
// 每波生成 4 路弹幕，分别向上下左右射出，同时随 angleOffset 旋转。
// 适合用作"封锁走位"的高压弹幕。
// ============================================================
class CrossPatternStrategy : public BulletStrategy {
private:
    float bulletSpeed;
    int   bulletsPerArm;  // 每臂子弹数
    BulletType bulletType;

public:
    CrossPatternStrategy(float speed = 280, int perArm = 4,
                         BulletType type = BulletType::SHURIKEN)
        : bulletSpeed(speed), bulletsPerArm(perArm), bulletType(type) {}

    void Execute(float bossX, float bossY, float& angleOffset,
        std::vector<std::unique_ptr<Bullet>>& bullets,
        Player* /*player*/) override
    {
        // 4 个主方向 + 旋转偏移
        const float baseAngles[4] = {
            0.0f,                     // 右
            Pi * 0.5f,                // 下
            Pi,                       // 左
            Pi * 1.5f                 // 上
        };

        for (int arm = 0; arm < 4; ++arm) {
            float centerAngle = baseAngles[arm] + angleOffset;
            float halfSpread = 0.15f; // 臂内微散射

            for (int i = 0; i < bulletsPerArm; ++i) {
                float t = (bulletsPerArm == 1) ? 0.0f
                          : (static_cast<float>(i) / (bulletsPerArm - 1) * 2.0f - 1.0f);
                float angle = centerAngle + t * halfSpread;
                float vx = bulletSpeed * std::cos(angle);
                float vy = bulletSpeed * std::sin(angle);
                bullets.push_back(std::make_unique<Bullet>(
                    bossX, bossY, vx, vy, bulletType));
            }
        }

        angleOffset += 0.12f; // 十字缓慢旋转
    }

    float GetShootInterval()  const override { return 0.25f; }
    float GetAngleIncrement() const override { return 0.12f; }
    const char* GetDescription() const override { return "CrossPattern (十字交叉弹幕)"; }
};


// ============================================================
// BurstStrategy —— 全向爆裂弹幕
//
// 每波发射大量子弹（默认48颗）覆盖全方向，分内外两层：
//   - 外层高速（fastSpeed），先到达玩家区域形成外围封锁
//   - 内层低速（slowSpeed），随后到达形成二段压迫
// angleOffset 每次大跳（Pi/3），让连续波次方向不可预测。
// 冷却较长（1.0s），适合用作"蓄力爆发"型阶段弹幕。
// ============================================================
class BurstStrategy : public BulletStrategy {
private:
    int   bulletCount;
    float fastSpeed;
    float slowSpeed;
    BulletType bulletType;
    float phaseAccum;   // 用于生成不可预测的角度跳跃

public:
    BurstStrategy(int count = 48, float fast = 400, float slow = 200,
                  BulletType type = BulletType::SHURIKEN)
        : bulletCount(count), fastSpeed(fast), slowSpeed(slow),
          bulletType(type), phaseAccum(0.0f) {}

    void Execute(float bossX, float bossY, float& angleOffset,
        std::vector<std::unique_ptr<Bullet>>& bullets,
        Player* /*player*/) override
    {
        int half = bulletCount / 2;

        // 外层高速弹 + 内层低速弹，形成双层扩张圆环
        for (int i = 0; i < bulletCount; ++i) {
            float angle = static_cast<float>(i) / bulletCount * 2.0f * Pi + angleOffset;
            float speed = (i < half) ? fastSpeed : slowSpeed;
            float vx = speed * std::cos(angle);
            float vy = speed * std::sin(angle);
            bullets.push_back(std::make_unique<Bullet>(
                bossX, bossY, vx, vy, bulletType));
        }

        // 每次爆发后角度大幅跳跃（~Pi/3 的整数倍偏移），
        // 使得连续爆发不会覆盖相同方向
        phaseAccum += Pi / 3.0f;
        angleOffset += (std::sin(phaseAccum * 1.7f) > 0 ? Pi / 3.0f : -Pi / 4.0f);
    }

    float GetShootInterval()  const override { return 1.0f; }
    float GetAngleIncrement() const override { return Pi / 3.0f; }
    const char* GetDescription() const override { return "Burst (全向爆裂弹幕)"; }
};


// ============================================================
// RainStrategy —— 弹幕雨
//
// 在 Boss 下方生成随机横向偏移的子弹，全部垂直下落。
// 每颗子弹的速度微调（±20%），让弹雨有前有后形成层次感。
// 高频率（0.18s 一波）+ 宽散布 → 形成持续的"弹幕雨"压制。
// ============================================================
class RainStrategy : public BulletStrategy {
private:
    int   bulletsPerWave;
    float bulletSpeed;
    float spreadWidth;
    BulletType bulletType;

public:
    RainStrategy(int count = 12, float speed = 350, float width = 400,
                 BulletType type = BulletType::VIRUS)
        : bulletsPerWave(count), bulletSpeed(speed),
          spreadWidth(width), bulletType(type) {}

    void Execute(float bossX, float bossY, float& angleOffset,
        std::vector<std::unique_ptr<Bullet>>& bullets,
        Player* /*player*/) override
    {
        for (int i = 0; i < bulletsPerWave; ++i) {
            // 在 [bossX - spreadWidth/2, bossX + spreadWidth/2] 随机分布
            float offsetX = (static_cast<float>(std::rand()) / RAND_MAX - 0.5f) * spreadWidth;
            // 速度 ±20% 随机变化，制造前-中-后三层弹雨
            float speedVar = bulletSpeed * (0.8f + static_cast<float>(std::rand()) / RAND_MAX * 0.4f);
            bullets.push_back(std::make_unique<Bullet>(
                bossX + offsetX, bossY, 0.0f, speedVar, bulletType));
        }
        angleOffset += 0.04f;
    }

    float GetShootInterval()  const override { return 0.18f; }
    float GetAngleIncrement() const override { return 0.04f; }
    const char* GetDescription() const override { return "Rain (弹幕雨)"; }
};
