#pragma execution_character_set("utf-8")
#include "Game.h"
#include <algorithm>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <memory>

namespace {
constexpr int ScreenWidth = 1920;
constexpr int ScreenHeight = 1080;
constexpr float ContinueSeconds = 10.0f;
constexpr int MaxVolume = 128;
constexpr int MaxPowerLevel = 4;

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

SDL_Texture* Game::LoadTextureWithColorKey(const char* filename) {
    SDL_Surface* surf = IMG_Load(filename);
    if (!surf) {
        std::cout << "Failed to load: " << filename << " Err: " << IMG_GetError() << std::endl;
        return nullptr;
    }
    Uint32 colorkey = SDL_MapRGB(surf->format, 255, 255, 255);
    SDL_SetColorKey(surf, SDL_TRUE, colorkey);
    SDL_Texture* tex = SDL_CreateTextureFromSurface(cur_Renderer, surf);
    SDL_FreeSurface(surf);
    return tex;
}

bool Game::Init()
{
    std::srand(static_cast<unsigned>(std::time(nullptr)));
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

    tex_PlayerReimu = IMG_LoadTexture(cur_Renderer, "Reimu.png");
    tex_PlayerMarisa = IMG_LoadTexture(cur_Renderer, "Marisa.png");
    tex_Enemy_Reimu = IMG_LoadTexture(cur_Renderer, "Reimu_Enemy.png");
    tex_Enemy_Marisa = IMG_LoadTexture(cur_Renderer, "Marisa_Enemy.png");
    tex_PlayerBullet = LoadTextureWithColorKey("PlayerBullet.png");
    tex_EnemyBullet = LoadTextureWithColorKey("EnemyBullet.png");
    tex_PowerUp = tex_PlayerBullet;
    tex_BackgroundMenu = IMG_LoadTexture(cur_Renderer, "TitleBg.png");
    tex_BackgroundBattle = IMG_LoadTexture(cur_Renderer, "BattleBg.png");

    font = TTF_OpenFont("assets\\fonts\\SimHei.ttf", 24);
    if (!font) std::cout << "Font load failed!" << std::endl;

    bgm_Menu = Mix_LoadMUS("assets\\audios\\Dream Battle.mp3");
    bgm_Battle = Mix_LoadMUS("assets\\audios\\Necro-Fantasy.mp3");
    se_Shoot = Mix_LoadWAV("assets\\sfx\\player_shoot.wav");
    se_EnemyShoot1 = Mix_LoadWAV("assets\\sfx\\enemy_shoot1.wav");
    se_EnemyShoot2 = Mix_LoadWAV("assets\\sfx\\enemy_shoot2.wav");
    se_Dead = Mix_LoadWAV("assets\\sfx\\playerdead.wav");
    se_Victory = Mix_LoadWAV("assets\\sfx\\victory.ogg");
    se_PowerUp = Mix_LoadWAV("assets\\sfx\\power_up.wav");
    se_Select = Mix_LoadWAV("assets\\sfx\\select.wav");
    se_BeHitted = Mix_LoadWAV("assets\\sfx\\playerbehitted.wav");
    se_REIMUBomb = Mix_LoadWAV("assets\\sfx\\reimubomb.wav");
    se_MARISABomb = Mix_LoadWAV("assets\\sfx\\marisabomb.wav");

    is_Running = true;
    lastTime = SDL_GetTicks();
    CurrentState = State::MAIN_MENU;
    menuCursor = 0;
    Mix_VolumeMusic(bgmVolume);
    Mix_Volume(-1, sfxVolume);
    if (bgm_Menu) Mix_PlayMusic(bgm_Menu, -1);
    return true;
}

void Game::ResetRunState()
{
    shootTimer = 0.0f;
    powerUpSpawnTimer = 1.0f + (rand() % 200) / 100.0f;
    continueTimer = ContinueSeconds;
    spellTimer = 0.0f;
    isSpellActive = false;
    spellUser = selectedCharID;
    shakeTime = 0.0f;
}

void Game::ClearBattleObjects()
{
    player.reset();
    Enemies.clear();
    playerBullets.clear();
    enemyBullets.clear();
    powerUps.clear();
}

void Game::ClearEnemyBullets()
{
    enemyBullets.clear();
}

void Game::PlaySfx(Mix_Chunk* chunk)
{
    if (chunk) Mix_PlayChannel(-1, chunk, 0);
}

void Game::DestroyTexture(SDL_Texture*& texture)
{
    if (texture) { SDL_DestroyTexture(texture); texture = nullptr; }
}

void Game::FreeChunk(Mix_Chunk*& chunk)
{
    if (chunk) { Mix_FreeChunk(chunk); chunk = nullptr; }
}

void Game::FreeMusic(Mix_Music*& music)
{
    if (music) { Mix_FreeMusic(music); music = nullptr; }
}

void Game::InitBattle(CharacterID playerID)
{
    ClearBattleObjects();
    selectedCharID = playerID;
    if (tex_PlayerReimu) SDL_SetTextureColorMod(tex_PlayerReimu, 255, 255, 255);
    if (tex_PlayerMarisa) SDL_SetTextureColorMod(tex_PlayerMarisa, 255, 255, 255);

    if (playerID == CharacterID::REIMU) {
        player = std::make_unique<Player>(960.0f, 900.0f, tex_PlayerReimu);
        Enemies.push_back(std::make_unique<Enemy>(960.0f, 200.0f, tex_Enemy_Marisa));
    } else {
        player = std::make_unique<Player>(960.0f, 900.0f, tex_PlayerMarisa);
        Enemies.push_back(std::make_unique<Enemy>(960.0f, 200.0f, tex_Enemy_Reimu));
    }

    SetupDialogue(playerID);
    if (!Enemies.empty()) {
        Enemies[0]->SetupPhases(playerID);
    }
    ResetRunState();
}

void Game::SetupDialogue(CharacterID playerID) {
    DialoueQueue.clear();
    cur_index = 0;
    CurrentState = State::DIALOGUE;
    stateBeforeDialogue = State::PLAYING;
    if (playerID == CharacterID::REIMU) {
        DialoueQueue.push_back({"Reimu","魔理沙，快停止这场DDOS攻击！",{255,100,100,255}});
        DialoueQueue.push_back({"Marisa","嘿嘿，我的僵尸网络可是无懈可击的，DAZE！",{255,255,100,255}});
    } else {
        DialoueQueue.push_back({"Marisa","灵梦！你的防火墙太脆弱了！",{255,255,100,255}});
        DialoueQueue.push_back({"Reimu","那就由我来给你打个补丁吧！",{255,100,100,255}});
    }
}

void Game::CheckEnemyPhase() {
    if (Enemies.empty()) return;
    auto& boss = *Enemies[0];
    if (boss.UpdatePhase()) {
        DialoueQueue = boss.GetTransitionDialogues();
        cur_index = 0;
        stateBeforeDialogue = CurrentState;
        CurrentState = State::DIALOGUE;
        boss.ClearPhaseTransition();
        ClearEnemyBullets();
    }
}

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
            if (key[SDL_SCANCODE_UP]) { if (menuSelect > 0) menuSelect--; PlaySfx(se_Select); keyLock = true; }
            if (key[SDL_SCANCODE_DOWN]) { if (menuSelect < 2) menuSelect++; PlaySfx(se_Select); keyLock = true; }
            if (key[SDL_SCANCODE_Z]) {
                if (menuSelect == 0) { CurrentState = State::SELECT_CHARACTER; PlaySfx(se_Select); }
                else if (menuSelect == 1) { CurrentState = State::VOLUME_SETTINGS; PlaySfx(se_Select); }
                else if (menuSelect == 2) is_Running = false;
                keyLock = true;
            }
        }
        if (!key[SDL_SCANCODE_UP] && !key[SDL_SCANCODE_DOWN] && !key[SDL_SCANCODE_Z]) keyLock = false;
        break;
    }

    case State::VOLUME_SETTINGS:
    {
        if (!keyLock) {
            if (key[SDL_SCANCODE_UP]) { if (volMenuSelect > 0) volMenuSelect--; PlaySfx(se_Select); keyLock = true; }
            else if (key[SDL_SCANCODE_DOWN]) { if (volMenuSelect < 2) volMenuSelect++; PlaySfx(se_Select); keyLock = true; }
            if (volMenuSelect == 0) {
                if (key[SDL_SCANCODE_LEFT] && bgmVolume > 0) { bgmVolume -= 4; Mix_VolumeMusic(bgmVolume); PlaySfx(se_Select); keyLock = true; }
                if (key[SDL_SCANCODE_RIGHT] && bgmVolume < MaxVolume) { bgmVolume += 4; Mix_VolumeMusic(bgmVolume); PlaySfx(se_Select); keyLock = true; }
            } else if (volMenuSelect == 1) {
                if (key[SDL_SCANCODE_LEFT] && sfxVolume > 0) { sfxVolume -= 4; Mix_Volume(-1, sfxVolume); PlaySfx(se_Select); keyLock = true; }
                if (key[SDL_SCANCODE_RIGHT] && sfxVolume < MaxVolume) { sfxVolume += 4; Mix_Volume(-1, sfxVolume); PlaySfx(se_Select); keyLock = true; }
            }
            if (key[SDL_SCANCODE_Z] || key[SDL_SCANCODE_ESCAPE]) {
                if (volMenuSelect == 2 || key[SDL_SCANCODE_ESCAPE]) { PlaySfx(se_Select); CurrentState = State::MAIN_MENU; }
                keyLock = true;
            }
        }
        if (!key[SDL_SCANCODE_UP] && !key[SDL_SCANCODE_DOWN] && !key[SDL_SCANCODE_LEFT] &&
            !key[SDL_SCANCODE_RIGHT] && !key[SDL_SCANCODE_Z] && !key[SDL_SCANCODE_ESCAPE]) keyLock = false;
        break;
    }

    case State::SELECT_CHARACTER:
    {
        if (!keyLock) {
            if (key[SDL_SCANCODE_LEFT]) { menuCursor = 0; PlaySfx(se_Select); }
            if (key[SDL_SCANCODE_RIGHT]) { menuCursor = 1; PlaySfx(se_Select); }
            if (key[SDL_SCANCODE_ESCAPE]) CurrentState = State::MAIN_MENU;
            if (key[SDL_SCANCODE_Z]) {
                PlaySfx(se_Select);
                selectedCharID = (menuCursor == 0) ? CharacterID::REIMU : CharacterID::MARISA;
                InitBattle(selectedCharID);
                Mix_FadeOutMusic(500);
                Mix_PlayMusic(bgm_Battle, -1);
            }
            if (key[SDL_SCANCODE_LEFT] || key[SDL_SCANCODE_RIGHT] || key[SDL_SCANCODE_Z]) keyLock = true;
        } else {
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
        if (player) player->Update(DeltaTime);

        // Boss移动更新
        for (auto& e : Enemies) {
            if (e->IsActive()) e->Update(DeltaTime, player.get());
        }

        // 玩家射击
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

        // P点生成
        powerUpSpawnTimer -= DeltaTime;
        if (powerUpSpawnTimer <= 0) {
            int batchCount = 3 + rand() % 4;
            float batchCenterX = 200.0f + rand() % 1520;
            for (int i = 0; i < batchCount; i++) {
                float offsetX = (float)(rand() % 60 - 30);
                float offsetY = (float)(rand() % 60 - 30);
                powerUps.push_back(std::make_unique<PowerUp>(batchCenterX + offsetX, -50.0f + offsetY, tex_PowerUp));
            }
            powerUpSpawnTimer = 4.0f + (rand() % 20) / 10.0f;
        }

        // Bomb/符卡
        static bool xPressed = false;
        if (key[SDL_SCANCODE_X] && !xPressed && player->CanUseBomb() && !isSpellActive) {
            player->ConsumeBomb();
            isSpellActive = true;
            spellTimer = 3.0f;
            spellUser = selectedCharID;
            player->SetInvincible(3.5f);
            if (se_REIMUBomb && spellUser == CharacterID::REIMU) PlaySfx(se_REIMUBomb);
            if (se_MARISABomb && spellUser == CharacterID::MARISA) PlaySfx(se_MARISABomb);
            xPressed = true;
        }
        if (!key[SDL_SCANCODE_X]) xPressed = false;

        if (isSpellActive) {
            spellTimer -= DeltaTime;
            for (auto& b : enemyBullets) {
                if (spellUser == CharacterID::MARISA) {
                    if (abs(b->GetPosition().x - player->GetPosition().x) < 100) b->Deactivate();
                } else {
                    b->Deactivate();
                }
            }
            for (auto& e : Enemies) {
                if (e->IsActive()) e->hit(10);
            }
            if (spellTimer <= 0) isSpellActive = false;
        }

        // Boss弹幕与音效（委托给Enemy封装）
        if (!Enemies.empty() && Enemies[0]->IsActive()) {
            auto& boss = *Enemies[0];
            if (boss.ShouldPlaySfx(DeltaTime)) {
                int idx = boss.GetCurrentSfxIndex();
                if (idx == 1) PlaySfx(se_EnemyShoot2);
                else          PlaySfx(se_EnemyShoot1);
            }
            boss.Shoot(enemyBullets, player.get(), DeltaTime);
        }

        // 更新所有对象
        for (auto& b : playerBullets) b->Update(DeltaTime);
        for (auto& b : enemyBullets) b->Update(DeltaTime);
        for (auto& p : powerUps) p->Update(DeltaTime);

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

        // 碰撞：子弹打玩家
        for (auto& b : enemyBullets) {
            if (b->IsActive() && player && !player->IsInvincible() && player->CheckCollision(b.get())) {
                b->Deactivate();
                player->hit(1.0f);
                if (player->Dead_judge()) {
                    isSpellActive = false; spellTimer = 0.0f; shakeTime = 0.0f;
                    Mix_HaltMusic(); continueTimer = ContinueSeconds;
                    PlaySfx(se_Dead); CurrentState = State::GAME_OVER;
                } else {
                    PlaySfx(se_BeHitted);
                    for (auto& eb : enemyBullets) eb->Deactivate();
                    player->ResetPosition();
                }
            }
        }

        // 碰撞：自机碰Boss（体术）
        if (player && !player->IsInvincible()) {
            for (auto& e : Enemies) {
                if (e->IsActive() && player->CheckCollision(e.get())) {
                    player->hit(1.0f);
                    if (player->Dead_judge()) {
                        isSpellActive = false; spellTimer = 0.0f; shakeTime = 0.0f;
                        Mix_HaltMusic(); continueTimer = ContinueSeconds;
                        PlaySfx(se_Dead); CurrentState = State::GAME_OVER;
                    } else {
                        PlaySfx(se_BeHitted);
                        for (auto& eb : enemyBullets) eb->Deactivate();
                        player->ResetPosition();
                    }
                    break;
                }
            }
        }

        // 碰撞：拾取P点
        for (auto& p : powerUps) {
            float pickUpWidth = 60.0f;
            float pickUpHeight = 80.0f;
            if (!p->IsActive() || !player) continue;
            float diffX = std::abs(player->GetPosition().x - p->GetPosition().x);
            float diffY = std::abs(player->GetPosition().y - p->GetPosition().y);
            if ((diffX < pickUpWidth / 2) && (diffY < pickUpHeight / 2)) {
                if (player->CollectPowerUp() == 1) PlaySfx(se_PowerUp);
                p->Deactivate();
            }
        }

        // 清理失效对象
        RemoveInactiveObjects(playerBullets);
        RemoveInactiveObjects(enemyBullets);
        RemoveInactiveObjects(powerUps);

        // 阶段切换检测
        CheckEnemyPhase();

        // 胜利判定
        if (!Enemies.empty() && !Enemies[0]->IsAlive()) {
            PlaySfx(se_Victory);
            Mix_HaltMusic();
            CurrentState = State::VICTORY;
            isSpellActive = false; spellTimer = 0.0f; shakeTime = 0.0f;
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
                CurrentState = State::MAIN_MENU; menuSelect = 0;
                continueTimer = ContinueSeconds; ClearBattleObjects();
                if (bgm_Menu) { Mix_HaltMusic(); Mix_PlayMusic(bgm_Menu, -1); }
                escPressed = true;
            }
        } else { escPressed = false; }

        if (CurrentState == State::GAME_OVER) {
            continueTimer -= DeltaTime;
            if (continueTimer <= 0) {
                CurrentState = State::MAIN_MENU;
                if (bgm_Menu) { Mix_HaltMusic(); Mix_PlayMusic(bgm_Menu, -1); }
                continueTimer = ContinueSeconds; ClearBattleObjects();
                break;
            }
            if (key[SDL_SCANCODE_R]) {
                if (!rPressed) {
                    player->ResetForContinue();
                    continueTimer = ContinueSeconds;
                    for (auto& b : enemyBullets) b->Deactivate();
                    if (bgm_Battle) { Mix_HaltMusic(); Mix_PlayMusic(bgm_Battle, -1); }
                    CurrentState = State::PLAYING;
                    rPressed = true;
                }
            } else { rPressed = false; }
        }
        break;
    }
    }
}

