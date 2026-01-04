#pragma once
#include "Bullet.h"
#include <vector>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class BulletPattern
{
public:
    // ★修改1：增加了 type 参数，默认是 VIRUS
    // 这是一个“通用环形弹幕”，可以通过 bullet_cnt 控制密度
    void ShootRotatingRing(float now_x, float now_y, float now_v, int bullet_cnt, float offset_angle, std::vector<Bullet*>& bullets, BulletType type = BulletType::VIRUS)
    {
        for (int i = 0; i < bullet_cnt; i++) {
            float angle = ((float)i / bullet_cnt * 2 * M_PI) + offset_angle;
            float vx = now_v * std::cos(angle);
            float vy = now_v * std::sin(angle);
            // 这里使用传入的 type，而不是写死
            bullets.push_back(new Bullet(now_x, now_y, vx, vy, type));
        }
    }

    // ★新增：螺旋弹幕 (高密度压制用)
    // 这种弹幕不是一次发一圈，而是像水管浇水一样快速旋转喷射
    void ShootSpiral(float now_x, float now_y, float speed, float angle_start, int count, float angle_step, std::vector<Bullet*>& bullets, BulletType type)
    {
        for (int i = 0; i < count; i++) {
            float angle = angle_start + (i * angle_step); // 角度递增
            float vx = speed * std::cos(angle);
            float vy = speed * std::sin(angle);
            bullets.push_back(new Bullet(now_x, now_y, vx, vy, type));
        }
    }

    void ShootStopAndGo(float now_x, float now_y, float waitTime, float attackSpeed, Player* player, int bullet_cnt, std::vector<Bullet*>& bullets, BulletType type = BulletType::LOCK)
    {
        float targetRadius = 150.0f; // 展开后的圆圈半径
        float unfoldTime = 0.5f;     // 展开过程持续多久 (0.5秒)

        for (int i = 0; i < bullet_cnt; i++)
        {
            float angle = (float)i / bullet_cnt * 2 * M_PI;

            // 1. 生成在 Boss 中心 (堆叠状态)
            // 这里的 waitTime 是展开后停顿的时间
            Bullet* b = new Bullet(now_x, now_y, waitTime, attackSpeed, player, type);

            // 2. 强制设为展开状态
            b->state = BulletState::UNFOLDING;
            b->unfoldTimer = unfoldTime;

            // 3. 计算展开速度
            // 速度 = 距离 / 时间
            // 这样 0.5秒 后，它正好飞到 targetRadius 的位置
            float unfoldSpeed = targetRadius / unfoldTime;
            b->Velocity.x = unfoldSpeed * std::cos(angle);
            b->Velocity.y = unfoldSpeed * std::sin(angle);

            bullets.push_back(b);
        }
    }
};