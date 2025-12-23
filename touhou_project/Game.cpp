#include "Game.h"
#include <iostream>
BulletPattern bp;
bool Game::Init()
{
    //SDL_Init(SDL_INIT_VIDEO);
   
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        std::cout << "SDL 初始化失败: " << SDL_GetError() << std::endl;
        return false;
    }
    SDL_Window* window = SDL_CreateWindow(u8"東方幻想乡", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, 0);
    cur_Window = window;
    cur_Renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
   
   
    if (!cur_Window)
    {
        std::cout << "窗口创建失败: " << SDL_GetError() << std::endl;
        return false;
    }


    if (!cur_Renderer)
    {
        std::cout << "渲染器创建失败: " << SDL_GetError() << std::endl;
        return false;
    }
    is_Running = true;
    //初始化玩家da
    player = new Player(240.0f, 400.0f);
    //初始化敌人
    Enemy*enemy = new Enemy(240.0f, 100.0f);
    
    Enemies.push_back(enemy);
    lastTime = SDL_GetTicks();
    shootTimer = 0.0f;
    enemyShootTimer = 0.0f;  // 加这行
    spiralAngle = 0.0f;

    CurrentState = State::START_MENU;
    BootProgress = 0.0f;
    BootTimer = 0.0f;
    std::vector<DialogueLine>Phase1
    {

        {},{},{},{},{}
    };
    StartDialogue(Phase1);


    return true;
}

void Game::Run()
{
    while (is_Running)
    {
        Uint32 currentTime = SDL_GetTicks();
        double deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        HandleEvents();

        Update(deltaTime);

        Render();

        SDL_Delay(16);

    }

}

void Game::HandleEvents()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
      /*  std::cout << "event type = " << event.type << std::endl;
        if (event.type == SDL_KEYDOWN) {
            std::cout << "KEYDOWN sym=" << event.key.keysym.sym
                << " scancode=" << event.key.keysym.scancode << std::endl;
        }*/
        if (event.type == SDL_QUIT) {
            is_Running = false;
        }

    }
}

//每一个 deltaTime 内所需要更新的所有内容
void Game::Update(float DeltaTime) 
{
    if (CurrentState == State::DIALOGUE) 
    {
		const Uint8* KeyStates = SDL_GetKeyboardState(NULL);
    }
    //先更新玩家的 update
    player->Update(DeltaTime);

    //检测玩家的操作
	//调用玩家攻击方式
	const Uint8* keyStates = SDL_GetKeyboardState(NULL);
    if (keyStates[SDL_SCANCODE_Z]) 
    {
        //std::cout << "press Z" << std::endl;
        if (shootTimer <= 0.0f) {

			bp.ShootStraight(player->Position.x, player->Position.y, 500.0f, playerBullets);
			shootTimer = 0.01f; // 设置射击间隔为0.1秒
        }
        
    }
    shootTimer -= DeltaTime;  //移到if外面，一直减
    if (shootTimer < 0.0f) shootTimer = 0.0f;  // 防止负数
	//更新敌人的射击
    enemyShootTimer -= DeltaTime;
    for (auto& enemy : Enemies) 
	{
        
        if (enemy->Blood > 4000.0f) {
            //第一阶段 环形弹幕+扇形
            if (enemyShootTimer<= 0.0f) {
                bp.ShootRing(enemy->Position.x, enemy->Position.y, 100.0f, 90, enemyBullets);
                bp.ShootSector(enemy->Position.x, enemy->Position.y, 75.0f, enemyBullets);
                enemyShootTimer = 0.2f; // 设置射击间隔为0.2秒
                enemyShootTimer = 0.2f; // 设置射击间隔为0.2秒
               
            }
        }
        else if (enemy->Blood > 2000.0f) {
            //第二阶段 扇形弹幕 
            if (enemyShootTimer  <= 0.0f) {
                bp.ShootSector(enemy->Position.x, enemy->Position.y, 200.0f, enemyBullets);
                enemyShootTimer= 0.1f; // 设置射击间隔为0.1秒
                bp.ShootSector_2(enemy->Position.x, enemy->Position.y, 200.0f, enemyBullets);
                enemyShootTimer = 0.17f; // 设置射击间隔为0.1秒
            }
        }
        else {
            //第三阶段 螺旋弹幕
            if (enemyShootTimer <= 0.0f) {
                static float spiralAngle = 0.0f;
                bp.ShootSpiral(enemy->Position.x, enemy->Position.y, 50.0f, spiralAngle,1 , enemyBullets);
                spiralAngle += M_PI / 60; // 每次发射后增加角度，形成螺旋效果
                enemyShootTimer = 0.01f; // 设置射击间隔为0.2秒
            }
        }
       
        
    }

    for (auto& bullet : playerBullets) bullet->Update(DeltaTime);
    for (auto& bullet : enemyBullets) bullet->Update(DeltaTime);

    //碰撞检测
	//(1)玩家子弹与敌人碰撞检测
    for (auto& bullet : playerBullets) 
    {
        if (!bullet->active)continue;
        for (auto& enemy : Enemies) 
        {
            if (!enemy->active)continue;
            if (bullet->CheckCollision(enemy)) 
            {
                bullet->active = false;
				enemy->hit(player->attack_point);
                if (enemy->GetBlood() <= 0) 
                {
					enemy->active = false;
                }
            }
        }
    }

	//(2)敌人子弹与玩家碰撞检测
    bool is_dead = false;
    for (auto& bullet : enemyBullets) 
    {
        if (!bullet->active) continue;
        if (bullet->CheckCollision(player)) 
        {
            bullet->active = false;
            player->hit(Enemies[0]->attack_point);
            if (player->Dead_judge()) 
            {
                
                if(!is_dead)std::cout << "输了喵。"<<'\n';
				is_dead = true;
				is_Running = false;

            }

        }
    }

    // 清理
    playerBullets.erase(
        remove_if(playerBullets.begin(), playerBullets.end(),
            [](Bullet* b) {
                if (!b->active) { delete b; return true; }
                return false;
            }), playerBullets.end());

    enemyBullets.erase(
        remove_if(enemyBullets.begin(), enemyBullets.end(),
            [](Bullet* b) {
                if (!b->active) { delete b; return true; }
                return false;
            }), enemyBullets.end());

    Enemies.erase(
        remove_if(Enemies.begin(), Enemies.end(),
            [](Enemy* e) {
                if (e->GetBlood() <= 0) { delete e; return true; }
                return false;
            }), Enemies.end());

    if (Enemies.empty()) 
    {
		std::cout << "恭喜喵，赢了喵!!!!主人是最棒的!!!"<<std::endl;
		is_Running = false;
    }

}

