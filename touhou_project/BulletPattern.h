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

    // 以前的 StopAndGo 也可以保留，稍微改造一下支持类型
    void ShootStopAndGo(float now_x, float now_y, float waitTime, float speed, Player* player, int bullet_cnt, std::vector<Bullet*>& bullets, BulletType type = BulletType::LOCK)
    {
        for (int i = 0; i < bullet_cnt; i++)
        {
            float angle = (float)i / bullet_cnt * 2 * M_PI;
            float dist = 60.0f;
            float spawn_x = now_x + dist * std::cos(angle);
            float spawn_y = now_y + dist * std::sin(angle);

            bullets.push_back(new Bullet(spawn_x, spawn_y, waitTime, speed, player, type));
        }
    }
};