void Game::Render()
{
    SDL_SetRenderDrawColor(cur_Renderer, 0, 0, 0, 255);
    SDL_RenderClear(cur_Renderer);
    SDL_Rect bgRect = {0, 0, 1920, 1080};

    if (CurrentState == State::MAIN_MENU || CurrentState == State::VOLUME_SETTINGS || CurrentState == State::SELECT_CHARACTER) {
        if (tex_BackgroundMenu) SDL_RenderCopy(cur_Renderer, tex_BackgroundMenu, NULL, &bgRect);
    } else {
        if (tex_BackgroundBattle) SDL_RenderCopy(cur_Renderer, tex_BackgroundBattle, NULL, &bgRect);
    }

    // ========== 主菜单 ==========
    if (CurrentState == State::MAIN_MENU && font) {
        SDL_Color white = {255,255,255,255};
        SDL_Surface* titleSurf = TTF_RenderUTF8_Blended(font,"东方代码乡",white);
        if (titleSurf) {
            SDL_Texture* titleTex = SDL_CreateTextureFromSurface(cur_Renderer, titleSurf);
            SDL_Rect tr = {(1920-titleSurf->w*2)/2, 250, titleSurf->w*2, titleSurf->h*2};
            SDL_RenderCopy(cur_Renderer, titleTex, NULL, &tr);
            SDL_FreeSurface(titleSurf); SDL_DestroyTexture(titleTex);
        }
        SDL_Color c0 = (menuSelect==0)?SDL_Color{255,255,0,255}:SDL_Color{150,150,150,255};
        SDL_Surface* s0 = TTF_RenderUTF8_Blended(font,"开始防火墙检测 (START)",c0);
        if (s0) { SDL_Texture* t0 = SDL_CreateTextureFromSurface(cur_Renderer, s0);
            SDL_Rect r0 = {(1920-s0->w)/2, 550, s0->w, s0->h}; SDL_RenderCopy(cur_Renderer, t0, NULL, &r0);
            if (menuSelect==0) { SDL_SetRenderDrawColor(cur_Renderer,255,255,0,255);
                SDL_Rect p = {r0.x-50, r0.y+5, 30, 30}; SDL_RenderFillRect(cur_Renderer,&p); }
            SDL_FreeSurface(s0); SDL_DestroyTexture(t0); }
        SDL_Color c1 = (menuSelect==1)?SDL_Color{255,255,0,255}:SDL_Color{150,150,150,255};
        SDL_Surface* s1 = TTF_RenderUTF8_Blended(font,"音量设置 (VOLUME)",c1);
        if (s1) { SDL_Texture* t1 = SDL_CreateTextureFromSurface(cur_Renderer, s1);
            SDL_Rect r1 = {(1920-s1->w)/2, 650, s1->w, s1->h}; SDL_RenderCopy(cur_Renderer, t1, NULL, &r1);
            if (menuSelect==1) { SDL_SetRenderDrawColor(cur_Renderer,255,255,0,255);
                SDL_Rect p = {r1.x-50, r1.y+5, 30, 30}; SDL_RenderFillRect(cur_Renderer,&p); }
            SDL_FreeSurface(s1); SDL_DestroyTexture(t1); }
        SDL_Color c2 = (menuSelect==2)?SDL_Color{255,255,0,255}:SDL_Color{150,150,150,255};
        SDL_Surface* s2 = TTF_RenderUTF8_Blended(font,"断开连接 (QUIT)",c2);
        if (s2) { SDL_Texture* t2 = SDL_CreateTextureFromSurface(cur_Renderer, s2);
            SDL_Rect r2 = {(1920-s2->w)/2, 750, s2->w, s2->h}; SDL_RenderCopy(cur_Renderer, t2, NULL, &r2);
            if (menuSelect==2) { SDL_SetRenderDrawColor(cur_Renderer,255,255,0,255);
                SDL_Rect p = {r2.x-50, r2.y+5, 30, 30}; SDL_RenderFillRect(cur_Renderer,&p); }
            SDL_FreeSurface(s2); SDL_DestroyTexture(t2); }
        SDL_Surface* hint = TTF_RenderUTF8_Blended(font,"使用方向键切换，Z键确认",{100,100,100,255});
        if (hint) { SDL_Texture* ht = SDL_CreateTextureFromSurface(cur_Renderer, hint);
            SDL_Rect hr = {(1920-hint->w)/2, 950, hint->w, hint->h}; SDL_RenderCopy(cur_Renderer, ht, NULL, &hr);
            SDL_FreeSurface(hint); SDL_DestroyTexture(ht); }
    }

    // ========== 音量设置 ==========
    if (CurrentState == State::VOLUME_SETTINGS && font) {
        SDL_SetRenderDrawBlendMode(cur_Renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(cur_Renderer, 0,0,0,180);
        SDL_Rect fs = {0,0,1920,1080}; SDL_RenderFillRect(cur_Renderer, &fs);
        SDL_Color white = {255,255,255,255};
        SDL_Surface* sT = TTF_RenderUTF8_Blended(font,"音量详细设置",white);
        if (sT) { SDL_Texture* tT = SDL_CreateTextureFromSurface(cur_Renderer, sT);
            SDL_Rect rT = {(1920-sT->w)/2, 300, sT->w, sT->h}; SDL_RenderCopy(cur_Renderer, tT, NULL, &rT);
            SDL_FreeSurface(sT); SDL_DestroyTexture(tT); }
        SDL_Color cB = (volMenuSelect==0)?SDL_Color{255,255,0,255}:SDL_Color{150,150,150,255};
        std::string bs = "BGM: < "+std::to_string(bgmVolume*100/128)+"% >";
        SDL_Surface* sB = TTF_RenderUTF8_Blended(font, bs.c_str(), cB);
        if (sB) { SDL_Texture* tB = SDL_CreateTextureFromSurface(cur_Renderer, sB);
            SDL_Rect rB = {(1920-sB->w)/2, 450, sB->w, sB->h}; SDL_RenderCopy(cur_Renderer, tB, NULL, &rB);
            SDL_FreeSurface(sB); SDL_DestroyTexture(tB); }
        SDL_Color cS = (volMenuSelect==1)?SDL_Color{255,255,0,255}:SDL_Color{150,150,150,255};
        std::string ss = "SFX: < "+std::to_string(sfxVolume*100/128)+"% >";
        SDL_Surface* sS = TTF_RenderUTF8_Blended(font, ss.c_str(), cS);
        if (sS) { SDL_Texture* tS = SDL_CreateTextureFromSurface(cur_Renderer, sS);
            SDL_Rect rS = {(1920-sS->w)/2, 550, sS->w, sS->h}; SDL_RenderCopy(cur_Renderer, tS, NULL, &rS);
            SDL_FreeSurface(sS); SDL_DestroyTexture(tS); }
        SDL_Color cBack = (volMenuSelect==2)?SDL_Color{255,255,0,255}:SDL_Color{150,150,150,255};
        SDL_Surface* sBack = TTF_RenderUTF8_Blended(font,"确认并返回主菜单",cBack);
        if (sBack) { SDL_Texture* tBack = SDL_CreateTextureFromSurface(cur_Renderer, sBack);
            SDL_Rect rBack = {(1920-sBack->w)/2, 700, sBack->w, sBack->h}; SDL_RenderCopy(cur_Renderer, tBack, NULL, &rBack);
            SDL_FreeSurface(sBack); SDL_DestroyTexture(tBack); }
    }

    // ========== 角色选择 ==========
    if (CurrentState == State::SELECT_CHARACTER) {
        int iconSize = 200, spacing = 400, cx = 960, cy = 540;
        SDL_Rect rRect = {cx-spacing/2-iconSize/2, cy-iconSize/2, iconSize, iconSize};
        SDL_Rect mRect = {cx+spacing/2-iconSize/2, cy-iconSize/2, iconSize, iconSize};
        if (tex_PlayerReimu) {
            SDL_SetTextureColorMod(tex_PlayerReimu, (menuCursor==0)?255:100, (menuCursor==0)?255:100, (menuCursor==0)?255:100);
            SDL_RenderCopy(cur_Renderer, tex_PlayerReimu, NULL, &rRect);
        }
        if (tex_PlayerMarisa) {
            SDL_SetTextureColorMod(tex_PlayerMarisa, (menuCursor==1)?255:100, (menuCursor==1)?255:100, (menuCursor==1)?255:100);
            SDL_RenderCopy(cur_Renderer, tex_PlayerMarisa, NULL, &mRect);
        }
        SDL_SetRenderDrawColor(cur_Renderer, 255,255,255,255);
        SDL_Rect border = (menuCursor==0)?rRect:mRect;
        border.x-=5; border.y-=5; border.w+=10; border.h+=10;
        SDL_RenderDrawRect(cur_Renderer, &border);
        if (font) {
            SDL_Surface* sf = TTF_RenderUTF8_Blended(font,"选择角色(←/→ + Z)",{255,255,255,255});
            if (sf) { SDL_Texture* tf = SDL_CreateTextureFromSurface(cur_Renderer, sf);
                SDL_Rect df = {cx-sf->w/2, cy-iconSize/2-80, sf->w, sf->h}; SDL_RenderCopy(cur_Renderer, tf, NULL, &df);
                SDL_FreeSurface(sf); SDL_DestroyTexture(tf); }
        }
    }

    // ========== 战斗/对话 ==========
    if (CurrentState == State::PLAYING || CurrentState == State::DIALOGUE) {
        if (player) player->Render(cur_Renderer);
        for (auto& e : Enemies) e->Render(cur_Renderer);
        for (auto& b : playerBullets) if (b->IsActive()) b->Render(cur_Renderer, tex_PlayerBullet);
        for (auto& b : enemyBullets) if (b->IsActive()) b->Render(cur_Renderer, tex_EnemyBullet);
        for (auto& p : powerUps) if (p->IsActive()) p->Render(cur_Renderer);

        // 符卡演出
        if (isSpellActive) {
            SDL_SetRenderDrawBlendMode(cur_Renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(cur_Renderer,0,0,0,150);
            SDL_Rect full = {0,0,1920,1080}; SDL_RenderFillRect(cur_Renderer, &full);
            if (spellUser == CharacterID::REIMU) {
                for (int i=0;i<8;i++) {
                    float angle = (spellTimer*4.0f)+(i*0.785f);
                    float dist = 300.0f-(spellTimer*50.0f);
                    const Vector2D& pp = player->GetPosition();
                    int tx = (int)(pp.x+std::cos(angle)*dist), ty = (int)(pp.y+std::sin(angle)*dist);
                    SDL_SetRenderDrawColor(cur_Renderer,255,100,200,180);
                    SDL_Rect orb = {tx-60,ty-60,120,120}; SDL_RenderFillRect(cur_Renderer,&orb);
                }
            } else {
                SDL_SetRenderDrawBlendMode(cur_Renderer, SDL_BLENDMODE_ADD);
                int ox = (shakeTime>0)?(rand()%10-5):0;
                const Vector2D& pp = player->GetPosition();
                SDL_SetRenderDrawColor(cur_Renderer,100,100,255,200);
                SDL_Rect bA = {(int)pp.x-120+ox,0,240,(int)pp.y}; SDL_RenderFillRect(cur_Renderer,&bA);
                SDL_SetRenderDrawColor(cur_Renderer,255,255,255,255);
                SDL_Rect bC = {(int)pp.x-70+ox,0,140,(int)pp.y}; SDL_RenderFillRect(cur_Renderer,&bC);
                SDL_SetRenderDrawBlendMode(cur_Renderer, SDL_BLENDMODE_BLEND);
            }
            std::string sName = (spellUser==CharacterID::REIMU)?"灵符「梦想封印」":"恋符「Master Spark」";
            SDL_Surface* sSurf = TTF_RenderUTF8_Blended(font, sName.c_str(), {255,255,255,255});
            if (sSurf) { SDL_Texture* sTex = SDL_CreateTextureFromSurface(cur_Renderer, sSurf);
                SDL_Rect sR = {1800-(int)((3.0f-spellTimer)*400), 150, sSurf->w, sSurf->h};
                SDL_RenderCopy(cur_Renderer, sTex, NULL, &sR); SDL_FreeSurface(sSurf); SDL_DestroyTexture(sTex); }
        }

        // UI
        if (font && player) {
            int uiX = 108, uiY = 100;
            std::string lifeStr = "Player: ";
            for (int i=0;i<player->GetLives();i++) lifeStr += "★ ";
            if (player->GetLives()<=0) lifeStr += "None";
            SDL_Surface* sL = TTF_RenderUTF8_Blended(font, lifeStr.c_str(), {255,105,180,255});
            if (sL) { SDL_Texture* tL = SDL_CreateTextureFromSurface(cur_Renderer, sL);
                SDL_Rect rL = {uiX,uiY,sL->w,sL->h}; SDL_RenderCopy(cur_Renderer,tL,NULL,&rL);
                SDL_FreeSurface(sL); SDL_DestroyTexture(tL); }
            char powerBuf[64];
            sprintf_s(powerBuf,"Power: %.2f / %.2f",player->GetTotalPower(),(float)MaxPowerLevel);
            SDL_Surface* sP = TTF_RenderUTF8_Blended(font, powerBuf, {255,255,255,255});
            if (sP) { SDL_Texture* tP = SDL_CreateTextureFromSurface(cur_Renderer, sP);
                SDL_Rect rP = {uiX,uiY+40,sP->w,sP->h}; SDL_RenderCopy(cur_Renderer,tP,NULL,&rP);
                SDL_FreeSurface(sP); SDL_DestroyTexture(tP); }
            std::string bombStr = "Spell: ";
            for (int i=0;i<player->GetBombs();i++) bombStr += "★ ";
            SDL_Surface* sB = TTF_RenderUTF8_Blended(font, bombStr.c_str(), {100,255,100,255});
            if (sB) { SDL_Texture* tB = SDL_CreateTextureFromSurface(cur_Renderer, sB);
                SDL_Rect rB = {uiX,uiY+80,sB->w,sB->h}; SDL_RenderCopy(cur_Renderer,tB,NULL,&rB);
                SDL_FreeSurface(sB); SDL_DestroyTexture(tB); }
        }

        // Boss符卡血条 + 倒计时
        if (!Enemies.empty() && Enemies[0]->IsActive()) {
            Enemy* boss = Enemies[0].get();
            int barW = 600, barH = 18;
            int uiX = (1900-barW)/2, uiY = 10;

            float chp = boss->GetCurrentSpellHP();
            float mhp = boss->GetCurrentSpellMaxHP();
            float pct = (mhp>0)?(chp/mhp):0.0f;
            if (pct<0)pct=0; if (pct>1)pct=1;

            SDL_SetRenderDrawBlendMode(cur_Renderer, SDL_BLENDMODE_BLEND);
            SDL_Rect bgR = {uiX,uiY,barW,barH};
            SDL_SetRenderDrawColor(cur_Renderer,0,0,0,150);
            SDL_RenderFillRect(cur_Renderer,&bgR);
            SDL_Rect hpR = {uiX,uiY,(int)(barW*pct),barH};
            if (pct<0.25f) SDL_SetRenderDrawColor(cur_Renderer,255,50,50,255);
            else SDL_SetRenderDrawColor(cur_Renderer,200,0,200,255);
            SDL_RenderFillRect(cur_Renderer,&hpR);
            SDL_SetRenderDrawColor(cur_Renderer,255,255,255,255);
            SDL_RenderDrawRect(cur_Renderer,&bgR);

            if (font) {
                int remain = (int)std::ceil(boss->GetSpellTimeRemaining());
                if (remain<0) remain=0;
                char tbuf[32]; sprintf_s(tbuf,"TIME: %d",remain);
                SDL_Color tc = (remain<=5)?SDL_Color{255,50,50,255}:SDL_Color{255,255,255,255};
                SDL_Surface* tS = TTF_RenderUTF8_Blended(font,tbuf,tc);
                if (tS) { SDL_Texture* tT = SDL_CreateTextureFromSurface(cur_Renderer,tS);
                    SDL_Rect tR = {uiX+barW+15,uiY-2,tS->w,tS->h}; SDL_RenderCopy(cur_Renderer,tT,NULL,&tR);
                    SDL_FreeSurface(tS); SDL_DestroyTexture(tT); }
                char pbuf[64]; sprintf_s(pbuf,"%d/%d",boss->GetCurrentPhaseIndex()+1,boss->GetTotalPhases());
                SDL_Surface* pS = TTF_RenderUTF8_Blended(font,pbuf,{200,200,200,255});
                if (pS) { SDL_Texture* pT = SDL_CreateTextureFromSurface(cur_Renderer,pS);
                    SDL_Rect pR = {uiX-80,uiY-2,pS->w,pS->h}; SDL_RenderCopy(cur_Renderer,pT,NULL,&pR);
                    SDL_FreeSurface(pS); SDL_DestroyTexture(pT); }
            }
        }

        // 对话遮罩+对话框
        if (CurrentState == State::DIALOGUE && cur_index < DialoueQueue.size()) {
            SDL_SetRenderDrawBlendMode(cur_Renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(cur_Renderer,0,0,0,150);
            SDL_Rect fs = {0,0,1920,1080}; SDL_RenderFillRect(cur_Renderer,&fs);
            SDL_Rect box = {100,750,1720,250};
            SDL_SetRenderDrawColor(cur_Renderer,0,0,60,255); SDL_RenderFillRect(cur_Renderer,&box);
            SDL_SetRenderDrawColor(cur_Renderer,200,200,200,255); SDL_RenderDrawRect(cur_Renderer,&box);
            if (font) {
                DialogueLine& line = DialoueQueue[cur_index];
                SDL_Surface* nS = TTF_RenderUTF8_Blended(font,line.name.c_str(),line.color);
                if (nS) { SDL_Texture* nT = SDL_CreateTextureFromSurface(cur_Renderer,nS);
                    SDL_Rect nR = {130,765,nS->w,nS->h}; SDL_RenderCopy(cur_Renderer,nT,NULL,&nR);
                    SDL_FreeSurface(nS); SDL_DestroyTexture(nT); }
                SDL_Surface* tS = TTF_RenderUTF8_Blended(font,line.Content.c_str(),{255,255,255,255});
                if (tS) { SDL_Texture* tT = SDL_CreateTextureFromSurface(cur_Renderer,tS);
                    SDL_Rect tR = {150,830,tS->w,tS->h}; SDL_RenderCopy(cur_Renderer,tT,NULL,&tR);
                    SDL_FreeSurface(tS); SDL_DestroyTexture(tT); }
                SDL_Surface* hS = TTF_RenderUTF8_Blended(font,"Press Z >>",{100,255,255,255});
                if (hS) { SDL_Texture* hT = SDL_CreateTextureFromSurface(cur_Renderer,hS);
                    SDL_Rect hR = {1650,950,hS->w,hS->h}; SDL_RenderCopy(cur_Renderer,hT,NULL,&hR);
                    SDL_FreeSurface(hS); SDL_DestroyTexture(hT); }
            }
        }
    }

    // ========== 结算界面 ==========
    if (CurrentState == State::VICTORY || CurrentState == State::GAME_OVER) {
        if (font) {
            SDL_SetRenderDrawBlendMode(cur_Renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(cur_Renderer,0,0,0,150);
            SDL_Rect fs = {0,0,1920,1080}; SDL_RenderFillRect(cur_Renderer,&fs);
            const char* msg = (CurrentState==State::VICTORY)?"VICTORY!":"满身疮痍";
            SDL_Color co = (CurrentState==State::VICTORY)?SDL_Color{255,255,0,255}:SDL_Color{255,0,0,255};
            SDL_Surface* sf = TTF_RenderUTF8_Blended(font,msg,co);
            if (sf) { SDL_Texture* tf = SDL_CreateTextureFromSurface(cur_Renderer,sf);
                SDL_Rect df = {(1920-sf->w)/2,(1080-sf->h)/2-50,sf->w,sf->h}; SDL_RenderCopy(cur_Renderer,tf,NULL,&df);
                SDL_FreeSurface(sf); SDL_DestroyTexture(tf); }
            if (CurrentState==State::GAME_OVER) {
                char cbuf[64]; sprintf_s(cbuf,"%d秒后返回菜单",(int)ceil(continueTimer));
                SDL_Color tc = (continueTimer>3.0f)?SDL_Color{255,255,255,255}:SDL_Color{255,50,50,255};
                SDL_Surface* sc = TTF_RenderUTF8_Blended(font,cbuf,tc);
                if (sc) { SDL_Texture* tcT = SDL_CreateTextureFromSurface(cur_Renderer,sc);
                    SDL_Rect rc = {(1920-sc->w)/2,(1080-sc->h)/2,sc->w,sc->h}; SDL_RenderCopy(cur_Renderer,tcT,NULL,&rc);
                    SDL_FreeSurface(sc); SDL_DestroyTexture(tcT); }
            }
            std::string hs = (CurrentState==State::GAME_OVER)?"按下R键不屈续关 / 按下ESC返回主菜单":"按下ESC返回主菜单";
            SDL_Surface* sh = TTF_RenderUTF8_Blended(font,hs.c_str(),{255,255,255,255});
            if (sh) { SDL_Texture* th = SDL_CreateTextureFromSurface(cur_Renderer,sh);
                SDL_Rect rh = {(1920-sh->w)/2,(1080-sh->h)/2+80,sh->w,sh->h}; SDL_RenderCopy(cur_Renderer,th,NULL,&rh);
                SDL_FreeSurface(sh); SDL_DestroyTexture(th); }
        }
    }

    SDL_RenderPresent(cur_Renderer);
}

void Game::Clean() {
    ClearBattleObjects();
    Mix_HaltMusic();
    FreeMusic(bgm_Menu); FreeMusic(bgm_Battle);
    FreeChunk(se_Shoot); FreeChunk(se_EnemyShoot1); FreeChunk(se_EnemyShoot2);
    FreeChunk(se_Dead); FreeChunk(se_Victory); FreeChunk(se_PowerUp);
    FreeChunk(se_Select); FreeChunk(se_BeHitted);
    FreeChunk(se_REIMUBomb); FreeChunk(se_MARISABomb);
    Mix_CloseAudio();
    DestroyTexture(tex_PlayerReimu); DestroyTexture(tex_PlayerMarisa);
    DestroyTexture(tex_Enemy_Reimu); DestroyTexture(tex_Enemy_Marisa);
    DestroyTexture(tex_EnemyBullet); DestroyTexture(tex_PlayerBullet);
    tex_PowerUp = nullptr;
    DestroyTexture(tex_BackgroundMenu); DestroyTexture(tex_BackgroundBattle);
    if (font) { TTF_CloseFont(font); font = nullptr; }
    if (cur_Renderer) { SDL_DestroyRenderer(cur_Renderer); cur_Renderer = nullptr; }
    if (cur_Window) { SDL_DestroyWindow(cur_Window); cur_Window = nullptr; }
    TTF_Quit(); IMG_Quit(); SDL_Quit();
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  