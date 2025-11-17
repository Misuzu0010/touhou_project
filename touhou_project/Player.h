#pragma once
#include "Entity.h"
#include "BulletPattern.h"
#include <SDL.h>

class Player : public Entity
{
public:
	//基本属性
	float Blood;
	//攻击力	
	float attack_point;

	Player(float x, float y) :Entity(x, y, 3), Blood(100.0f), attack_point(5.0f) { active=true; }

	//计时器相关变量
	Uint32 lastTime;
	float shootTimer;
	
	void Update(float deltaTime) override 
	{
		Velocity.x = 0;
		Velocity.y = 0;
		//每次更新的时候 重置速度
		float speed = 100.0f;
		const Uint8* keyStates = SDL_GetKeyboardState(NULL);
		//输入逻辑
		if (keyStates[SDL_SCANCODE_W]) 
		{
			Velocity.y = -speed;
		}
		if (keyStates[SDL_SCANCODE_S]) 
		{
			Velocity.y = speed;
		}
		if (keyStates[SDL_SCANCODE_A]) 
		{
			Velocity.x = -speed;
		}
		if (keyStates[SDL_SCANCODE_D]) 
		{
			Velocity.x = speed;
		}
		if (keyStates[SDL_SCANCODE_UP])
		{
			Velocity.y = -speed;
		}
		if (keyStates[SDL_SCANCODE_DOWN])
		{
			Velocity.y = speed;
		}
		if (keyStates[SDL_SCANCODE_LEFT])
		{
			Velocity.x = -speed;
		}
		if (keyStates[SDL_SCANCODE_RIGHT])
		{
			Velocity.x = speed;
		}
		//调用父类的更新函数
		Entity::Update(deltaTime);
		//限制玩家在屏幕内移动
		if (Position.x < 0) Position.x = 0;
		if (Position.x > 640) Position.x = 640;
		if (Position.y < 0) Position.y = 0;
		if (Position.y > 480) Position.y = 480;

	}

	void hit(float damage) 
	{
		Blood -= damage;
	}
	bool Dead_judge() 
	{
		return Blood <= 0.0f;
	}

	float get_hp()
	{
		return Blood;
	}

	
	
};