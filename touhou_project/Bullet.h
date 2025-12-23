#pragma once
#include "Entity.h"
#include <SDL.h>

//对子弹类进行封装
class Bullet :public Entity 
{
public:
	//子弹的颜色
	SDL_Color color;
	//子弹存活时间
	float time_alive = 0.0f;
	//初始化构造这个子弹
	Bullet(float x, float y, float speed_x, float speed_y) :Entity(x, y, 3), time_alive(0)
	{
		Velocity.x = speed_x;
		Velocity.y = speed_y;
		color = RandomColor();
		//对每一个子弹的颜色都进行随机化处理
		//并且是在随着时间变化 直到出屏幕或者是与角色碰撞 停止他的变化
		//再对他进行清理？
		//设定屏幕中子弹的上限值 超过这个值的时候就不再产生新的子弹
	}

	//随机颜色
	SDL_Color RandomColor() 
	{
		return SDL_Color
		{ 
		Uint8(rand() % 256),
		Uint8(rand() % 256),
		Uint8(rand() % 256),
		255 };
	}

	//更新这个子弹的状态
	void Update(float deltaTime) override 
	{
		Entity::Update(deltaTime);
		time_alive += deltaTime;
		//随着时间的推移 让子弹的颜色发生变化
		if (time_alive > 0.1f) {
			color = RandomColor();
			time_alive = 0.0f;
		}

		//检测是否出界
		if (Position.x > 640 || Position.x < 0 ||
			Position.y > 480 || Position.y< 0) {
			active = false;
		}
	}

	//渲染子弹
	void Render(SDL_Renderer* renderer) 
	{
		//设置子弹颜色 通过SDL的渲染器来设置颜色，传入这几个参数
		//这些参数会在前面的SDL_Color color;中进行随机化
		//从而达到子弹颜色变化的效果
		SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);

		SDL_Rect rect =
		{
			(int)(Position.x - radius),
			(int)(Position.y - radius),
			(int)(radius * 2),
			(int)(radius * 2)

		};

		SDL_RenderFillRect(renderer, &rect);
		// 画实心矩形（子弹显示出来）
	}
};