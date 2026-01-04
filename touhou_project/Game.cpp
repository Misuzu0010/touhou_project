#pragma execution_character_set("utf-8")  //解决中文不显示问题
#include "Game.h"
#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <ctime>

BulletPattern bp;

Game::Game() : player(nullptr), tex_PlayerReimu(nullptr), tex_PlayerMarisa(nullptr),
tex_EnemyBullet(nullptr), tex_PlayerBullet(nullptr), tex_PowerUp(nullptr),
font(nullptr), currentPhaseIndex(0), powerUpSpawnTimer(0), powerUpSpawnInterval(3.0f),
cur_Window(nullptr), cur_Renderer(nullptr), is_Running(false) {
}

// 辅助：加载JPG并抠掉灰色背景
SDL_Texture* Game::LoadTextureWithColorKey(const char* filename) {
    SDL_Surface* surf = IMG_Load(filename);
    if (!surf) {
        std::cout << "Failed to load: " << filename << " Err: " << IMG_GetError() << std::endl;
        return nullptr;
    }

    // 尝试去除灰色背景 (RGB ~230)
    // 根据你的截图，背景很白，试着设为 230-255 范围
    // 这里取一个大概值，最好是去PS转PNG
    Uint32 colorkey = SDL_MapRGB(surf->format, 255, 255, 255);
    SDL_SetColorKey(surf, SDL_TRUE, colorkey);

    SDL_Texture* tex = SDL_CreateTextureFromSurface(cur_Renderer, surf);
    SDL_FreeSurface(surf);
    return tex;
}

bool Game::Init()
{
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    if (SDL_Init(SDL_INIT_VIDEO) != 0) return false;

    int imgFlags = IMG_INIT_PNG | IMG_INIT_JPG;
    if (!(IMG_Init(imgFlags) & imgFlags)) return false;

    if (TTF_Init() == -1) return false;

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cout << "SDL_mixer Error: " << Mix_GetError() << std::endl;
        return false;
    }

    cur_Window = SDL_CreateWindow("Touhou STG - CyberSecurity",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1920, 1080, SDL_WINDOW_SHOWN);
    cur_Renderer = SDL_CreateRenderer(cur_Window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (!cur_Window || !cur_Renderer) return false;

    // ★ 修复文件名加载
    // 角色图
    tex_PlayerReimu = IMG_LoadTexture(cur_Renderer, "Reimu.png");
    tex_PlayerMarisa = IMG_LoadTexture(cur_Renderer, "Marisa.png");
    tex_Enemy_Reimu = IMG_LoadTexture(cur_Renderer, "Reimu_Enemy.png");
    tex_Enemy_Marisa = IMG_LoadTexture(cur_Renderer, "Marisa_Enemy.png");

    // 子弹图 (带去色处理)
    // 注意：文件名里有空格！！
    tex_PlayerBullet = LoadTextureWithColorKey("PlayerBullet.png");
    tex_EnemyBullet = LoadTextureWithColorKey("EnemyBullet.png");
    tex_PowerUp = tex_PlayerBullet; // 复用

	//字体加载
    font = TTF_OpenFont("assets\\fonts\\SimHei.ttf", 24);
    if (!font) std::cout << "Font load failed!" << std::endl;

    //bgm加载
    bgm_Menu = Mix_LoadMUS("assets\\audios\\まらしぃ - 少女眠想曲　～ Dream Battle.mp3");
    bgm_Battle = Mix_LoadMUS("assets\\audios\\上海アリス幻樂団 - 少女幻葬　～ Necro-Fantasy.mp3");
    if (!bgm_Menu) std::cout << "BGM Load Failed!" << std::endl;

	//音效加载
    se_Shoot = Mix_LoadWAV("assets\\sfx\\灵击.wav");
	se_EnemyShoot = Mix_LoadWAV("assets\\sfx\\弹幕展开tan.wav");
    //se_Hit = Mix_LoadWAV("assets\\sfx\\命中.wav");
    se_Dead = Mix_LoadWAV("assets\\sfx\\死亡音效.wav");
    se_Victory = Mix_LoadWAV("assets\\sfx\\胜利音效.wav");
	se_PowerUp = Mix_LoadWAV("assets\\sfx\\火力升级.wav");
    if (!se_Shoot || !se_EnemyShoot ||  !se_Dead || !se_Victory || !se_PowerUp) {
        std::cout << "SFX Load Failed!" << std::endl;
	}

	// 初始化游戏状态
    is_Running = true;
    lastTime = SDL_GetTicks();
    CurrentState = State::MAIN_MENU;
    menuCursor = 0;
    Mix_VolumeMusic(bgmVolume);      // 同步背景音乐音量
    Mix_Volume(-1, sfxVolume);       // 同步所有音效频道的音量 (-1表示所有频道)
    Mix_PlayMusic(bgm_Menu, -1);
    return true;
}

void Game::InitBattle(CharacterID playerID)
{
    if (player) { delete player; player = nullptr; }
    for (auto& e : Enemies) delete e;
    Enemies.clear();
    for (auto& b : playerBullets) delete b;
    playerBullets.clear();
    for (auto& b : enemyBullets) delete b;
    enemyBullets.clear();

    // 根据选择创建 玩家(背身) 和 敌人(正面)
    if (playerID == CharacterID::REIMU) {
        // 玩家在下方 (960, 900)
        player = new Player(960, 900, tex_PlayerReimu);
        // Boss 在上方 (960, 200)
        Enemies.push_back(new Enemy(960, 200, tex_Enemy_Marisa));
    }
    else {
        player = new Player(960, 900, tex_PlayerMarisa);
        Enemies.push_back(new Enemy(960, 200, tex_Enemy_Reimu));
    }

    SetupDialogue(playerID);
    SetupEnemyPhases(playerID);
    currentPhaseIndex = 0;
    powerUpSpawnTimer = 0;
    shootTimer = 0;
    enemyShootTimer = 0.5f;
}

