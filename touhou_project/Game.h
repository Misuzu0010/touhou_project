#pragma once
#include<SDL.h>
#include <vector>
#include "Player.h"
#include "Enemy.h"
#include "Bullet.h"
#include"BulletPattern.h"

class Game 
{
public:
	//检测是否正常载入
	bool Init();

	//运行游戏主循环
	void Run();

	//清理资源
	void Clean();

	//处理遇到的各种事件
	void HandleEvents();

	//更新人物 弹幕的位置
	void Update(float DeltaTime);

	//渲染当前游戏画面
	void Render();

	//指向游戏窗口的指针，负责管理游戏窗口的创建、显示和销毁
	SDL_Window* cur_Window;

	//指向渲染器的指针，负责管理游戏图形的渲染和显示
	SDL_Renderer* cur_Renderer;

	//游戏是否在运行的标志
	bool is_Running;

	//玩家指针
	Player* player;
	
	//敌人指针
	std::vector<Enemy*>Enemies;

	//子弹存储
	std::vector<Bullet*> playerBullets;  // 不限数量
	std::vector<Bullet*> enemyBullets;   // 最多限750

	Uint32 lastTime;
	float shootTimer;       // 玩家用
	float enemyShootTimer;  // 敌人用
	float spiralAngle;

};
