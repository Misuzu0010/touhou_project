#pragma execution_character_set("utf-8")  //解决中文不显示问题
#include "Game.h"
#include <algorithm>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <memory>

namespace {
// 全局玩法常量集中放在匿名命名空间，避免屏幕尺寸、音量上限等魔法数字散落。
constexpr int ScreenWidth = 1920;
constexpr int ScreenHeight = 1080;
constexpr float ContinueSeconds = 10.0f;
constexpr int MaxVolume = 128;
constexpr int MaxPowerLevel = 4;

BulletPattern bp;

// 移除 active=false 的对象。容器元素是 unique_ptr，erase 时会自动释放内存。
template <typename T>
void RemoveInactiveObjects(std::vector<std::unique_ptr<T>>& objects)
{
    objects.erase(std::remove_if(objects.begin(), objects.end(),
        [](const std::unique_ptr<T>& object) {
            return object && !object->IsActive();
        }), objects.end());
}
}

Game::Game() = default;

// 加载带透明色键的贴图。
// 当前主要用于子弹图集：把纯白背景设为透明，再转换为 SDL_Texture。
SDL_Texture* Game::LoadTextureWithColorKey(const char* filename) {
    SDL_Surface* surf = IMG_Load(filename);
    if (!surf) {
        std::cout << "Failed to load: " << filename << " Err: " << IMG_GetError() << std::endl;
        return nullptr;
    }

    // 只处理纯白 RGB(255,255,255)。如果素材背景不是纯白，应优先在素材中制作透明通道。
    Uint32 colorkey = SDL_MapRGB(surf->format, 255, 255, 255);
    SDL_SetColorKey(surf, SDL_TRUE, colorkey);

    SDL_Texture* tex = SDL_CreateTextureFromSurface(cur_Renderer, surf);
    SDL_FreeSurface(surf);
    return tex;
}

bool Game::Init()
{
    // 初始化随机种子，P 点生成位置和横向漂移会使用 rand()。
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    // 初始化 SDL 子系统。视频用于窗口/渲染，音频用于 SDL_mixer。
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) return false;

    int imgFlags = IMG_INIT_PNG | IMG_INIT_JPG;
    if (!(IMG_Init(imgFlags) & imgFlags)) return false;

    if (TTF_Init() == -1) return false;

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cout << "SDL_mixer Error: " << Mix_GetError() << std::endl;
        return false;
    }
    Mix_AllocateChannels(64);
 

    cur_Window = SDL_CreateWindow("Touhou STG - CyberSecurity",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, ScreenWidth, ScreenHeight, SDL_WINDOW_SHOWN);
    cur_Renderer = SDL_CreateRenderer(cur_Window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (!cur_Window || !cur_Renderer) return false;

    // 角色贴图：玩家使用背身图，Boss 使用正面图。
    tex_PlayerReimu = IMG_LoadTexture(cur_Renderer, "Reimu.png");
    tex_PlayerMarisa = IMG_LoadTexture(cur_Renderer, "Marisa.png");
    tex_Enemy_Reimu = IMG_LoadTexture(cur_Renderer, "Reimu_Enemy.png");
    tex_Enemy_Marisa = IMG_LoadTexture(cur_Renderer, "Marisa_Enemy.png");

    // 子弹图集使用色键处理透明背景；P 点复用玩家子弹图集。
    tex_PlayerBullet = LoadTextureWithColorKey("PlayerBullet.png");
    tex_EnemyBullet = LoadTextureWithColorKey("EnemyBullet.png");
    tex_PowerUp = tex_PlayerBullet; // 复用
    // 背景图：菜单/选择界面和战斗界面分别使用不同背景。
    tex_BackgroundMenu = IMG_LoadTexture(cur_Renderer, "TitleBg.png");
    tex_BackgroundBattle = IMG_LoadTexture(cur_Renderer, "BattleBg.png");

    // 字体用于所有中文 UI 文本，路径相对于运行目录。
    font = TTF_OpenFont("assets\\fonts\\SimHei.ttf", 24);
    if (!font) std::cout << "Font load failed!" << std::endl;

    // 背景音乐使用 Mix_Music，适合循环播放。
    bgm_Menu = Mix_LoadMUS("assets\\audios\\Dream Battle.mp3");
    bgm_Battle = Mix_LoadMUS("assets\\audios\\Necro-Fantasy.mp3");
    if (!bgm_Menu) std::cout << "BGM Load Failed!" << std::endl;

    // 短音效使用 Mix_Chunk。播放时统一通过 PlaySfx 做空指针保护。
    se_Shoot = Mix_LoadWAV("assets\\sfx\\player_shoot.wav");
	se_EnemyShoot1 = Mix_LoadWAV("assets\\sfx\\enemy_shoot1.wav");
    se_EnemyShoot2 = Mix_LoadWAV("assets\\sfx\\enemy_shoot2.wav");
    se_Dead = Mix_LoadWAV("assets\\sfx\\playerdead.wav");
    se_Victory = Mix_LoadWAV("assets\\sfx\\victory.ogg");
	se_PowerUp = Mix_LoadWAV("assets\\sfx\\power_up.wav");
    se_Select= Mix_LoadWAV("assets\\sfx\\select.wav");
    se_BeHitted = Mix_LoadWAV("assets\\sfx\\playerbehitted.wav");
    se_REIMUBomb = Mix_LoadWAV("assets\\sfx\\reimubomb.wav");
	se_MARISABomb = Mix_LoadWAV("assets\\sfx\\marisabomb.wav");
    if (!se_Shoot || !se_EnemyShoot1 || !se_EnemyShoot2 || !se_Dead || !se_Victory || !se_PowerUp) {
        std::cout << "SFX Load Failed!" << std::endl;
	}

    // 初始化游戏状态和音量，并启动主菜单 BGM。
    is_Running = true;
    lastTime = SDL_GetTicks();
    CurrentState = State::MAIN_MENU;
    menuCursor = 0;
    pauseMenuSelect = 0;
    Mix_VolumeMusic(bgmVolume);      // 同步背景音乐音量
    Mix_Volume(-1, sfxVolume);       // 同步所有音效频道的音量 (-1表示所有频道)
    if (bgm_Menu) Mix_PlayMusic(bgm_Menu, -1);
    return true;
}

void Game::ResetRunState()
{
    // 重置单局战斗相关状态。新开局和重新进入战斗时都应从干净状态开始。
    currentPhaseIndex = 0;
    shootTimer = 0.0f;
    enemyShootTimer = 0.5f;
    powerUpSpawnTimer = 1.0f + (rand() % 200) / 100.0f;
    continueTimer = ContinueSeconds;
    spellTimer = 0.0f;
    isSpellActive = false;
    spellUser = selectedCharID;
    shakeTime = 0.0f;
}

void Game::ClearBattleObjects()
{
    // 释放当前战斗中的所有实体；unique_ptr 会在 clear/reset 时自动 delete。
    player.reset();
    Enemies.clear();
    playerBullets.clear();
    enemyBullets.clear();
    powerUps.clear();
    items.clear();
}