// ... 对话和阶段设置函数保持原样 ...
void Game::SetupDialogue(CharacterID playerID) {
    DialoueQueue.clear();
    cur_index = 0;
    CurrentState = State::DIALOGUE;
    stateBeforeDialogue = State::PLAYING;
    if (playerID == CharacterID::REIMU) {
        DialoueQueue.push_back({ "Reimu", "魔理沙，快停止这场 DDOS 攻击！", {255,100,100,255} });
        DialoueQueue.push_back({ "Marisa", "嘿嘿，我的僵尸网络可是无懈可击的，DAZE！", {255,255,100,255} });
    }
    else {
        DialoueQueue.push_back({ "Marisa", "灵梦！你的防火墙太脆弱了！", {255,255,100,255} });
        DialoueQueue.push_back({ "Reimu", "那就由我来给你打个补丁吧！", {255,100,100,255} });
    }
}

// 1. 设置敌人的阶段 (必须把阶段2加进去，不然永远不出手里剑！)
void Game::SetupEnemyPhases(CharacterID playerID) {
    enemyPhases.clear();

    // --- 阶段 1: 4000血触发 (警告) ---
    EnemyPhase p1;
    p1.hpThreshold = 4000;
    p1.dialogueTriggered = false;
    p1.dialogues.push_back({ "System", "警告：检测到防火墙被突破！", {200,200,200,255} });
    p1.dialogues.push_back({ "Boss", "部署加密锁，把你的数据都锁死吧！", {255,50,50,255} });
    enemyPhases.push_back(p1);

    // --- ★★★ 阶段 2: 2000血触发 (暴走/手里剑) ★★★ ---
    // 主人之前漏了这个，所以才看不到手里剑！
    EnemyPhase p2;
    p2.hpThreshold = 2000;
    p2.dialogueTriggered = false;
    p2.dialogues.push_back({ "System", "严重错误：内核异常！", {255,0,0,255} });
    p2.dialogues.push_back({ "Boss", "格式化所有分区！全！部！删！除！", {255,0,0,255} });
    enemyPhases.push_back(p2);
}

// 2. 检查阶段切换 (顺便清空子弹)
void Game::CheckEnemyPhase() {
    if (Enemies.empty()) return;
    float hp = Enemies[0]->GetBlood();

    // 遍历所有阶段配置
    for (int i = currentPhaseIndex; i < enemyPhases.size(); ++i) {
        // 如果血量低于阈值，且该阶段对话还没触发过
        if (hp <= enemyPhases[i].hpThreshold && !enemyPhases[i].dialogueTriggered) {

            // 更新当前阶段索引
            currentPhaseIndex = i + 1; // 0是初始, 触发p1变成阶段1, 触发p2变成阶段2
            // 注意：这里的 currentPhaseIndex 对应 Update 里的逻辑判断
            // 如果你的 Update 里写的是 if(currentPhaseIndex == 1) 发金锁，那这里要对上
            // 修正逻辑：建议直接用 i+1 或者专门的状态变量，这里假设 Update 里是用 0, 1, 2 区分的

            // 标记对话已触发
            enemyPhases[i].dialogueTriggered = true;

            // 加载对话内容
            DialoueQueue = enemyPhases[i].dialogues;
            cur_index = 0;

            // 记录之前的状态并进入对话
            stateBeforeDialogue = CurrentState;
            CurrentState = State::DIALOGUE;

            // ★★★ 核心修复：清空所有敌方子弹 (消弹) ★★★
            for (auto b : enemyBullets) {
                delete b; // 释放内存
            }
            enemyBullets.clear(); // 清空容器

            std::cout << "Phase Triggered! Bullets Cleared." << std::endl;
            break;
        }
    }
}
// Game.cpp


// ... 其他头文件 ...

// ★★★ 链接器报错就是因为找不到下面这个函数！ ★★★
void Game::Run()
{
    while (is_Running) {
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        if (deltaTime > 0.05f) deltaTime = 0.05f;
        lastTime = currentTime;

        HandleEvents();
        Update(deltaTime);
        Render();
    }
}

// ... 以及其他的函数实现 ...
void Game::HandleEvents()
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) is_Running = false;

    }
}

