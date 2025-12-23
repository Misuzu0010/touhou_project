#pragma once

#include "Bullet.h"
#include <vector>
#include <cmath>
#include<iostream>

//弹幕分为环形 扇形 螺旋形 三种基本类型
//直线型可以通过扇形进行变形
//第一部分 环形弹幕
//先想好一个环形需要多少子弹来形成 
//考虑到碰撞体积 我需要保证子弹跑到玩家之前有一定的间隔进行躲避
//即以敌人为中心 向周围周期性发射环形子弹
//每隔一段时间发射一次
//同时 考虑到连贯性 不存在说发射到一定数量就停止发射的情况
//用于存储每一个子弹的指针
//函数实现部分

//环形弹幕
class BulletPattern
{
public:
	void ShootRing(float now_x, float now_y, float now_v, int bullet_cnt, std::vector<Bullet*>& Enemy_bullets)
	{
		for (int i = 0; i <= bullet_cnt; i++)
		{
			float angle = (float)i / bullet_cnt * 2 * M_PI;
			float speed_x = now_v * cos(angle);
			float speed_y = now_v * sin(angle);
			Bullet* new_bullet = new Bullet(now_x, now_y, speed_x, speed_y);
			Enemy_bullets.push_back(new_bullet);
		}
	}

	//第二部分 扇形弹幕
	void ShootSector(float now_x, float now_y, float now_v, std::vector<Bullet*>& Enemy_bullets)
	{
		int bullet_cnt = 45;
		for (int i = 0; i < bullet_cnt; i++) {
			float n_angle = (float)i / bullet_cnt * M_PI;
			float speed_x = now_v * cos(n_angle);
			float speed_y = now_v * sin(n_angle);
			Bullet* new_bullet = new Bullet(now_x, now_y, speed_x, speed_y);
			Enemy_bullets.push_back(new_bullet);
		}
	}

	void ShootSector_2(float now_x, float now_y, float now_v, std::vector<Bullet*>& Enemy_bullets)
	{
		int bullet_cnt = 60;
		for (int i = 0; i < bullet_cnt; i++) {
			float n_angle = (float)i / bullet_cnt * M_PI;
			float speed_x = now_v * cos(n_angle);
			float speed_y = now_v * sin(n_angle);
			Bullet* new_bullet = new Bullet(now_x, now_y, speed_x, speed_y);
			Enemy_bullets.push_back(new_bullet);
		}
	}

	//boss用 螺旋发射子弹
	void ShootSpiral(float now_x, float now_y, float now_v, float n_angle, int bullet_cnt, std::vector<Bullet*>& Enemy_bullets)
	{
		for (int i = 0; i < bullet_cnt; i++) {
			float upd_angle = (float)n_angle + i * 2 * M_PI / 60;
			float speed_x = now_v * cos(upd_angle);
			float speed_y = now_v * sin(upd_angle);
			Bullet* new_bullet = new Bullet(now_x, now_y, speed_x, speed_y);
			Enemy_bullets.push_back(new_bullet);
		}
	}

	//player用 直线发射子弹
	void ShootStraight(float now_x, float now_y, float now_v, std::vector<Bullet*>& player_bullets)
	{
		float speed_x = 0;
		float speed_y = -now_v;
		Bullet* new_bullet = new Bullet(now_x, now_y, speed_x, speed_y);
		player_bullets.push_back(new_bullet);
		//std::cout << "ShootStraight: pushing bullet at (" << now_x << "," << now_y << ")\n";
	}
};