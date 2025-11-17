#pragma once
#include <cmath>
//包含数学库 用于计算距离
class Vector2D
{
public:
	//坐标
	float x, y;

	//子弹和人物 boss的方位赋值
	Vector2D(float x = 0, float y = 0) : x(x), y(y) {}

};

//所有游戏物件的基类
class Entity
{
public:
	//记录物件的位置
	Vector2D Position;

	//记录物件的速度
	Vector2D Velocity;

	//碰撞半径
	float radius;

	//物件是否存活
	bool active;

	//调用构造函数
	Entity(float x, float y, float r) 
        : Position(x,y), Velocity(0,0), radius(r), active(true) {}
	//子类重写update函数
	virtual void Update(float cut_DeltaTime)
	{
		Position.x += Velocity.x * cut_DeltaTime;
		Position.y += Velocity.y * cut_DeltaTime;
		
	}

	//检测是否发生碰撞
	bool CheckCollision(Entity * Other)
	{
		float distX = Position.x - Other->Position.x;
		float distY = Position.y - Other->Position.y;
		//计算距离
		float distance = sqrt((distX * distX) + (distY * distY));
		//检测碰撞
		return distance < (radius + Other->radius);

	}
	

};