void Game::Update(float DeltaTime)
{
    const Uint8* key = SDL_GetKeyboardState(NULL);
    static bool keyLock = false;

    switch (CurrentState)
    {
    case State::MAIN_MENU:
    {
        if (!keyLock) {
            if (key[SDL_SCANCODE_UP]) {
                if (menuSelect > 0) menuSelect--;
                keyLock = true;
            }
            if (key[SDL_SCANCODE_DOWN]) {
				if (menuSelect < 2) menuSelect++; // 目前只有两个按钮：开始和退出  后续可以根据主菜单功能加更多
                keyLock = true;
            }
            if (key[SDL_SCANCODE_Z]) {
                if (menuSelect == 0) {
                    CurrentState = State::SELECT_CHARACTER;
                }
                else if (menuSelect == 1) {
                    CurrentState = State::VOLUME_SETTINGS;
                }
                else if (menuSelect == 2) is_Running = false;
                keyLock = true;
            }
            
        }

        // 当所有按键都松开时，解锁
        if (!key[SDL_SCANCODE_UP] && !key[SDL_SCANCODE_DOWN] && !key[SDL_SCANCODE_Z]) {
            keyLock = false;
        }
        break;
    }

    case State::VOLUME_SETTINGS:
    {
        if (!keyLock) {
            // 1. 上下键选择调节对象
            if (key[SDL_SCANCODE_UP]) {
                if (volMenuSelect > 0) volMenuSelect--;
                keyLock = true;
            }
            else if (key[SDL_SCANCODE_DOWN]) {
                if (volMenuSelect < 2) volMenuSelect++;
                keyLock = true;
            }

            // 2. 左右键调节数值
            if (volMenuSelect == 0) { // 调节 BGM
                if (key[SDL_SCANCODE_LEFT] && bgmVolume > 0) {
                    bgmVolume -= 8;
                    Mix_VolumeMusic(bgmVolume);
                    keyLock = true;
                }
                if (key[SDL_SCANCODE_RIGHT] && bgmVolume < 128) {
                    bgmVolume += 8;
                    Mix_VolumeMusic(bgmVolume);
                    keyLock = true;
                }
            }
            else if (volMenuSelect == 1) { // 调节音效
                if (key[SDL_SCANCODE_LEFT] && sfxVolume > 0) {
                    sfxVolume -= 8;
                    // Mix_Volume(-1, vol) 调节所有音效频道
                    Mix_Volume(-1, sfxVolume);
                    keyLock = true;
                }
                if (key[SDL_SCANCODE_RIGHT] && sfxVolume < 128) {
                    sfxVolume += 8;
                    Mix_Volume(-1, sfxVolume);
                    keyLock = true;
                }
            }

            // 3. 确认返回
            if (key[SDL_SCANCODE_Z] || key[SDL_SCANCODE_ESCAPE]) {
                if (volMenuSelect == 2 || key[SDL_SCANCODE_ESCAPE]) {
                    CurrentState = State::MAIN_MENU;
                }
                keyLock = true;
            }
        }

        // 解锁逻辑（确保包含左右键）
        if (!key[SDL_SCANCODE_UP] && !key[SDL_SCANCODE_DOWN] &&
            !key[SDL_SCANCODE_LEFT] && !key[SDL_SCANCODE_RIGHT] &&
            !key[SDL_SCANCODE_Z] && !key[SDL_SCANCODE_ESCAPE]) {
            keyLock = false;
        }
        break;
    }

    case State::SELECT_CHARACTER:
    {

        if (!keyLock) {
            if (key[SDL_SCANCODE_LEFT]) menuCursor = 0;
            if (key[SDL_SCANCODE_RIGHT]) menuCursor = 1;
			if (key[SDL_SCANCODE_ESCAPE]) CurrentState = State::MAIN_MENU;
            if (key[SDL_SCANCODE_Z]) {
                selectedCharID = (menuCursor == 0) ? CharacterID::REIMU : CharacterID::MARISA;
                InitBattle(selectedCharID);
                Mix_FadeOutMusic(500); // 500ms 内淡出
                Mix_PlayMusic(bgm_Battle, -1);
            }
            if (key[SDL_SCANCODE_LEFT] || key[SDL_SCANCODE_RIGHT] || key[SDL_SCANCODE_Z]) keyLock = true;
        }
        else {
            if (!key[SDL_SCANCODE_LEFT] && !key[SDL_SCANCODE_RIGHT] && !key[SDL_SCANCODE_Z]) keyLock = false;
        }
        break;
    }

    case State::DIALOGUE:
    {
        static bool zPressed = false;
        if (key[SDL_SCANCODE_Z] && !zPressed) {
            cur_index++;
            if (cur_index >= DialoueQueue.size()) CurrentState = stateBeforeDialogue;
            zPressed = true;
        }
        if (!key[SDL_SCANCODE_Z]) zPressed = false;
        break;
    }

    case State::PLAYING:
    {
        if (player) player->Update(DeltaTime);

        // 玩家射击
        if (key[SDL_SCANCODE_Z] && shootTimer <= 0.0f) {
            Mix_PlayChannel(-1, se_Shoot, 0);
            BulletType bType = (selectedCharID == CharacterID::REIMU) ? BulletType::PLAYER_TALISMAN : BulletType::PLAYER_STAR;
            // 简单实现：根据Level增加子弹
            playerBullets.push_back(new Bullet(player->Position.x, player->Position.y, 0, -600, bType));
            if (player->powerLevel >= 1) {
                playerBullets.push_back(new Bullet(player->Position.x - 15, player->Position.y, -100, -600, bType));
                playerBullets.push_back(new Bullet(player->Position.x + 15, player->Position.y, 100, -600, bType));
            }
            shootTimer = 0.07f;
        }
        shootTimer -= DeltaTime;

        // 生成P点
        powerUpSpawnTimer -= DeltaTime;
        if (powerUpSpawnTimer <= 0) {
            powerUps.push_back(new PowerUp(50 + rand() % 540, -20, tex_PowerUp));
            powerUpSpawnTimer = 3.0f;
        }

        // Boss逻辑
        // Game.cpp -> Update 函数内部 -> Boss逻辑部分

        // ==========================================================
        // ★★★ Boss AI 核心逻辑 (State Machine) ★★★
        // ==========================================================
        enemyShootTimer -= DeltaTime;
        static float angleOffset = 0; // 用于让弹幕旋转起来

        if (!Enemies.empty() && Enemies[0]->active) {
            if (enemyShootTimer <= 0) {
                // 根据 currentPhaseIndex 决定攻击方式
                // 0: 初始阶段, 1: 警告阶段, 2: 暴走阶段 (你可以自己去 SetupEnemyPhases 里加更多阶段)
                if (se_EnemyShoot) Mix_PlayChannel(-1, se_EnemyShoot, 0);
                // ★阶段 0: 基础弹幕 (绿球)
                if (currentPhaseIndex == 0) {

                    // 发射 36 发子弹 (密度提升)，类型为 VIRUS
                    bp.ShootRotatingRing(Enemies[0]->Position.x, Enemies[0]->Position.y, 300, 36, angleOffset, enemyBullets, BulletType::VIRUS);

                    angleOffset += 0.1f; // 缓慢旋转
                    enemyShootTimer = 0.5f; // ★射速提升：每0.2秒发一波 (原来是0.5)

                }
                // ★阶段 1: 追踪陷阱 (金锁)
                else if (currentPhaseIndex == 1) {

                    // 发射 24 发“停顿-追踪”弹，类型为 LOCK
                    // 这种子弹生成后会停一下，然后飞向玩家
                    bp.ShootStopAndGo(Enemies[0]->Position.x, Enemies[0]->Position.y, 1.0f, 600, player, 24, enemyBullets, BulletType::LOCK);

                    angleOffset -= 0.15f; // 反向旋转
                    enemyShootTimer = 0.8f; // 这种强力弹幕间隔长一点

                }
                // ★阶段 2: 终极暴走 (紫色手里剑) - 假设你在 SetupEnemyPhases 里加了这个阶段
                // 或者如果血量很低强制进入
                else if (currentPhaseIndex >= 2 || Enemies[0]->GetBlood() < 2000) {

                // 疯狂螺旋！一次喷射 5 发，像机关枪一样扫射
                // 这里的 angleOffset 每次 update 都在狂变
                    angleOffset += 0.5f;

                // 使用新写的 ShootSpiral
                    bp.ShootSpiral(Enemies[0]->Position.x, Enemies[0]->Position.y, 500, angleOffset, 5, 0.2f, enemyBullets, BulletType::SHURIKEN);

                    enemyShootTimer = 0.05f; // ★极速：每 0.05秒 发射一次！
                }
            }
        }

        // 更新所有对象
        for (auto b : playerBullets) b->Update(DeltaTime);
        for (auto b : enemyBullets) b->Update(DeltaTime);
        for (auto p : powerUps) p->Update(DeltaTime);

        // 碰撞：子弹打Boss
        for (auto b : playerBullets) {
            if (!b->active) continue;
            for (auto e : Enemies) {
                if (e->active && b->CheckCollision(e)) {
                    e->hit(player->attack_point);
                    //if (se_Hit) Mix_PlayChannel(-1, se_Hit, 0); // 敌人受击声
                    b->active = false;
                }
            }
        }

        // 碰撞：子弹打玩家
        for (auto b : enemyBullets) {
            if (b->active && player && player->CheckCollision(b)) {
                player->hit(10);
                b->active = false;
                if (player->Dead_judge()) {
                    if (se_Dead) Mix_PlayChannel(-1, se_Dead, 0); // 玩家死亡音效
                    Mix_HaltMusic(); // 停止BGM
                    CurrentState = State::GAME_OVER;
                }
            }
        }

        // 碰撞：吃P点
        for (auto p : powerUps) {
            if (p->active && player && player->CheckCollision(p)) {
                if (player->CollectPowerUp()) {
                    if (se_PowerUp) Mix_PlayChannel(-1, se_PowerUp, 0);
                }
                p->active = false;
            }
        }

        // 清理失效对象
        // (略去繁琐的 remove_if 代码，保持整洁，实际运行时建议加上)
        // ... CheckEnemyPhase() ...
        CheckEnemyPhase();

        // 胜利判定
        if (!Enemies.empty() && Enemies[0]->GetBlood() <= 0) {
            if (se_Victory) Mix_PlayChannel(-1, se_Victory, 0); // 胜利音效
            Mix_HaltMusic(); // 停止战斗BGM
            CurrentState = State::VICTORY;
        }
        break;
    }

    case State::GAME_OVER:
    case State::VICTORY:
        static bool escPressed = false;

        if (key[SDL_SCANCODE_ESCAPE] && !escPressed) {
            // 1. 切换游戏状态回到主菜单
            CurrentState = State::MAIN_MENU;
            menuSelect = 0; // 重置菜单光标到第一项

            // 2. 音乐切换核心代码
            if (bgm_Menu) {
                Mix_HaltMusic();             // 停止当前所有正在播放的背景音乐
                Mix_PlayMusic(bgm_Menu, -1); // 重新循环播放主菜单背景音乐 (-1代表无限循环)
            }
            if (!key[SDL_SCANCODE_ESCAPE]) {
                escPressed = false;
            }
            break;
        }
    }
}
void Game::Render()
{
    // ============================================================
    // 状态 0: 主菜单
    // ============================================================
    SDL_SetRenderDrawColor(cur_Renderer, 0, 0, 0, 255);
    SDL_RenderClear(cur_Renderer);
    if (CurrentState == State::MAIN_MENU) {
        if (font) {
            // --- [1. 绘制游戏大标题] ---
            SDL_Color white = { 255, 255, 255, 255 };
            SDL_Surface* titleSurf = TTF_RenderUTF8_Blended(font, "东方代码乡", white);
            if (titleSurf) {
                SDL_Texture* titleTex = SDL_CreateTextureFromSurface(cur_Renderer, titleSurf);
                // 将标题放大并居中（1920x1080屏幕）
                SDL_Rect titleRect = { (1920 - titleSurf->w * 2) / 2, 250, titleSurf->w * 2, titleSurf->h * 2 };
                SDL_RenderCopy(cur_Renderer, titleTex, NULL, &titleRect);
                SDL_FreeSurface(titleSurf); SDL_DestroyTexture(titleTex);
            }

            // --- [2. 绘制选项 0：开始游戏] ---
            SDL_Color colorStart = (menuSelect == 0) ? SDL_Color{ 255, 255, 0, 255 } : SDL_Color{ 150, 150, 150, 255 };
            SDL_Surface* s1 = TTF_RenderUTF8_Blended(font, "开始防火墙检测 (START)", colorStart);
            if (s1) {
                SDL_Texture* t1 = SDL_CreateTextureFromSurface(cur_Renderer, s1);
                SDL_Rect r1 = { (1920 - s1->w) / 2, 550, s1->w, s1->h }; // 位置上移
                SDL_RenderCopy(cur_Renderer, t1, NULL, &r1);

                if (menuSelect == 0) {
                    SDL_SetRenderDrawColor(cur_Renderer, 255, 255, 0, 255); // 指示器设为黄色
                    SDL_Rect pointer = { r1.x - 50, r1.y + 5, 30, 30 };
                    SDL_RenderFillRect(cur_Renderer, &pointer);
                }
                SDL_FreeSurface(s1); SDL_DestroyTexture(t1);
            }

            // --- [3. 绘制选项 1：音量调节] ---
            SDL_Color colorVol = (menuSelect == 1) ? SDL_Color{ 255, 255, 0, 255 } : SDL_Color{ 150, 150, 150, 255 };
            SDL_Surface* sVol = TTF_RenderUTF8_Blended(font, "音量设置 (VOLUME)", colorVol);
            if (sVol) {
                SDL_Texture* tVol = SDL_CreateTextureFromSurface(cur_Renderer, sVol);
                SDL_Rect rVol = { (1920 - sVol->w) / 2, 650, sVol->w, sVol->h }; // 位于中间
                SDL_RenderCopy(cur_Renderer, tVol, NULL, &rVol);

                if (menuSelect == 1) {
                    SDL_SetRenderDrawColor(cur_Renderer, 255, 255, 0, 255);
                    SDL_Rect pointer = { rVol.x - 50, rVol.y + 5, 30, 30 };
                    SDL_RenderFillRect(cur_Renderer, &pointer);
                }
                SDL_FreeSurface(sVol); SDL_DestroyTexture(tVol);
            }

            // --- [4. 绘制选项 2：退出游戏] ---
            SDL_Color colorQuit = (menuSelect == 2) ? SDL_Color{ 255, 255, 0, 255 } : SDL_Color{ 150, 150, 150, 255 };
            SDL_Surface* s2 = TTF_RenderUTF8_Blended(font, "断开连接 (QUIT)", colorQuit);
            if (s2) {
                SDL_Texture* t2 = SDL_CreateTextureFromSurface(cur_Renderer, s2);
                SDL_Rect r2 = { (1920 - s2->w) / 2, 750, s2->w, s2->h };
                SDL_RenderCopy(cur_Renderer, t2, NULL, &r2);

                if (menuSelect == 2) {
                    SDL_SetRenderDrawColor(cur_Renderer, 255, 255, 0, 255);
                    SDL_Rect pointer = { r2.x - 50, r2.y + 5, 30, 30 };
                    SDL_RenderFillRect(cur_Renderer, &pointer);
                }
                SDL_FreeSurface(s2); SDL_DestroyTexture(t2);
            }

            // --- [5. 绘制页脚提示] ---
            SDL_Surface* hint = TTF_RenderUTF8_Blended(font, "使用方向键切换，Z 键确认", { 100, 100, 100, 255 });
            if (hint) {
                SDL_Texture* hTex = SDL_CreateTextureFromSurface(cur_Renderer, hint);
                SDL_Rect hRect = { (1920 - hint->w) / 2, 950, hint->w, hint->h };
                SDL_RenderCopy(cur_Renderer, hTex, NULL, &hRect);
                SDL_FreeSurface(hint); SDL_DestroyTexture(hTex);
            }
        }
    }

    // ============================================================
    // 状态 1: 音量调节界面
    // ============================================================
    if (CurrentState == State::VOLUME_SETTINGS) {
        if (font) {
            // --- [1. 背景暗化] ---
            SDL_SetRenderDrawBlendMode(cur_Renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(cur_Renderer, 0, 0, 0, 180);
            SDL_Rect fullScreen = { 0, 0, 1920, 1080 };
            SDL_RenderFillRect(cur_Renderer, &fullScreen);

            // --- [2. 绘制标题] ---
            SDL_Color white = { 255, 255, 255, 255 };
            SDL_Surface* sT = TTF_RenderUTF8_Blended(font, "音量详细设置", white);
            if (sT) {
                SDL_Texture* tT = SDL_CreateTextureFromSurface(cur_Renderer, sT);
                SDL_Rect rT = { (1920 - sT->w) / 2, 300, sT->w, sT->h };
                SDL_RenderCopy(cur_Renderer, tT, NULL, &rT);
                SDL_FreeSurface(sT); SDL_DestroyTexture(tT);
            }

            // --- [3. 绘制 BGM 调节项] ---
            SDL_Color colorBGM = (volMenuSelect == 0) ? SDL_Color{ 255, 255, 0, 255 } : SDL_Color{ 150, 150, 150, 255 };
            std::string bgmStr = "背景音乐 (BGM):  < " + std::to_string(bgmVolume * 100 / 128) + "% >";
            SDL_Surface* sB = TTF_RenderUTF8_Blended(font, bgmStr.c_str(), colorBGM);
            if (sB) {
                SDL_Texture* tB = SDL_CreateTextureFromSurface(cur_Renderer, sB);
                SDL_Rect rB = { (1920 - sB->w) / 2, 450, sB->w, sB->h };
                SDL_RenderCopy(cur_Renderer, tB, NULL, &rB);
                SDL_FreeSurface(sB); SDL_DestroyTexture(tB);
            }

            // --- [4. 绘制音效调节项] ---
            SDL_Color colorSFX = (volMenuSelect == 1) ? SDL_Color{ 255, 255, 0, 255 } : SDL_Color{ 150, 150, 150, 255 };
            std::string sfxStr = "游戏音效 (SFX):  < " + std::to_string(sfxVolume * 100 / 128) + "% >";
            SDL_Surface* sS = TTF_RenderUTF8_Blended(font, sfxStr.c_str(), colorSFX);
            if (sS) {
                SDL_Texture* tS = SDL_CreateTextureFromSurface(cur_Renderer, sS);
                SDL_Rect rS = { (1920 - sS->w) / 2, 550, sS->w, sS->h };
                SDL_RenderCopy(cur_Renderer, tS, NULL, &rS);
                SDL_FreeSurface(sS); SDL_DestroyTexture(tS);
            }

            // --- [5. 绘制返回选项] ---
            SDL_Color colorBack = (volMenuSelect == 2) ? SDL_Color{ 255, 255, 0, 255 } : SDL_Color{ 150, 150, 150, 255 };
            SDL_Surface* sBack = TTF_RenderUTF8_Blended(font, "确认并返回主菜单", colorBack);
            if (sBack) {
                SDL_Texture* tBack = SDL_CreateTextureFromSurface(cur_Renderer, sBack);
                SDL_Rect rBack = { (1920 - sBack->w) / 2, 700, sBack->w, sBack->h };
                SDL_RenderCopy(cur_Renderer, tBack, NULL, &rBack);
                SDL_FreeSurface(sBack); SDL_DestroyTexture(tBack);
            }
        }
    }

    // ============================================================
// 状态 A: 角色选择界面 (仅调整位置至全屏居中)
// ============================================================
    if (CurrentState == State::SELECT_CHARACTER) {
        // 设定 UI 参数
        int iconSize = 200;  // 角色图标大小
        int spacing = 400;   // 两个角色中心点之间的间距
        int centerX = 1920 / 2;
        int centerY = 1080 / 2;

        // 计算居中位置：左侧角色和右侧角色的矩形区域
        SDL_Rect rRect = { centerX - (spacing / 2) - (iconSize / 2), centerY - (iconSize / 2), iconSize, iconSize };
        SDL_Rect mRect = { centerX + (spacing / 2) - (iconSize / 2), centerY - (iconSize / 2), iconSize, iconSize };

        // 绘制灵梦 (Reimu)
        if (tex_PlayerReimu) {
            SDL_SetTextureColorMod(tex_PlayerReimu, (menuCursor == 0) ? 255 : 100, (menuCursor == 0) ? 255 : 100, (menuCursor == 0) ? 255 : 100);
            SDL_RenderCopy(cur_Renderer, tex_PlayerReimu, NULL, &rRect);
        }
        // 绘制魔理沙 (Marisa)
        if (tex_PlayerMarisa) {
            SDL_SetTextureColorMod(tex_PlayerMarisa, (menuCursor == 1) ? 255 : 100, (menuCursor == 1) ? 255 : 100, (menuCursor == 1) ? 255 : 100);
            SDL_RenderCopy(cur_Renderer, tex_PlayerMarisa, NULL, &mRect);
        }

        // 绘制高亮边框
        SDL_SetRenderDrawColor(cur_Renderer, 255, 255, 255, 255);
        SDL_Rect border = (menuCursor == 0) ? rRect : mRect;
        border.x -= 5; border.y -= 5; border.w += 10; border.h += 10;
        SDL_RenderDrawRect(cur_Renderer, &border);

        // 绘制文字提示 (水平居中)
        if (font) {
            SDL_Surface* surf = TTF_RenderUTF8_Blended(font, "选择角色(←/→ + Z)", { 255, 255, 255, 255 });
            if (surf) {
                SDL_Texture* t = SDL_CreateTextureFromSurface(cur_Renderer, surf);
                // 文字放在图标上方 100 像素处并居中
                SDL_Rect dst = { centerX - (surf->w / 2), centerY - (iconSize / 2) - 80, surf->w, surf->h };
                SDL_RenderCopy(cur_Renderer, t, NULL, &dst);
                SDL_FreeSurface(surf);
                SDL_DestroyTexture(t);
            }
        }
    }
    // ============================================================
    // 状态 B: 游戏进行中 OR 对话中 (包含时停逻辑)
    // ============================================================
    else if (CurrentState == State::PLAYING || CurrentState == State::DIALOGUE) {

        // 1. 绘制游戏层
        if (player) player->Render(cur_Renderer);
        for (auto e : Enemies) e->Render(cur_Renderer);
        // 子弹
        for (auto b : playerBullets) if (b->active) b->Render(cur_Renderer, tex_PlayerBullet);
        for (auto b : enemyBullets) if (b->active) b->Render(cur_Renderer, tex_EnemyBullet);
        // 道具
        for (auto p : powerUps) if (p->active) p->Render(cur_Renderer);

        // 2. 绘制 UI (血条与数值)
        if (font && player) {
            // --- [参数定义] ---
            int uiX = 108;           // UI 起始 X 坐标
            int uiY = 60;           // UI 起始 Y 坐标
            int barW = 300;         // 血条总宽度
            int barH = 20;          // 血条高度
            float hpPercent = player->hp / player->max_hp; // 计算血量比例
            if (hpPercent < 0) hpPercent = 0;

            // --- [1. 绘制血条底槽 (深灰色背景)] ---
            SDL_Rect bgRect = { uiX, uiY, barW, barH };
            SDL_SetRenderDrawColor(cur_Renderer, 50, 50, 50, 255);
            SDL_RenderFillRect(cur_Renderer, &bgRect);

            // --- [2. 绘制实际血条 (红色动态条)] ---
            SDL_Rect hpRect = { uiX, uiY, (int)(barW * hpPercent), barH };
            SDL_SetRenderDrawColor(cur_Renderer, 255, 0, 0, 255); // 红色代表血量
            SDL_RenderFillRect(cur_Renderer, &hpRect);

            // --- [3. 绘制边框 (白色外框)] ---
            SDL_SetRenderDrawColor(cur_Renderer, 200, 200, 200, 255);
            SDL_RenderDrawRect(cur_Renderer, &bgRect);

            // --- [4. 绘制文字数值 (HP 与 Power)] ---
            char buf[64];
            sprintf_s(buf, "HP: %.0f / %.0f  Power: %d", player->hp, player->max_hp, player->powerLevel);

            SDL_Surface* s = TTF_RenderUTF8_Blended(font, buf, { 255, 255, 255, 255 });
            if (s) {
                SDL_Texture* t = SDL_CreateTextureFromSurface(cur_Renderer, s);
                // 将文字放在血条的正下方 (偏移 25 像素)
                SDL_Rect textDst = { uiX, uiY + barH + 5, s->w, s->h };
                SDL_RenderCopy(cur_Renderer, t, NULL, &textDst);

                SDL_FreeSurface(s);
                SDL_DestroyTexture(t);
            }
        }
        // 3. 绘制 Boss 血条 (顶部居中长条)
        if (!Enemies.empty() && Enemies[0]->active) {
            Enemy* boss = Enemies[0];

            // --- [参数定义] ---
            int screenW = 1900;
            int barW = 600;         // Boss 血条宽度（比玩家的长，显大气）
            int barH = 15;           // Boss 血条高度
            int uiX = (screenW - barW) / 2; // 水平居中
            int uiY = 10;            // 距离顶部 10 像素

            // 假设 Boss 的最大血量可以通过逻辑推断或在 Enemy 类中定义
            // 这里根据你的 SetupEnemyPhases 逻辑，假设初始血量较高（例如 6000）
            // 如果 Enemy 类有 GetMaxBlood() 最好，如果没有，这里先设为一个基准值
            float currentHp = boss->GetBlood();
            static float maxHp = 6000.0f; // 建议在 Enemy 类初始化时记录这个值

            float hpPercent = currentHp / maxHp;
            if (hpPercent < 0) hpPercent = 0;
            if (hpPercent > 1) hpPercent = 1;

            // --- [1. 绘制底槽 (半透明黑色)] ---
            SDL_SetRenderDrawBlendMode(cur_Renderer, SDL_BLENDMODE_BLEND);
            SDL_Rect bgRect = { uiX, uiY, barW, barH };
            SDL_SetRenderDrawColor(cur_Renderer, 0, 0, 0, 150);
            SDL_RenderFillRect(cur_Renderer, &bgRect);

            // --- [2. 绘制血条 (亮紫色/红色)] ---
            SDL_Rect hpRect = { uiX, uiY, (int)(barW * hpPercent), barH };
            // 东方风格 Boss 常规用紫色或深红色
            SDL_SetRenderDrawColor(cur_Renderer, 200, 0, 200, 255);
            SDL_RenderFillRect(cur_Renderer, &hpRect);

            // --- [3. 绘制外框 (白色)] ---
            SDL_SetRenderDrawColor(cur_Renderer, 255, 255, 255, 255);
            SDL_RenderDrawRect(cur_Renderer, &bgRect);

            // --- [4. 绘制 Boss 阶段标记 (可选)] ---
            // 在血条上标记出 4000 和 2000 的位置，对应你的阶段切换
            SDL_SetRenderDrawColor(cur_Renderer, 255, 255, 255, 200);
            int p1X = uiX + (int)(barW * (4000.0f / maxHp));
            int p2X = uiX + (int)(barW * (2000.0f / maxHp));
            SDL_RenderDrawLine(cur_Renderer, p1X, uiY, p1X, uiY + barH);
            SDL_RenderDrawLine(cur_Renderer, p2X, uiY, p2X, uiY + barH);
        }
        // 3. ★★★ 时停遮罩与对话框逻辑 (别再漏掉了！) ★★★
        if (CurrentState == State::DIALOGUE && cur_index < DialoueQueue.size()) {

            // --- [时停黑纱] ---
            SDL_SetRenderDrawBlendMode(cur_Renderer, SDL_BLENDMODE_BLEND);
            // 既然背景是纯黑，这个遮罩主要用来压暗角色和子弹，突显文字
            SDL_SetRenderDrawColor(cur_Renderer, 0, 0, 0, 150);
            SDL_Rect fullscreen = { 0, 0, 1920, 1080 };
            SDL_RenderFillRect(cur_Renderer, &fullscreen);
            SDL_SetRenderDrawBlendMode(cur_Renderer, SDL_BLENDMODE_NONE);

            // --- [对话框底板] ---
            SDL_Rect boxRect = { 100, 750, 1720, 250 };
            SDL_SetRenderDrawColor(cur_Renderer, 0, 0, 60, 255); // 深蓝色底板
            SDL_RenderFillRect(cur_Renderer, &boxRect);
            SDL_SetRenderDrawColor(cur_Renderer, 200, 200, 200, 255); // 银色边框
            SDL_RenderDrawRect(cur_Renderer, &boxRect);

            // --- [文字渲染] ---
            if (font) {
                DialogueLine& line = DialoueQueue[cur_index];

                // 名字
                SDL_Surface* nameSurf = TTF_RenderUTF8_Blended(font, line.name.c_str(), line.color);
                if (nameSurf) {
                    SDL_Texture* nameTex = SDL_CreateTextureFromSurface(cur_Renderer, nameSurf);
                    SDL_Rect nameDst = { 130, 765, nameSurf->w, nameSurf->h };
                    SDL_RenderCopy(cur_Renderer, nameTex, NULL, &nameDst);
                    SDL_FreeSurface(nameSurf); SDL_DestroyTexture(nameTex);
                }

                // 内容
                SDL_Surface* textSurf = TTF_RenderUTF8_Blended(font, line.Content.c_str(), { 255, 255, 255, 255 });
                if (textSurf) {
                    SDL_Texture* textTex = SDL_CreateTextureFromSurface(cur_Renderer, textSurf);
                    SDL_Rect textDst = { 150, 830, textSurf->w, textSurf->h };
                    SDL_RenderCopy(cur_Renderer, textTex, NULL, &textDst);
                    SDL_FreeSurface(textSurf); SDL_DestroyTexture(textTex);
                }

                // 提示
                SDL_Surface* hintSurf = TTF_RenderUTF8_Blended(font, "Press Z >>", { 100, 255, 255, 255 });
                if (hintSurf) {
                    SDL_Texture* hintTex = SDL_CreateTextureFromSurface(cur_Renderer, hintSurf);
                    SDL_Rect hintDst = { 1650, 950, hintSurf->w, hintSurf->h };
                    SDL_RenderCopy(cur_Renderer, hintTex, NULL, &hintDst);
                    SDL_FreeSurface(hintSurf); SDL_DestroyTexture(hintTex);
                }
            }
        }
    }

    // ============================================================
    // 状态 C: 结算界面
    // ============================================================
    else if (CurrentState == State::VICTORY || CurrentState == State::GAME_OVER) {
        if (font) {
            const char* msg = (CurrentState == State::VICTORY) ? "VICTORY! (按下ESC返回)" : "满身疮痍 (按下ESC返回)";
            SDL_Color color = (CurrentState == State::VICTORY) ? SDL_Color{ 255, 255, 0, 255 } : SDL_Color{ 255, 0, 0, 255 };

            SDL_Surface* surf = TTF_RenderUTF8_Blended(font, msg, color);
            if (surf) {
                SDL_Texture* t = SDL_CreateTextureFromSurface(cur_Renderer, surf);
                SDL_Rect dst = { (1920 - surf->w) / 2, (1080 - surf->h) / 2, surf->w, surf->h };
                SDL_RenderCopy(cur_Renderer, t, NULL, &dst);
                SDL_FreeSurface(surf); SDL_DestroyTexture(t);
            }
        }
    }

    SDL_RenderPresent(cur_Renderer);
}

void Game::Clean() {
    // 停止播放并释放音乐资源
    Mix_HaltMusic();
    Mix_FreeMusic(bgm_Menu);
    Mix_FreeMusic(bgm_Battle);
    Mix_FreeChunk(se_Shoot);
    Mix_FreeChunk(se_EnemyShoot);
    //Mix_FreeChunk(se_Hit);
    Mix_FreeChunk(se_Dead);
    Mix_FreeChunk(se_Victory);
    Mix_FreeChunk(se_PowerUp);
    // 关闭音频设备
    Mix_CloseAudio();
    // 记得销毁所有Texture和指针
    SDL_DestroyRenderer(cur_Renderer);
    SDL_DestroyWindow(cur_Window);
    TTF_Quit(); IMG_Quit(); SDL_Quit();
}
