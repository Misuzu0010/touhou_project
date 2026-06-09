#pragma once
#include "Bullet.h"
#include <cmath>
#include <memory>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

constexpr float Pi = 3.14159265358979323846f;

// Boss 弹幕生成工具。
// 该类不保存状态，只根据传入参数向 bullets 容器追加 Bullet 对象。
// bullets 使用 unique_ptr 管理所有权，生成函数只负责创建和配置子弹。
class BulletPattern
{
public:
    // 生成一圈均匀分布的旋转弹幕。
    // offset_angle 用于让每一波圆环产生角度偏移，形成旋转视觉效果。
    void ShootRotatingRing(float now_x, float now_y, float now_v, int bullet_cnt, float offset_angle, std::vector<std::unique_ptr<Bullet>>& bullets, BulletType type = BulletType::VIRUS)
    {
        for (int i = 0; i < bullet_cnt; i++) {
            float angle = (static_cast<float>(i) / bullet_cnt * 2.0f * Pi) + offset_angle;
            float vx = now_v * std::cos(angle);
            float vy = now_v * std::sin(angle);
            // type 决定弹幕贴图切片和判定半径，因此同一算法可复用给不同弹种。
            bullets.push_back(std::make_unique<Bullet>(now_x, now_y, vx, vy, type));
        }
    }

    // 生成短促螺旋弹幕。
    // angle_start 是本次喷射起始角，angle_step 是同一批子弹之间的角度差。
    void ShootSpiral(float now_x, float now_y, float speed, float angle_start, int count, float angle_step, std::vector<std::unique_ptr<Bullet>>& bullets, BulletType type)
    {
        for (int i = 0; i < count; i++) {
            float angle = angle_start + (i * angle_step);
            float vx = speed * std::cos(angle);
            float vy = speed * std::sin(angle);
            bullets.push_back(std::make_unique<Bullet>(now_x, now_y, vx, vy, type));
        }
    }

    // 生成“展开 -> 停顿 -> 瞄准玩家”的弹幕。
    // 典型用途是锁链/陷阱类弹幕：先围绕 Boss 展开，再统一朝玩家当前位置发射。
    void ShootStopAndGo(float now_x, float now_y, float waitTime, float attackSpeed, Player* player, int bullet_cnt, std::vector<std::unique_ptr<Bullet>>& bullets, BulletType type = BulletType::LOCK)
    {
        float targetRadius = 150.0f; // 展开后的圆圈半径
        float unfoldTime = 0.5f;     // 展开阶段持续时间。

        for (int i = 0; i < bullet_cnt; i++)
        {
            float angle = static_cast<float>(i) / bullet_cnt * 2.0f * Pi;

            // 初始生成在 Boss 中心，随后通过 UNFOLDING 状态向外散开。
            auto b = std::make_unique<Bullet>(now_x, now_y, waitTime, attackSpeed, player, type);

            // 展开速度 = 目标半径 / 展开时间，使子弹在 unfoldTime 后到达环形位置。
            float unfoldSpeed = targetRadius / unfoldTime;
            b->StartUnfolding(unfoldTime, unfoldSpeed * std::cos(angle), unfoldSpeed * std::sin(angle));

            bullets.push_back(std::move(b));
        }
    }
};