void Game::ClearEnemyBullets()
{
    // 阶段切换、Miss 消弹等场景只需要清理敌弹，不影响玩家子弹和道具。
    enemyBullets.clear();
}

void Game::PlaySfx(Mix_Chunk* chunk)
{
    // 音效资源可能加载失败，集中判空能避免每个调用点重复检查。
    if (chunk) {
        Mix_PlayChannel(-1, chunk, 0);
    }
}

void Game::DestroyTexture(SDL_Texture*& texture)
{
    // SDL_Texture 需要在销毁 renderer 前释放；释放后置空防止重复释放。
    if (texture) {
        SDL_DestroyTexture(texture);
        texture = nullptr;
    }
}

void Game::FreeChunk(Mix_Chunk*& chunk)
{
    // Mix_Chunk 是短音效资源，对应 Mix_LoadWAV。
    if (chunk) {
        Mix_FreeChunk(chunk);
        chunk = nullptr;
    }
}

void Game::FreeMusic(Mix_Music*& music)
{
    // Mix_Music 是长音乐资源，对应 Mix_LoadMUS。
    if (music) {
        Mix_FreeMusic(music);
        music = nullptr;
    }
}

void Game::InitBattle(CharacterID playerID)
{
    // 开始新战斗前先清理上一局遗留对象，避免子弹、P 点或 Boss 残留。
    ClearBattleObjects();
    selectedCharID = playerID;
    pauseMenuSelect = 0;

    if (tex_PlayerReimu) SDL_SetTextureColorMod(tex_PlayerReimu, 255, 255, 255);
    if (tex_PlayerMarisa) SDL_SetTextureColorMod(tex_PlayerMarisa, 255, 255, 255);

    // 根据玩家选择创建玩家和对手 Boss：灵梦对魔理沙，魔理沙对灵梦。
    if (playerID == CharacterID::REIMU) {
        // 玩家出生在屏幕下方中央。
        player = std::make_unique<Player>(960.0f, 900.0f, tex_PlayerReimu);
        // Boss 出生在屏幕上方中央。
        Enemies.push_back(std::make_unique<Enemy>(960.0f, 200.0f, tex_Enemy_Marisa));
    }
    else {
        player = std::make_unique<Player>(960.0f, 900.0f, tex_PlayerMarisa);
        Enemies.push_back(std::make_unique<Enemy>(960.0f, 200.0f, tex_Enemy_Reimu));
    }

    SetupDialogue(playerID);
    SetupEnemyPhases(playerID);
    ResetRunState();
}

void Game::SetupDialogue(CharacterID playerID) {
    // 开场对话会进入 DIALOGUE 状态；玩家按 Z 读完后回到 PLAYING。
    DialoueQueue.clear();
    cur_index = 0;
    CurrentState = State::DIALOGUE;
    stateBeforeDialogue = State::PLAYING;

    if (playerID == CharacterID::REIMU)
    {
        DialoueQueue.push_back({ "Reimu", "魔理沙，快停止这场 DDOS 攻击！", {255,100,100,255} });
        DialoueQueue.push_back({ "Marisa", "嘿嘿，我的僵尸网络可是无懈可击的，DAZE！", {255,255,100,255} });
    }
    else if (playerID == CharacterID::MARISA)
    {
        DialoueQueue.push_back({ "Marisa", "灵梦！你的防火墙太脆弱了！", {255,255,100,255} });
        DialoueQueue.push_back({ "Reimu", "那就由我来给你打个补丁吧！", {255,100,100,255} });
    }
}
// 配置 Boss 血量阶段。阶段触发后会进入剧情对话，并在 CheckEnemyPhase 中清空敌弹。
void Game::SetupEnemyPhases(CharacterID playerID) {
    enemyPhases.clear();
    std::string bossName = (playerID == CharacterID::REIMU) ? "Marisa" : "Reimu";
    std::string SystemName = (playerID == CharacterID::MARISA) ? "Marisa" : "Reimu";

    // 阶段 1：血量降到 4000 以下，切换为锁链弹幕。
    EnemyPhase p1;
    p1.hpThreshold = 4000;
    p1.dialogueTriggered = false;
    p1.dialogues.push_back({ SystemName, "警告：检测到防火墙被突破！", {200,200,200,255} });
    p1.dialogues.push_back({ bossName, "部署加密锁，把你的数据都锁死吧！", {255,50,50,255} });
    enemyPhases.push_back(p1);

    // 阶段 2：血量降到 2000 以下，切换为高速手里剑弹幕。
    EnemyPhase p2;
    p2.hpThreshold = 2000;
    p2.dialogueTriggered = false;
    p2.dialogues.push_back({ SystemName, "严重错误：内核异常！", {255,0,0,255} });
    p2.dialogues.push_back({ bossName, "格式化所有分区！全！部！删！除！", {255,0,0,255} });
    enemyPhases.push_back(p2);
}

// 检查 Boss 是否进入下一个血量阶段；触发阶段时暂停战斗并清屏敌弹。
void Game::CheckEnemyPhase() {
    if (Enemies.empty()) return;
    float hp = Enemies[0]->GetBlood();

    // 从 currentPhaseIndex 开始检查，避免已触发阶段被重复处理。
    for (int i = currentPhaseIndex; i < enemyPhases.size(); ++i) {
        if (hp <= enemyPhases[i].hpThreshold && !enemyPhases[i].dialogueTriggered) {

            // currentPhaseIndex 与 Boss AI 分支对应：0 初始，1 锁链阶段，2 暴走阶段。
            currentPhaseIndex = i + 1;

            enemyPhases[i].dialogueTriggered = true;

            // 阶段对话结束后会恢复到触发前的游戏状态。
            DialoueQueue = enemyPhases[i].dialogues;
            cur_index = 0;
            stateBeforeDialogue = CurrentState;
            CurrentState = State::DIALOGUE;

            // 阶段切换时消除旧弹幕，避免玩家在对话结束后立即被残留敌弹命中。
            ClearEnemyBullets();

            std::cout << "Phase Triggered! Bullets Cleared." << std::endl;
            break;
        }
    }
}

// 主循环根据 SDL_GetTicks 计算 deltaTime，并限制最大步长，避免卡顿后逻辑跳跃。
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