void Game::Render() 
{
	SDL_SetRenderDrawColor(cur_Renderer, 0, 0, 0, 255);
    SDL_RenderClear(cur_Renderer);
    
  //渲染玩家的代码
    SDL_SetRenderDrawColor(cur_Renderer, 255, 255, 255, 255);
    SDL_Rect playerRect = { (int)player->Position.x - player->radius, (int)player->Position.y - player->radius, (int)player->radius * 2, (int)player->radius * 2 };
    SDL_RenderFillRect(cur_Renderer, &playerRect);

    //敌人
    for (auto& enemy : Enemies) 
    {
        SDL_SetRenderDrawColor(cur_Renderer, 0, 0, 0, 255);
        SDL_Rect enemyRect = { (int)enemy->Position.x - enemy->radius, (int)enemy->Position.y - enemy->radius, (int)enemy->radius * 2, (int)enemy->radius * 2 };
        SDL_RenderFillRect(cur_Renderer, &enemyRect);
    }

    //子弹
    for (auto& bullet : playerBullets) 
    {
		if (bullet->active)bullet->Render(cur_Renderer);
    }
    for (auto& bullet : enemyBullets) 
    {
        if (bullet->active)bullet->Render(cur_Renderer);
    }

	//显示渲染结果
    SDL_RenderPresent(cur_Renderer);
}

void Game::Clean() 
{
    SDL_DestroyRenderer(cur_Renderer);
    SDL_DestroyWindow(cur_Window);
    SDL_Delay(1000);
    SDL_Quit();

    std::cout << "游戏清理完毕！" << std::endl;
    //内存管理
    delete player;
    for (auto& enemy : Enemies) delete enemy;
    Enemies.clear();

    for (auto& bullet : playerBullets) delete bullet;
    playerBullets.clear();

    for (auto& bullet : enemyBullets) delete bullet;
    enemyBullets.clear();
}

void Game::StartDialogue(const std::vector<DialogueLine>& Lines) 
{
    DialoueQueue = Lines;
    cur_index = 0;
	CurrentState = State::DIALOGUE;
}