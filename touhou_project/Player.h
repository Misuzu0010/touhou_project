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
        // 1. 归零速度
        Velocity.x = 0;
        Velocity.y = 0;

        // 2. 调整基础速度（建议改快点！）
        float speed = 300.0f;

        const Uint8* keyStates = SDL_GetKeyboardState(NULL);

        // 3. Y轴逻辑 (W/上 减坐标，S/下 加坐标)
        float dirY = 0.0f;
        if (keyStates[SDL_SCANCODE_W] || keyStates[SDL_SCANCODE_UP])    dirY -= 1.0f;
        if (keyStates[SDL_SCANCODE_S] || keyStates[SDL_SCANCODE_DOWN])  dirY += 1.0f;

        // 4. X轴逻辑 (A/左 减坐标，D/右 加坐标)
        float dirX = 0.0f;
        if (keyStates[SDL_SCANCODE_A] || keyStates[SDL_SCANCODE_LEFT])  dirX -= 1.0f;
        if (keyStates[SDL_SCANCODE_D] || keyStates[SDL_SCANCODE_RIGHT]) dirX += 1.0f;

        // 5. 应用速度 (简单的防斜向加速归一化可以以后做，先保证基础移动)
        Velocity.x = dirX * speed;
        Velocity.y = dirY * speed;

        // 6. 调用父类更新位置
        Entity::Update(deltaTime);

        // 7. 边界限制 (保持你原来的代码)
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