// 这里只处理 SDL 窗口事件；按键输入在 Update 中按当前状态读取。
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
                PlaySfx(se_Select);
                keyLock = true;
            }
            if (key[SDL_SCANCODE_DOWN]) {
                if (menuSelect < 3) menuSelect++; // 主菜单共有 4 个选项：开始、音量、帮助、退出。
                PlaySfx(se_Select);
                keyLock = true;
            }
            if (key[SDL_SCANCODE_Z]) {
                if (menuSelect == 0) {
                    CurrentState = State::SELECT_CHARACTER;
                    PlaySfx(se_Select);
                }
                else if (menuSelect == 1) {
                    CurrentState = State::VOLUME_SETTINGS;
                    PlaySfx(se_Select);
                }
                else if (menuSelect == 2) {
                    CurrentState = State::HELP;
                    PlaySfx(se_Select);
                }
                else if (menuSelect == 3) {
                    is_Running = false;
                }
                keyLock = true;
            }
            
        }

        // 当所有按键都松开时，解锁
        if (!key[SDL_SCANCODE_UP] && !key[SDL_SCANCODE_DOWN] && !key[SDL_SCANCODE_Z]) {
            keyLock = false;
        }
        break;
    }

    case State::HELP:
    {
        if (!keyLock && (key[SDL_SCANCODE_Z] || key[SDL_SCANCODE_ESCAPE])) {
            CurrentState = State::MAIN_MENU;
            PlaySfx(se_Select);
            keyLock = true;
        }

        if (!key[SDL_SCANCODE_Z] && !key[SDL_SCANCODE_ESCAPE]) {
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
                PlaySfx(se_Select);
                keyLock = true;
            }
            else if (key[SDL_SCANCODE_DOWN]) {
                if (volMenuSelect < 2) volMenuSelect++;
                PlaySfx(se_Select);
                keyLock = true;
            }

            // 2. 左右键调节数值
            if (volMenuSelect == 0) { // 调节 BGM
                if (key[SDL_SCANCODE_LEFT] && bgmVolume > 0) {
                    bgmVolume -= 4;
                    Mix_VolumeMusic(bgmVolume);
                    PlaySfx(se_Select);
                    keyLock = true;
                }
                if (key[SDL_SCANCODE_RIGHT] && bgmVolume < MaxVolume) {
                    bgmVolume += 4;
                    Mix_VolumeMusic(bgmVolume);
                    PlaySfx(se_Select);
                    keyLock = true;
                }
            }
            else if (volMenuSelect == 1) { // 调节音效
                if (key[SDL_SCANCODE_LEFT] && sfxVolume > 0) {
                    sfxVolume -= 4;
                    // Mix_Volume(-1, vol) 调节所有音效频道
                    Mix_Volume(-1, sfxVolume);
                    PlaySfx(se_Select);
                    keyLock = true;
                }
                if (key[SDL_SCANCODE_RIGHT] && sfxVolume < MaxVolume) {
                    sfxVolume += 4;
                    Mix_Volume(-1, sfxVolume);
                    PlaySfx(se_Select);
                    keyLock = true;
                }
            }

            // 3. 确认返回
            if (key[SDL_SCANCODE_Z] || key[SDL_SCANCODE_ESCAPE]) {
                if (volMenuSelect == 2 || key[SDL_SCANCODE_ESCAPE]) {
                    PlaySfx(se_Select);
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
            if (key[SDL_SCANCODE_LEFT]) {
                menuCursor = 0;
                PlaySfx(se_Select);
            }
            if (key[SDL_SCANCODE_RIGHT]) {
                menuCursor = 1;
                PlaySfx(se_Select);
            }
            if (key[SDL_SCANCODE_ESCAPE]) CurrentState = State::MAIN_MENU;
            if (key[SDL_SCANCODE_Z]) {
                PlaySfx(se_Select);
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
            PlaySfx(se_Select);
            if (cur_index >= DialoueQueue.size()) CurrentState = stateBeforeDialogue;
            zPressed = true;
        }
        if (!key[SDL_SCANCODE_Z]) zPressed = false;
        break;
    }

    case State::PLAYING:
    {
        static bool escPressed = false;
        if (key[SDL_SCANCODE_ESCAPE]) {
            if (!escPressed) {
                CurrentState = State::PAUSED;
                pauseMenuSelect = 0;
                PlaySfx(se_Select);
                escPressed = true;
            }
            break;
        }
        escPressed = false;

        if (player) player->Update(DeltaTime);

        // 玩家射击：按住 Z 时按冷却间隔连续发弹，火力等级会增加侧向子弹。
        if (key[SDL_SCANCODE_Z] && shootTimer <= 0.0f) {
            PlaySfx(se_Shoot);
            BulletType bType = (selectedCharID == CharacterID::REIMU) ? BulletType::PLAYER_TALISMAN : BulletType::PLAYER_STAR;
            const Vector2D& playerPosition = player->GetPosition();
            playerBullets.push_back(std::make_unique<Bullet>(playerPosition.x, playerPosition.y, 0.0f, -600.0f, bType));
            if (player->GetPowerLevel() >= 1) {
                playerBullets.push_back(std::make_unique<Bullet>(playerPosition.x - 15.0f, playerPosition.y, -100.0f, -600.0f, bType));
                playerBullets.push_back(std::make_unique<Bullet>(playerPosition.x + 15.0f, playerPosition.y, 100.0f, -600.0f, bType));
            }
            shootTimer = 0.07f;
        }
        shootTimer -= DeltaTime;

        // 生成 P 点：每隔一段随机时间从屏幕上方掉落一批火力道具。
        // 同时有一定概率生成 BombItem 或 LifeItem（体现多态道具体系）
        powerUpSpawnTimer -= DeltaTime;
        if (powerUpSpawnTimer <= 0) {
            // 每批生成 3 到 6 个 P 点。
            int batchCount = 3 + rand() % 4;

            // 批次中心限制在 200 到 1719，避免道具过于靠近屏幕边缘。
            float batchCenterX = 200.0f + rand() % 1520;

            for (int i = 0; i < batchCount; i++) {

                // 给同批 P 点少量偏移，避免生成位置完全重叠。
                float offsetX = (float)(rand() % 60 - 30); // -30 到 30 的偏移
                float offsetY = (float)(rand() % 60 - 30); // -30 到 30 的偏移

                // 30% 概率生成新类型道具（Bomb/Life），70% 仍为 P 点
                int roll = rand() % 10;
                if (roll < 2 && items.size() < 30) {
                    // 20% 概率生成残机道具（最稀有）
                    items.push_back(std::make_unique<LifeItem>(batchCenterX + offsetX, -50.0f + offsetY, tex_PowerUp));
                }
                else if (roll < 5 && items.size() < 30) {
                    // 30% 概率生成 Bomb 道具
                    items.push_back(std::make_unique<BombItem>(batchCenterX + offsetX, -50.0f + offsetY, tex_PowerUp));
                }
                else {
                    // 其余 50% 仍为传统 P 点
                    powerUps.push_back(std::make_unique<PowerUp>(batchCenterX + offsetX, -50.0f + offsetY, tex_PowerUp));
                }
            }

            // 下一批掉落间隔为 4.0 到 5.9 秒。
            powerUpSpawnTimer = 4.0f + (rand() % 20) / 10.0f;
        }

        // 处理 Bomb/符卡触发。xPressed 防止长按 X 时重复消耗 Bomb。
        static bool xPressed = false;
        if (key[SDL_SCANCODE_X] && !xPressed && player->CanUseBomb() && !isSpellActive) {
            player->ConsumeBomb();
            isSpellActive = true;
            spellTimer = 3.0f; // 符卡持续3秒
            spellUser = selectedCharID;

            player->SetInvincible(3.5f); // 无敌略长于符卡

            if (se_REIMUBomb && spellUser == CharacterID::REIMU) PlaySfx(se_REIMUBomb);
            if (se_MARISABomb && spellUser == CharacterID::MARISA) PlaySfx(se_MARISABomb);
            xPressed = true;
        }
        if (!key[SDL_SCANCODE_X]) xPressed = false;
        if (!key[SDL_SCANCODE_X]) xPressed = false;

        if (isSpellActive) {
            spellTimer -= DeltaTime;

            // 持续消弹：符卡期间任何接触到特效区域的子弹都会消失
            for (auto& b : enemyBullets) {
                if (spellUser == CharacterID::MARISA) {
                    // 魔理沙：激光范围内的子弹消失
                    if (abs(b->GetPosition().x - player->GetPosition().x) < 100) b->Deactivate();
                }
                else {
                    // 灵梦：全屏缓慢消弹
                    b->Deactivate();
                }
            }

            // 持续伤害
            for (auto& e : Enemies) {
                if (e->IsActive()) e->hit(10); // 每帧造成小额伤害，累计很高
            }

            if (spellTimer <= 0) isSpellActive = false;
        }

        // Boss 弹幕 AI：根据 currentPhaseIndex 选择攻击模式。
        enemyShootTimer -= DeltaTime;
        static float angleOffset = 0; // 用于让弹幕旋转起来
        static float enemySeCooldown = 0.0f;
        enemySeCooldown -= DeltaTime;
       
        if (!Enemies.empty() && Enemies[0]->IsActive()) {
            if (enemyShootTimer <= 0) {
                // currentPhaseIndex：0 初始圆环弹，1 锁链追踪弹，2 低血量高速螺旋弹。
                if (enemySeCooldown <= 0) {
                    if (currentPhaseIndex >= 2 || Enemies[0]->GetBlood() < 2000) {
                        // 播放阶段 2 专属的高频/暴走音效
                        PlaySfx(se_EnemyShoot2);
                        enemySeCooldown = 0.06f; // 阶段 2 射速极快，冷却缩短以配合节奏
                    }
                    else {
                        // 播放阶段 0 和 1 的常规攻击音效
                        PlaySfx(se_EnemyShoot1);
                        enemySeCooldown = 0.12f; // 普通阶段冷却稍长，听感更清晰
                    }
                }

                // 阶段 0：病毒圆环弹，0.5 秒一波。
                if (currentPhaseIndex == 0) {
                    const Vector2D& enemyPosition = Enemies[0]->GetPosition();
                    bp.ShootRotatingRing(enemyPosition.x, enemyPosition.y, 300, 36, angleOffset, enemyBullets, BulletType::VIRUS);

                    angleOffset += 0.1f; // 缓慢旋转
                    enemyShootTimer = 0.5f;

                }
                // 阶段 1：锁链弹先展开再瞄准玩家，间隔较长。
                else if (currentPhaseIndex == 1) {
                    const Vector2D& enemyPosition = Enemies[0]->GetPosition();
                    bp.ShootStopAndGo(enemyPosition.x, enemyPosition.y, 1.0f, 600, player.get(), 24, enemyBullets, BulletType::LOCK);

                    angleOffset -= 0.15f; // 反向旋转
                    enemyShootTimer = 0.8f; // 这种强力弹幕间隔长一点

                }
                // 阶段 2：高速手里剑螺旋弹，0.05 秒一轮。
                else if (currentPhaseIndex >= 2 || Enemies[0]->GetBlood() < 2000) {
                    angleOffset += 0.5f;

                    const Vector2D& enemyPosition = Enemies[0]->GetPosition();
                    bp.ShootSpiral(enemyPosition.x, enemyPosition.y, 500, angleOffset, 5, 0.2f, enemyBullets, BulletType::SHURIKEN);

                    enemyShootTimer = 0.05f;
                }
            }
        }

        // 更新所有对象
        for (auto& b : playerBullets) b->Update(DeltaTime);
        for (auto& b : enemyBullets) b->Update(DeltaTime);
        for (auto& p : powerUps) p->Update(DeltaTime);

        // 更新所有新类型道具（Item 多态体系）
        for (auto& item : items) item->Update(DeltaTime);

        // 碰撞：子弹打Boss
        for (auto& b : playerBullets) {
            if (!b->IsActive()) continue;
            for (auto& e : Enemies) {
                if (e->IsActive() && b->CheckCollision(e.get())) {
                    e->hit(player->GetAttackPoint());
                    b->Deactivate();
                }
            }
        }

        // 碰撞：子弹打玩家 (已适配东方残机与无敌机制)
        for (auto& b : enemyBullets) {
            // 只有在子弹激活、玩家存在，且玩家当前【不在】无敌状态时才检测碰撞
            if (b->IsActive() && player && !player->IsInvincible() && player->CheckCollision(b.get())) {

                b->Deactivate();      // 销毁击中玩家的子弹
                player->hit(1.0f);    // 减少一个残机 (原作机制：Miss)
  

                if (player->Dead_judge()) {
                    // 残机小于 0，彻底游戏结束
                    isSpellActive = false;
                    spellTimer = 0.0f;
                    shakeTime = 0.0f;
                    Mix_HaltMusic();    // 停止背景音乐
                    continueTimer = ContinueSeconds;
                    PlaySfx(se_Dead);
                    CurrentState = State::GAME_OVER;
                }
                else {
                    PlaySfx(se_BeHitted);
                    // Miss 后清空敌弹，给玩家复位后的无敌时间一个稳定起点。
                    for (auto& eb : enemyBullets) {
                        eb->Deactivate();
                    }
                    player->ResetPosition();
                }
            }
        }

        // 碰撞：拾取 P 点。
        for (auto& p : powerUps) {

            // 使用矩形拾取范围，而不是玩家真实判定点；这样贴图边缘接触也能拾取。
            float pickUpWidth = 60.0f;
            float pickUpHeight = 80.0f;

            if (!p->IsActive() || !player) continue;

            float diffX = std::abs(player->GetPosition().x - p->GetPosition().x);
            float diffY = std::abs(player->GetPosition().y - p->GetPosition().y);

            // 只检查玩家中心与 P 点中心的距离是否落在拾取矩形内。
            bool isColliding = (diffX < pickUpWidth / 2) && (diffY < pickUpHeight / 2);

       
            if (p->IsActive() && player && isColliding) {
                if (player->CollectPowerUp()==1) {
                    PlaySfx(se_PowerUp);
                }
                p->Deactivate();
            }
        }

        // 碰撞：拾取新类型道具（Item 多态体系）。
        // Game 只持有 Item 基类指针，调用 Apply() 时由多态分发到具体道具实现。
        // 新增道具类型时，此处代码无需修改——符合开闭原则。
        for (auto& item : items) {
            if (!item->IsActive() || !player) continue;

            float diffX = std::abs(player->GetPosition().x - item->GetPosition().x);
            float diffY = std::abs(player->GetPosition().y - item->GetPosition().y);

            float pickUpWidth = 60.0f;
            float pickUpHeight = 80.0f;
            bool isColliding = (diffX < pickUpWidth / 2) && (diffY < pickUpHeight / 2);

            if (isColliding) {
                // 多态调用：Game 不关心具体道具类型，Item 自己知道效果
                item->Apply(*player);
                PlaySfx(se_PowerUp);  // 复用拾取音效
                item->Deactivate();
            }
        }

        // 清理失效对象
        RemoveInactiveObjects(playerBullets);
        RemoveInactiveObjects(enemyBullets);
        RemoveInactiveObjects(powerUps);
        RemoveInactiveObjects(items);

        // 对象清理后再检查阶段，避免阶段切换对话中残留已失效实体。
        CheckEnemyPhase();

        // 胜利判定
        if (!Enemies.empty() && Enemies[0]->GetBlood() <= 0) {
            // Boss 击杀时掉落一批高品质道具（Bomb + Life）
            const Vector2D& bossPos = Enemies[0]->GetPosition();
            for (int i = 0; i < 3; i++) {
                float offsetX = (float)(rand() % 80 - 40);
                items.push_back(std::make_unique<BombItem>(bossPos.x + offsetX, bossPos.y + (float)(rand() % 40), tex_PowerUp));
            }
            // 必定掉落 1 个残机道具作为击杀奖励
            items.push_back(std::make_unique<LifeItem>(bossPos.x, bossPos.y, tex_PowerUp));

            PlaySfx(se_Victory); // 胜利音效
            Mix_HaltMusic(); // 停止战斗BGM
            CurrentState = State::VICTORY;
            isSpellActive = false;
            spellTimer = 0.0f;
            shakeTime = 0.0f;
        }
        break;
    }

    case State::PAUSED:
    {
        if (!keyLock) {
            if (key[SDL_SCANCODE_UP]) {
                if (pauseMenuSelect > 0) pauseMenuSelect--;
                PlaySfx(se_Select);
                keyLock = true;
            }
            else if (key[SDL_SCANCODE_DOWN]) {
                if (pauseMenuSelect < 1) pauseMenuSelect++;
                PlaySfx(se_Select);
                keyLock = true;
            }
            else if (key[SDL_SCANCODE_Z]) {
                if (pauseMenuSelect == 0) {
                    CurrentState = State::PLAYING;
                }
                else {
                    CurrentState = State::MAIN_MENU;
                    menuSelect = 0;
                    continueTimer = ContinueSeconds;
                    ClearBattleObjects();
                    if (bgm_Menu) {
                        Mix_HaltMusic();
                        Mix_PlayMusic(bgm_Menu, -1);
                    }
                }
                PlaySfx(se_Select);
                keyLock = true;
            }
            else if (key[SDL_SCANCODE_ESCAPE]) {
                CurrentState = State::PLAYING;
                PlaySfx(se_Select);
                keyLock = true;
            }
        }

        if (!key[SDL_SCANCODE_UP] && !key[SDL_SCANCODE_DOWN] &&
            !key[SDL_SCANCODE_Z] && !key[SDL_SCANCODE_ESCAPE]) {
            keyLock = false;
        }
        break;
    }

    case State::GAME_OVER:
    case State::VICTORY:
    {
        static bool escPressed = false;
        static bool rPressed = false;

        if (key[SDL_SCANCODE_ESCAPE]) {
            if (!escPressed) {
                CurrentState = State::MAIN_MENU;
                menuSelect = 0;
                continueTimer = ContinueSeconds;
                ClearBattleObjects();
                if (bgm_Menu) {
                    Mix_HaltMusic();
                    Mix_PlayMusic(bgm_Menu, -1);
                }
                escPressed = true;
            }
        }
        else {
            escPressed = false; // 抬起按键时重置
        }

        if (CurrentState == State::GAME_OVER) {
            // --- 1. 倒计时逻辑 ---
            continueTimer -= DeltaTime;
            if (continueTimer <= 0) {
                // 时间到，强制回到主菜单
                CurrentState = State::MAIN_MENU;
                if (bgm_Menu) {
                    Mix_HaltMusic();
                    Mix_PlayMusic(bgm_Menu, -1);
                }
                continueTimer = ContinueSeconds; // 重置计时器供下次使用
                ClearBattleObjects();
                break;
            }
            // --- 2. 续关 (按 R) ---
            if (key[SDL_SCANCODE_R]) {
                if (!rPressed) {
                    // 续关重置：恢复残机，但清空火力作为惩罚。
                    player->ResetForContinue();  // 恢复残机、清空火力并回到初始点。
                    continueTimer = ContinueSeconds;

                    // 清理屏幕弹幕，给玩家喘息机会
                    for (auto& b : enemyBullets) b->Deactivate();

                    // 恢复战斗音乐
                    if (bgm_Battle) {
                        Mix_HaltMusic();
                        Mix_PlayMusic(bgm_Battle, -1);
                    }

                    CurrentState = State::PLAYING;
                    rPressed = true;
                }
            }
            else {
                rPressed = false; // 抬起按键时重置
            }
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
    SDL_Rect bgRect = { 0, 0, 1920, 1080 };
    if (CurrentState == State::MAIN_MENU ||
        CurrentState == State::VOLUME_SETTINGS ||
        CurrentState == State::HELP ||
        CurrentState == State::SELECT_CHARACTER)
    {
        if (tex_BackgroundMenu) {
            SDL_RenderCopy(cur_Renderer, tex_BackgroundMenu, NULL, &bgRect);
        }
    }
    // 战斗、对话、结算界面使用战斗背景。
    else
    {
        if (tex_BackgroundBattle) {
            SDL_RenderCopy(cur_Renderer, tex_BackgroundBattle, NULL, &bgRect);
        }
    }
    if (CurrentState == State::MAIN_MENU) {
        if (font) {
          
            // --- [1. 绘制游戏大标题] ---
            SDL_Color white = { 255, 255, 255, 255 };
            SDL_Surface* titleSurf = TTF_RenderUTF8_Blended(font,"东方代码乡",white);
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

            // --- [4. 绘制选项 2：帮助界面] ---
            SDL_Color colorHelp = (menuSelect == 2) ? SDL_Color{ 255, 255, 0, 255 } : SDL_Color{ 150, 150, 150, 255 };
            SDL_Surface* sHelp = TTF_RenderUTF8_Blended(font, "帮助说明 (HELP)", colorHelp);
            if (sHelp) {
                SDL_Texture* tHelp = SDL_CreateTextureFromSurface(cur_Renderer, sHelp);
                SDL_Rect rHelp = { (1920 - sHelp->w) / 2, 750, sHelp->w, sHelp->h };
                SDL_RenderCopy(cur_Renderer, tHelp, NULL, &rHelp);

                if (menuSelect == 2) {
                    SDL_SetRenderDrawColor(cur_Renderer, 255, 255, 0, 255);
                    SDL_Rect pointer = { rHelp.x - 50, rHelp.y + 5, 30, 30 };
                    SDL_RenderFillRect(cur_Renderer, &pointer);
                }
                SDL_FreeSurface(sHelp); SDL_DestroyTexture(tHelp);
            }

            // --- [5. 绘制选项 3：退出游戏] ---
            SDL_Color colorQuit = (menuSelect == 3) ? SDL_Color{ 255, 255, 0, 255 } : SDL_Color{ 150, 150, 150, 255 };
            SDL_Surface* s2 = TTF_RenderUTF8_Blended(font, "断开连接 (QUIT)", colorQuit);
            if (s2) {
                SDL_Texture* t2 = SDL_CreateTextureFromSurface(cur_Renderer, s2);
                SDL_Rect r2 = { (1920 - s2->w) / 2, 850, s2->w, s2->h };
                SDL_RenderCopy(cur_Renderer, t2, NULL, &r2);

                if (menuSelect == 3) {
                    SDL_SetRenderDrawColor(cur_Renderer, 255, 255, 0, 255);
                    SDL_Rect pointer = { r2.x - 50, r2.y + 5, 30, 30 };
                    SDL_RenderFillRect(cur_Renderer, &pointer);
                }
                SDL_FreeSurface(s2); SDL_DestroyTexture(t2);
            }

            // --- [6. 绘制页脚提示] ---
            SDL_Surface* hint = TTF_RenderUTF8_Blended(font,"使用方向键切换，Z 键确认",{ 100, 100, 100, 255 });
            if (hint) {
                SDL_Texture* hTex = SDL_CreateTextureFromSurface(cur_Renderer, hint);
                SDL_Rect hRect = { (1920 - hint->w) / 2, 950, hint->w, hint->h };
                SDL_RenderCopy(cur_Renderer, hTex, NULL, &hRect);
                SDL_FreeSurface(hint); SDL_DestroyTexture(hTex);
            }
        }
    }

    if (CurrentState == State::HELP) {
        if (font) {
            SDL_SetRenderDrawBlendMode(cur_Renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(cur_Renderer, 0, 0, 0, 180);
            SDL_Rect fullScreen = { 0, 0, 1920, 1080 };
            SDL_RenderFillRect(cur_Renderer, &fullScreen);

            SDL_Color titleColor = { 255, 255, 255, 255 };
            SDL_Surface* titleSurf = TTF_RenderUTF8_Blended(font, "帮助与操作说明", titleColor);
            if (titleSurf) {
                SDL_Texture* titleTex = SDL_CreateTextureFromSurface(cur_Renderer, titleSurf);
                SDL_Rect titleRect = { (1920 - titleSurf->w) / 2, 180, titleSurf->w, titleSurf->h };
                SDL_RenderCopy(cur_Renderer, titleTex, NULL, &titleRect);
                SDL_FreeSurface(titleSurf);
                SDL_DestroyTexture(titleTex);
            }

            const char* helpLines[] = {
                "方向键：移动主菜单光标与操作角色",
                "Z 键：确认、跳过对话、持续射击",
                "X 键：释放符卡 / Bomb",
                "左 Shift：低速移动并显示判定点",
                "ESC：战斗中暂停，暂停页可继续或返回主菜单",
                "R 键：仅在 Game Over 后续关"
            };

            for (int i = 0; i < 6; ++i) {
                SDL_Surface* lineSurf = TTF_RenderUTF8_Blended(font, helpLines[i], { 220, 220, 220, 255 });
                if (lineSurf) {
                    SDL_Texture* lineTex = SDL_CreateTextureFromSurface(cur_Renderer, lineSurf);
                    SDL_Rect lineRect = { 420, 320 + i * 80, lineSurf->w, lineSurf->h };
                    SDL_RenderCopy(cur_Renderer, lineTex, NULL, &lineRect);
                    SDL_FreeSurface(lineSurf);
                    SDL_DestroyTexture(lineTex);
                }
            }

            SDL_Surface* hintSurf = TTF_RenderUTF8_Blended(font, "按 Z 或 ESC 返回主菜单", { 100, 255, 255, 255 });
            if (hintSurf) {
                SDL_Texture* hintTex = SDL_CreateTextureFromSurface(cur_Renderer, hintSurf);
                SDL_Rect hintRect = { (1920 - hintSurf->w) / 2, 900, hintSurf->w, hintSurf->h };
                SDL_RenderCopy(cur_Renderer, hintTex, NULL, &hintRect);
                SDL_FreeSurface(hintSurf);
                SDL_DestroyTexture(hintTex);
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
    // 状态 B: 游戏进行中或对话中（包含时停逻辑）
    // ============================================================
    else if (CurrentState == State::PLAYING || CurrentState == State::DIALOGUE || CurrentState == State::PAUSED) {

        // 1. 绘制游戏层
        if (player) player->Render(cur_Renderer);
        for (auto& e : Enemies) e->Render(cur_Renderer);
        // 子弹
        for (auto& b : playerBullets) if (b->IsActive()) b->Render(cur_Renderer, tex_PlayerBullet);
        for (auto& b : enemyBullets) if (b->IsActive()) b->Render(cur_Renderer, tex_EnemyBullet);
        // 道具
        for (auto& p : powerUps) if (p->IsActive()) p->Render(cur_Renderer);
        // 渲染新类型道具（Item 多态体系）
        for (auto& item : items) if (item->IsActive()) item->Render(cur_Renderer);
        // 符卡演出层。
        if (isSpellActive) {
            // --- [1. 背景变暗] ---
            SDL_SetRenderDrawBlendMode(cur_Renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(cur_Renderer, 0, 0, 0, 150);
            SDL_Rect full = { 0, 0, 1920, 1080 };
            SDL_RenderFillRect(cur_Renderer, &full);

            // --- [2. 特效绘制] ---
            if (spellUser == CharacterID::REIMU) {
                // 灵梦：灵符「梦想封印」
                // 绘制 8 个巨大的彩色光玉环绕
                for (int i = 0; i < 8; i++) {
                    float angle = (spellTimer * 4.0f) + (i * 0.785f);
                    float dist = 300.0f - (spellTimer * 50.0f); // 逐渐向中心收缩
                    const Vector2D& playerPosition = player->GetPosition();
                    int tx = static_cast<int>(playerPosition.x + std::cos(angle) * dist);
                    int ty = static_cast<int>(playerPosition.y + std::sin(angle) * dist);

                    SDL_SetRenderDrawColor(cur_Renderer, 255, 100, 200, 180);
                    SDL_Rect orb = { tx - 60, ty - 60, 120, 120 };
                    SDL_RenderFillRect(cur_Renderer, &orb); // 当前用矩形占位表现光玉效果。
                }
            }
            else if (spellUser == CharacterID::MARISA) {
                // 魔理沙：恋符「Master Spark」
                SDL_SetRenderDrawBlendMode(cur_Renderer, SDL_BLENDMODE_ADD); // 加法混合，超亮！
                int offsetX = (shakeTime > 0) ? (rand() % 10 - 5) : 0;
                const Vector2D& playerPosition = player->GetPosition();

                // 激光外壳
                SDL_SetRenderDrawColor(cur_Renderer, 100, 100, 255, 200);
                SDL_Rect beamAura = { (int)playerPosition.x - 120 + offsetX, 0, 240, (int)playerPosition.y };
                SDL_RenderFillRect(cur_Renderer, &beamAura);

                // 激光核心
                SDL_SetRenderDrawColor(cur_Renderer, 255, 255, 255, 255);
                SDL_Rect beamCore = { (int)playerPosition.x - 70 + offsetX, 0, 140, (int)playerPosition.y };
                SDL_RenderFillRect(cur_Renderer, &beamCore);
                SDL_SetRenderDrawBlendMode(cur_Renderer, SDL_BLENDMODE_BLEND);
            }

            // --- [3. 符卡名称宣示] ---
            std::string sName = (spellUser == CharacterID::REIMU) ? "灵符「梦想封印」" : "恋符「Master Spark」";
            SDL_Surface* sSurf = TTF_RenderUTF8_Blended(font, sName.c_str(), { 255, 255, 255, 255 });
            if (sSurf) {
                SDL_Texture* sTex = SDL_CreateTextureFromSurface(cur_Renderer, sSurf);
                // 名称从右侧滑入
                SDL_Rect sRect = { 1800 - (int)((3.0f - spellTimer) * 400), 150, sSurf->w, sSurf->h };
                SDL_RenderCopy(cur_Renderer, sTex, NULL, &sRect);
                SDL_FreeSurface(sSurf); SDL_DestroyTexture(sTex);
            }
        }
        // 2. 绘制 UI (残机与 Power 展示)
        if (font && player) {
            // --- [参数定义] ---
            int uiX = 108;           // UI 起始 X 坐标
            int uiY = 100;           // UI 起始 Y 坐标

            // --- [1. 构建残机显示字符串] ---
            // 用星形符号直观显示剩余残机。
            std::string lifeStr = "Player: ";
            for (int i = 0; i < player->GetLives(); i++) {
                lifeStr += "★ ";
            }
            if (player->GetLives() <= 0) lifeStr += "None"; // 0残机时的提示

            // --- [2. 渲染残机文字] ---
            SDL_Color pink = { 255, 105, 180, 255 }; // 东方 UI 常用粉红色或纯白色
            SDL_Surface* sLife = TTF_RenderUTF8_Blended(font, lifeStr.c_str(), pink);
            if (sLife) {
                SDL_Texture* tLife = SDL_CreateTextureFromSurface(cur_Renderer, sLife);
                SDL_Rect rectLife = { uiX, uiY, sLife->w, sLife->h };
                SDL_RenderCopy(cur_Renderer, tLife, NULL, &rectLife);

                // 渲染完立即释放，防止内存泄漏
                SDL_FreeSurface(sLife);
                SDL_DestroyTexture(tLife);
            }

            // --- [3. 渲染 Power 数值] ---
            // 东方风格：Power [ 1.00 / 3.00 ]
            char powerBuf[64];
            float totalPower = player->GetTotalPower();
            sprintf_s(powerBuf, "Power:  %.2f / %.2f", totalPower, static_cast<float>(MaxPowerLevel));

            SDL_Surface* sPower = TTF_RenderUTF8_Blended(font, powerBuf, { 255, 255, 255, 255 });
            if (sPower) {
                SDL_Texture* tPower = SDL_CreateTextureFromSurface(cur_Renderer, sPower);
                // 将 Power 文字放在 Player 文字下方 (偏移 40 像素)
                SDL_Rect rectPower = { uiX, uiY + 40, sPower->w, sPower->h };
                SDL_RenderCopy(cur_Renderer, tPower, NULL, &rectPower);

                SDL_FreeSurface(sPower);
                SDL_DestroyTexture(tPower);
            }
            // --- [4. 渲染 Spell Card 数值] ---
            std::string bombStr = "Spell:  ";
            for (int i = 0; i < player->GetBombs(); i++) {
                bombStr += "★ "; // 用星形符号显示可用符卡数量。
            }
            SDL_Surface* sBomb = TTF_RenderUTF8_Blended(font, bombStr.c_str(), { 100, 255, 100, 255 }); // 绿色
            if (sBomb) {
                SDL_Texture* tBomb = SDL_CreateTextureFromSurface(cur_Renderer, sBomb);
                SDL_Rect rectBomb = { uiX, uiY + 80, sBomb->w, sBomb->h }; // uiY + 80 放在 Power 下面
                SDL_RenderCopy(cur_Renderer, tBomb, NULL, &rectBomb);
                SDL_FreeSurface(sBomb);
                SDL_DestroyTexture(tBomb);
            }
        }
        // 3. 绘制 Boss 血条 (顶部居中长条)
        if (!Enemies.empty() && Enemies[0]->IsActive()) {
            Enemy* boss = Enemies[0].get();

            // --- [参数定义] ---
            int screenW = 1900;
            int barW = 600;         // Boss 血条宽度（比玩家的长，显大气）
            int barH = 15;           // Boss 血条高度
            int uiX = (screenW - barW) / 2; // 水平居中
            int uiY = 10;            // 距离顶部 10 像素

            // 当前 Boss 初始血量固定为 6000，血条按该值归一化。
            float currentHp = boss->GetBlood();
            static float maxHp = 6000.0f; // 与 Enemy 构造函数中的初始血量保持一致。

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
        // 对话状态下绘制半透明遮罩和对话框，战斗逻辑暂停在 DIALOGUE 状态。
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

        if (CurrentState == State::PAUSED && font) {
            SDL_SetRenderDrawBlendMode(cur_Renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(cur_Renderer, 0, 0, 0, 170);
            SDL_Rect fullScreen = { 0, 0, 1920, 1080 };
            SDL_RenderFillRect(cur_Renderer, &fullScreen);

            SDL_Rect panel = { 660, 270, 600, 420 };
            SDL_SetRenderDrawColor(cur_Renderer, 20, 20, 60, 235);
            SDL_RenderFillRect(cur_Renderer, &panel);
            SDL_SetRenderDrawColor(cur_Renderer, 220, 220, 220, 255);
            SDL_RenderDrawRect(cur_Renderer, &panel);

            SDL_Surface* titleSurf = TTF_RenderUTF8_Blended(font, "游戏暂停", { 255, 255, 255, 255 });
            if (titleSurf) {
                SDL_Texture* titleTex = SDL_CreateTextureFromSurface(cur_Renderer, titleSurf);
                SDL_Rect titleRect = { (1920 - titleSurf->w) / 2, 340, titleSurf->w, titleSurf->h };
                SDL_RenderCopy(cur_Renderer, titleTex, NULL, &titleRect);
                SDL_FreeSurface(titleSurf);
                SDL_DestroyTexture(titleTex);
            }

            SDL_Color continueColor = (pauseMenuSelect == 0) ? SDL_Color{ 255, 255, 0, 255 } : SDL_Color{ 170, 170, 170, 255 };
            SDL_Surface* continueSurf = TTF_RenderUTF8_Blended(font, "继续游戏", continueColor);
            if (continueSurf) {
                SDL_Texture* continueTex = SDL_CreateTextureFromSurface(cur_Renderer, continueSurf);
                SDL_Rect continueRect = { (1920 - continueSurf->w) / 2, 450, continueSurf->w, continueSurf->h };
                SDL_RenderCopy(cur_Renderer, continueTex, NULL, &continueRect);
                if (pauseMenuSelect == 0) {
                    SDL_SetRenderDrawColor(cur_Renderer, 255, 255, 0, 255);
                    SDL_Rect pointer = { continueRect.x - 50, continueRect.y + 4, 24, 24 };
                    SDL_RenderFillRect(cur_Renderer, &pointer);
                }
                SDL_FreeSurface(continueSurf);
                SDL_DestroyTexture(continueTex);
            }

            SDL_Color backColor = (pauseMenuSelect == 1) ? SDL_Color{ 255, 255, 0, 255 } : SDL_Color{ 170, 170, 170, 255 };
            SDL_Surface* backSurf = TTF_RenderUTF8_Blended(font, "返回主菜单", backColor);
            if (backSurf) {
                SDL_Texture* backTex = SDL_CreateTextureFromSurface(cur_Renderer, backSurf);
                SDL_Rect backRect = { (1920 - backSurf->w) / 2, 530, backSurf->w, backSurf->h };
                SDL_RenderCopy(cur_Renderer, backTex, NULL, &backRect);
                if (pauseMenuSelect == 1) {
                    SDL_SetRenderDrawColor(cur_Renderer, 255, 255, 0, 255);
                    SDL_Rect pointer = { backRect.x - 50, backRect.y + 4, 24, 24 };
                    SDL_RenderFillRect(cur_Renderer, &pointer);
                }
                SDL_FreeSurface(backSurf);
                SDL_DestroyTexture(backTex);
            }

            SDL_Surface* hintSurf = TTF_RenderUTF8_Blended(font, "方向键选择，Z 确认，ESC 继续游戏", { 100, 255, 255, 255 });
            if (hintSurf) {
                SDL_Texture* hintTex = SDL_CreateTextureFromSurface(cur_Renderer, hintSurf);
                SDL_Rect hintRect = { (1920 - hintSurf->w) / 2, 620, hintSurf->w, hintSurf->h };
                SDL_RenderCopy(cur_Renderer, hintTex, NULL, &hintRect);
                SDL_FreeSurface(hintSurf);
                SDL_DestroyTexture(hintTex);
            }
        }
    }

    // ============================================================
    // 状态 C: 结算界面
    // ============================================================
    else if (CurrentState == State::VICTORY || CurrentState == State::GAME_OVER) {
        if (font) {
            // --- 1. 绘制半透明遮罩 (增强氛围感) ---
            SDL_SetRenderDrawBlendMode(cur_Renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(cur_Renderer, 0, 0, 0, 150); // 黑色半透明
            SDL_Rect fullScreen = { 0, 0, 1920, 1080 };
            SDL_RenderFillRect(cur_Renderer, &fullScreen);

            // --- 2. 准备主提示文字 ---
            const char* msg = (CurrentState == State::VICTORY) ? "VICTORY!" : "满身疮痍";
            SDL_Color color = (CurrentState == State::VICTORY) ? SDL_Color{ 255, 255, 0, 255 } : SDL_Color{ 255, 0, 0, 255 };

            SDL_Surface* surf = TTF_RenderUTF8_Blended(font, msg, color);
            if (surf) {
                SDL_Texture* t = SDL_CreateTextureFromSurface(cur_Renderer, surf);
                SDL_Rect dst = { (1920 - surf->w) / 2, (1080 - surf->h) / 2 - 50, surf->w, surf->h };
                SDL_RenderCopy(cur_Renderer, t, NULL, &dst);
                SDL_FreeSurface(surf); SDL_DestroyTexture(t);
            }

            // --- 3. 绘制倒计时数字（仅 GAME_OVER 状态） ---
            if (CurrentState == State::GAME_OVER) {
                char countBuf[64];
                // 使用 ceil 向上取整，让玩家看到从 10 开始倒数
                sprintf_s(countBuf, "%d 秒后返回菜单", (int)ceil(continueTimer));

                // 倒计时小于3秒变红，增加紧张感
                SDL_Color tColor = (continueTimer > 3.0f) ? SDL_Color{ 255, 255, 255, 255 } : SDL_Color{ 255, 50, 50, 255 };

                SDL_Surface* sCount = TTF_RenderUTF8_Blended(font, countBuf, tColor);
                if (sCount) {
                    SDL_Texture* tCount = SDL_CreateTextureFromSurface(cur_Renderer, sCount);
                    SDL_Rect rCount = { (1920 - sCount->w) / 2, (1080 - sCount->h) / 2, sCount->w, sCount->h };
                    SDL_RenderCopy(cur_Renderer, tCount, NULL, &rCount);
                    SDL_FreeSurface(sCount); SDL_DestroyTexture(tCount);
                }
            }

            // --- 4. 准备交互提示文字 (续关与退出) ---
            // 只有在 GAME_OVER 时才显示“按R续关”
            std::string hintStr = (CurrentState == State::GAME_OVER) ?
                "按下 R 键不屈续关 / 按下 ESC 返回主菜单" :
                "按下 ESC 返回主菜单";

            SDL_Surface* hintSurf = TTF_RenderUTF8_Blended(font, hintStr.c_str(), { 255, 255, 255, 255 });
            if (hintSurf) {
                SDL_Texture* ht = SDL_CreateTextureFromSurface(cur_Renderer, hintSurf);
                // 放在主标题下方 100 像素的位置
                SDL_Rect hDst = { (1920 - hintSurf->w) / 2, (1080 - hintSurf->h) / 2 + 80, hintSurf->w, hintSurf->h };
                SDL_RenderCopy(cur_Renderer, ht, NULL, &hDst);
                SDL_FreeSurface(hintSurf); SDL_DestroyTexture(ht);
            }
        }
    }

    SDL_RenderPresent(cur_Renderer);
}

void Game::Clean() {
    ClearBattleObjects();

    Mix_HaltMusic();

    FreeMusic(bgm_Menu);
    FreeMusic(bgm_Battle);
    FreeChunk(se_Shoot);
    FreeChunk(se_EnemyShoot1);
    FreeChunk(se_EnemyShoot2);
    FreeChunk(se_Dead);
    FreeChunk(se_Victory);
    FreeChunk(se_PowerUp);
    FreeChunk(se_Select);
    FreeChunk(se_BeHitted);
    FreeChunk(se_REIMUBomb);
    FreeChunk(se_MARISABomb);
    Mix_CloseAudio();

    DestroyTexture(tex_PlayerReimu);
    DestroyTexture(tex_PlayerMarisa);
    DestroyTexture(tex_Enemy_Reimu);
    DestroyTexture(tex_Enemy_Marisa);
    DestroyTexture(tex_EnemyBullet);
    DestroyTexture(tex_PlayerBullet);
    tex_PowerUp = nullptr; // tex_PowerUp 只是 tex_PlayerBullet 的别名，不能重复销毁。
    DestroyTexture(tex_BackgroundMenu);
    DestroyTexture(tex_BackgroundBattle);

    if (font) {
        TTF_CloseFont(font);
        font = nullptr;
    }

    if (cur_Renderer) {
        SDL_DestroyRenderer(cur_Renderer);
        cur_Renderer = nullptr;
    }
    if (cur_Window) {
        SDL_DestroyWindow(cur_Window);
        cur_Window = nullptr;
    }

    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}
