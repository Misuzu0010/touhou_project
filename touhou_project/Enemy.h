#pragma once
#include "Entity.h"
#include "BulletPattern.h"
#include "Game.h"
#include <SDL.h>
class Enemy : public Entity
{
public:
	float Blood;
	float attack_point;
	BulletPattern bp;

	//计时器相关变量
	Uint32 lastTime;
	float shootTimer;
	Enemy(float x, float y) :Entity(x, y, 100), Blood(6000.0f),attack_point(1.0f) {
		active = true;
	}

	//所以 更新只需要写血的变化 攻击状态的变化就行
	//0-2000 第一阶段攻击
	//环形
	//2000-4000 第二阶段攻击
	//扇形
	//4000-6000 第三阶段攻击
	//螺旋
	
	

	void hit(float attack) 
	{
		Blood -= attack;
	}
	float GetBlood() {
		return Blood;
	}

	

	
};