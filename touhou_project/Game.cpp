#pragma execution_character_set("utf-8")
#include "Game.h"
#include <algorithm>
#include <cmath>
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
    if (!surf) return nullptr;
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
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) return false;
    Mix_AllocateChannels(64);

    cur_Window = SDL_CreateWindow("Touhou STG - CyberSecurity",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, ScreenWidth, ScreenHeight, SDL_WINDOW_SHOWN);
    cur_Renderer = SDL_CreateRenderer(cur_Window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!cur_Window || !cur_Renderer) return false;

    tex_PlayerReimu   = IMG_LoadTexture(cur_Renderer, "assets/image/Reimu.png");
    tex_PlayerMarisa  = IMG_LoadTexture(cur_Renderer, "assets/image/Marisa.png");
    tex_Enemy_Reimu   = IMG_LoadTexture(cur_Renderer, "assets/image/Reimu_Enemy.png");
    tex_Enemy_Marisa  = IMG_LoadTexture(cur_Renderer, "assets/image/Marisa_Enemy.png");
    tex_PlayerBullet  = LoadTextureWithColorKey("assets/image/PlayerBullet.png");
    tex_EnemyBullet   = LoadTextureWithColorKey("assets/image/EnemyBullet.png");
    tex_PowerUp       = tex_PlayerBullet;
    tex_BackgroundMenu  = IMG_LoadTexture(cur_Renderer, "assets/image/TitleBg.png");
    tex_BackgroundBattle = IMG_LoadTexture(cur_Renderer, "assets/image/BattleBg.png");
    tex_BombReimu  = IMG_LoadTexture(cur_Renderer, "assets/image/Reimu_b.png");
    tex_BombMarisa = IMG_LoadTexture(cur_Renderer, "assets/image/Marisa_b.png");

    font = TTF_OpenFont("assets\\fonts\\SimHei.ttf", 24);

    bgm_Menu   = Mix_LoadMUS("assets\\audios\\Dream Battle.mp3");
    bgm_Battle = Mix_LoadMUS("assets\\audios\\Necro-Fantasy.mp3");
    se_Shoot       = Mix_LoadWAV("assets\\sfx\\player_shoot.wav");
    se_EnemyShoot1 = Mix_LoadWAV("assets\\sfx\\enemy_shoot1.wav");
    se_EnemyShoot2 = Mix_LoadWAV("assets\\sfx\\enemy_shoot2.wav");
    se_Dead     = Mix_LoadWAV("assets\\sfx\\playerdead.wav");
    se_Victory  = Mix_LoadWAV("assets\\sfx\\victory.ogg");
    se_PowerUp  = Mix_LoadWAV("assets\\sfx\\power_up.wav");
    se_Select   = Mix_LoadWAV("assets\\sfx\\select.wav");
    se_BeHitted = Mix_LoadWAV("assets\\sfx\\playerbehitted.wav");
    se_REIMUBomb  = Mix_LoadWAV("assets\\sfx\\reimubomb.wav");
    se_MARISABomb = Mix_LoadWAV("assets\\sfx\\marisabomb.wav");

    is_Running = true;
    lastTime = SDL_GetTicks();
    CurrentState = State::MAIN_MENU;
    Mix_VolumeMusic(bgmVolume);
    Mix_Volume(-1, sfxVolume);
    if (bgm_Menu) Mix_PlayMusic(bgm_Menu, -1);
    return true;
}

void Game::ResetRunState() {
    shootTimer = 0;
    powerUpSpawnTimer = 1.0f + (rand() % 200) / 100.0f;
    continueTimer = ContinueSeconds;
    spellTimer = 0; isSpellActive = false;
    spellUser = selectedCharID; shakeTime = 0;
}

void Game::ClearBattleObjects() {
    player.reset(); Enemies.clear();
    playerBullets.clear(); enemyBullets.clear(); powerUps.clear();
}

void Game::ClearEnemyBullets() { enemyBullets.clear(); }

void Game::PlaySfx(Mix_Chunk* c) { if (c) Mix_PlayChannel(-1, c, 0); }

void Game::DestroyTexture(SDL_Texture*& t) { if (t) { SDL_DestroyTexture(t); t = nullptr; } }

void Game::FreeChunk(Mix_Chunk*& c) { if (c) { Mix_FreeChunk(c); c = nullptr; } }

void Game::FreeMusic(Mix_Music*& m) { if (m) { Mix_FreeMusic(m); m = nullptr; } }

void Game::InitBattle(CharacterID playerID) {
    ClearBattleObjects();
    selectedCharID = playerID;
    if (tex_PlayerReimu) SDL_SetTextureColorMod(tex_PlayerReimu, 255, 255, 255);
    if (tex_PlayerMarisa) SDL_SetTextureColorMod(tex_PlayerMarisa, 255, 255, 255);

    if (playerID == CharacterID::REIMU) {
        player = std::make_unique<Player>(960, 900, tex_PlayerReimu);
        Enemies.push_back(std::make_unique<Enemy>(960, 200, tex_Enemy_Marisa));
    } else {
        player = std::make_unique<Player>(960, 900, tex_PlayerMarisa);
        Enemies.push_back(std::make_unique<Enemy>(960, 200, tex_Enemy_Reimu));
    }
    SetupDialogue(playerID);
    if (!Enemies.empty()) Enemies[0]->SetupPhases(playerID);
    ResetRunState();
}

void Game::SetupDialogue(CharacterID pid) {
    DialoueQueue.clear(); cur_index = 0;
    CurrentState = State::DIALOGUE; stateBeforeDialogue = State::PLAYING;
    if (pid == CharacterID::REIMU) {
        DialoueQueue.push_back({"Reimu", "魔理沙，快停止这场DDOS攻击！", {255,100,100,255}});
        DialoueQueue.push_back({"Marisa", "嘿嘿，我的僵尸网络可是无懈可击的，DAZE！", {255,255,100,255}});
    } else {
        DialoueQueue.push_back({"Marisa", "灵梦！你的防火墙太脆弱了！", {255,255,100,255}});
        DialoueQueue.push_back({"Reimu", "那就由我来给你打个补丁吧！", {255,100,100,255}});
    }
}

void Game::CheckEnemyPhase() {
    if (Enemies.empty()) return;
    if (isSpellActive) return;  // Bomb期间什么都不做

    auto& boss = *Enemies[0];
    boss.UpdatePhase();
    if (boss.HasPhaseTransition()) {
        DialoueQueue = boss.GetTransitionDialogues();
        cur_index = 0;
        stateBeforeDialogue = CurrentState;
        CurrentState = State::DIALOGUE;
        boss.ClearPhaseTransition();
        ClearEnemyBullets();
    }
}

void Game::Run() {
    while (is_Running) {
        Uint32 ct = SDL_GetTicks();
        float dt = (ct - lastTime) / 1000.0f;
        if (dt > 0.05f) dt = 0.05f;
        lastTime = ct;
        HandleEvents();
        Update(dt);
        Render();
    }
}

void Game::HandleEvents() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) { if (e.type == SDL_QUIT) is_Running = false; }
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
        for (auto& e : Enemies) { if (e->IsActive()) e->Update(DeltaTime, player.get()); }

        if (key[SDL_SCANCODE_Z] && shootTimer <= 0) {
            PlaySfx(se_Shoot);
            BulletType bType = (selectedCharID == CharacterID::REIMU) ? BulletType::PLAYER_TALISMAN : BulletType::PLAYER_STAR;
            const Vector2D& pp = player->GetPosition();
            playerBullets.push_back(std::make_unique<Bullet>(pp.x, pp.y, 0, -600, bType));
            if (player->GetPowerLevel() >= 1) {
                playerBullets.push_back(std::make_unique<Bullet>(pp.x-15, pp.y, -100, -600, bType));
                playerBullets.push_back(std::make_unique<Bullet>(pp.x+15, pp.y,  100, -600, bType));
            }
            shootTimer = 0.07f;
        }
        shootTimer -= DeltaTime;

        powerUpSpawnTimer -= DeltaTime;
        if (powerUpSpawnTimer <= 0) {
            int n = 3 + rand() % 4;
            float cx = 200 + rand() % 1520;
            for (int i=0;i<n;i++) {
                powerUps.push_back(std::make_unique<PowerUp>(cx+rand()%60-30, -50+rand()%60-30, tex_PowerUp));
            }
            powerUpSpawnTimer = 4.0f + (rand() % 20) / 10.0f;
        }

        static bool xPressed = false;
        if (key[SDL_SCANCODE_X] && !xPressed && player->CanUseBomb() && !isSpellActive) {
            player->ConsumeBomb();
            isSpellActive = true; spellTimer = 3.0f; spellUser = selectedCharID;
            player->SetInvincible(3.5f);
            if (spellUser == CharacterID::REIMU) { PlaySfx(se_REIMUBomb);
                // 8颗光玉逐个登场，每个间隔0.25s
                reimuOrbs.clear();
                for (int i = 0; i < 8; i++) {
                    ReimuOrb orb;
                    orb.angle      = 0;
                    orb.x = orb.y  = 0;
                    orb.spawnDelay = i * 0.25f;
                    orb.speed      = 900.0f;
                    orb.spawned    = false;
                    orb.launched   = false;
                    orb.alive      = true;
                    reimuOrbs.push_back(orb);
                }
                orbSpawnTimer   = 0;
                orbSpawnIndex   = 0;
                orbAllSpawned   = false;
                orbAllLaunched  = false;
            } else { PlaySfx(se_MARISABomb); }
            xPressed = true;
        }
        if (!key[SDL_SCANCODE_X]) xPressed = false;

        if (isSpellActive) {
            spellTimer -= DeltaTime;
            for (auto& b : enemyBullets) {
                if (spellUser == CharacterID::MARISA) {
                    if (abs(b->GetPosition().x - player->GetPosition().x) < 100) b->Deactivate();
                } else { b->Deactivate(); }
            }

            // 灵梦光玉更新
            if (spellUser == CharacterID::REIMU && player) {
                const Vector2D& pp = player->GetPosition();
                bool bossAlive = !Enemies.empty() && Enemies[0]->IsActive();
                float bossX = bossAlive ? Enemies[0]->GetPosition().x : pp.x;
                float bossY = bossAlive ? Enemies[0]->GetPosition().y : pp.y - 400;
                bool bossInvincible = (CurrentState == State::DIALOGUE);

                // 1. 逐个出场
                if (!orbAllSpawned) {
                    orbSpawnTimer += DeltaTime;
                    while (orbSpawnIndex < 8 && orbSpawnTimer >= reimuOrbs[orbSpawnIndex].spawnDelay) {
                        reimuOrbs[orbSpawnIndex].spawned = true;
                        orbSpawnIndex++;
                    }
                    if (orbSpawnIndex >= 8) orbAllSpawned = true;
                }

                // 2. 已出场的光玉绕灵梦旋转（未发射前）
                for (auto& orb : reimuOrbs) {
                    if (!orb.alive) continue;
                    if (!orb.launched) {
                        if (orb.spawned) {
                            int idx = (int)(&orb - &reimuOrbs[0]);
                            orb.angle += DeltaTime * 5.0f;
                            float targetDist = 200.0f + std::sin(DeltaTime * 30.0f + idx) * 30.0f;
                            // 从判定点平滑扩张到轨道
                            float t = std::min(1.0f, (orbSpawnTimer - orb.spawnDelay) / 0.3f);
                            float dist = targetDist * t;
                            orb.x = pp.x + std::cos(orb.angle + idx * 0.785f) * dist;
                            orb.y = pp.y + std::sin(orb.angle + idx * 0.785f) * dist;
                        } else {
                            orb.x = pp.x;
                            orb.y = pp.y;
                        }
                    } else {
                        // 已发射：直线飞向Boss（或屏幕外）
                        orb.x += std::cos(orb.angle) * orb.speed * DeltaTime;
                        orb.y += std::sin(orb.angle) * orb.speed * DeltaTime;
                        if (!bossInvincible && bossAlive && Enemies[0]->IsActive()) {
                            Entity tmp(orb.x, orb.y, 20);
                            if (tmp.CheckCollision(Enemies[0].get())) {
                                Enemies[0]->hit(60);
                                orb.alive = false;
                            }
                        }
                        if (orb.x < -200 || orb.x > 2120 || orb.y < -200 || orb.y > 1280)
                            orb.alive = false;
                    }
                }

                // 3. 全部出场后统一发射
                if (orbAllSpawned && !orbAllLaunched) {
                    for (auto& orb : reimuOrbs) {
                        if (!orb.alive || orb.launched) continue;
                        float tx = bossInvincible ? (orb.x < 960 ? -200.0f : 2120.0f) : bossX;
                        float ty = bossInvincible ? -200.0f : bossY;
                        float dx = tx - orb.x, dy = ty - orb.y;
                        orb.angle = std::atan2(dy, dx);
                        orb.launched = true;
                    }
                    orbAllLaunched = true;
                }
            }

            // 魔理沙Bomb持续伤害
            for (auto& e : Enemies) {
                if (spellUser == CharacterID::MARISA && e->IsActive() && e->GetCurrentSpellHP() > 0)
                    e->hit(5);
            }
            if (spellTimer <= 0) { isSpellActive = false; reimuOrbs.clear(); orbAllSpawned = false; orbAllLaunched = false; }
        }

        if (!Enemies.empty() && Enemies[0]->IsActive()) {
            auto& boss = *Enemies[0];
            if (boss.ShouldPlaySfx(DeltaTime)) {
                if (boss.GetCurrentSfxIndex() == 1) PlaySfx(se_EnemyShoot2);
                else PlaySfx(se_EnemyShoot1);
            }
            boss.Shoot(enemyBullets, player.get(), DeltaTime);
        }

        for (auto& b : playerBullets) b->Update(DeltaTime);
        for (auto& b : enemyBullets) b->Update(DeltaTime);
        for (auto& p : powerUps) p->Update(DeltaTime);

        for (auto& b : playerBullets) {
            if (!b->IsActive()) continue;
            for (auto& e : Enemies) {
                if (e->IsActive() && b->CheckCollision(e.get())) { e->hit(player->GetAttackPoint()); b->Deactivate(); }
            }
        }

        for (auto& b : enemyBullets) {
            if (b->IsActive() && player && !player->IsInvincible() && player->CheckCollision(b.get())) {
                b->Deactivate(); player->hit(1);
                if (player->Dead_judge()) {
                    isSpellActive=false; spellTimer=0; shakeTime=0;
                    Mix_HaltMusic(); continueTimer=ContinueSeconds; PlaySfx(se_Dead); CurrentState=State::GAME_OVER;
                } else {
                    PlaySfx(se_BeHitted);
                    for (auto& eb : enemyBullets) eb->Deactivate();
                    player->ResetPosition();
                }
            }
        }

        if (player && !player->IsInvincible()) {
            for (auto& e : Enemies) {
                if (e->IsActive() && player->CheckCollision(e.get())) {
                    player->hit(1);
                    if (player->Dead_judge()) {
                        isSpellActive=false; spellTimer=0; shakeTime=0;
                        Mix_HaltMusic(); continueTimer=ContinueSeconds; PlaySfx(se_Dead); CurrentState=State::GAME_OVER;
                    } else {
                        PlaySfx(se_BeHitted);
                        for (auto& eb : enemyBullets) eb->Deactivate();
                        player->ResetPosition();
                    }
                    break;
                }
            }
        }

        for (auto& p : powerUps) {
            if (!p->IsActive() || !player) continue;
            if (std::abs(player->GetPosition().x-p->GetPosition().x)<30 &&
                std::abs(player->GetPosition().y-p->GetPosition().y)<40) {
                if (player->CollectPowerUp()==1) PlaySfx(se_PowerUp);
                p->Deactivate();
            }
        }

        RemoveInactiveObjects(playerBullets);
        RemoveInactiveObjects(enemyBullets);
        RemoveInactiveObjects(powerUps);
        CheckEnemyPhase();

        if (!Enemies.empty() && !Enemies[0]->IsAlive()) {
            PlaySfx(se_Victory); Mix_HaltMusic();
            CurrentState = State::VICTORY; isSpellActive = false; spellTimer = 0; shakeTime = 0;
        }
        break;
    }
    case State::GAME_OVER:
    case State::VICTORY:
    {
        static bool escPressed = false, rPressed = false;
        if (key[SDL_SCANCODE_ESCAPE]) {
            if (!escPressed) {
                CurrentState = State::MAIN_MENU; menuSelect = 0;
                continueTimer = ContinueSeconds; ClearBattleObjects();
                if (bgm_Menu) { Mix_HaltMusic(); Mix_PlayMusic(bgm_Menu, -1); }
                escPressed = true;
            }
        } else escPressed = false;

        if (CurrentState == State::GAME_OVER) {
            continueTimer -= DeltaTime;
            if (continueTimer <= 0) {
                CurrentState = State::MAIN_MENU;
                if (bgm_Menu) { Mix_HaltMusic(); Mix_PlayMusic(bgm_Menu, -1); }
                continueTimer = ContinueSeconds; ClearBattleObjects();
                break;
            }
            if (key[SDL_SCANCODE_R] && !rPressed) {
                player->ResetForContinue(); continueTimer = ContinueSeconds;
                for (auto& b : enemyBullets) b->Deactivate();
                if (bgm_Battle) { Mix_HaltMusic(); Mix_PlayMusic(bgm_Battle, -1); }
                CurrentState = State::PLAYING; rPressed = true;
            } else if (!key[SDL_SCANCODE_R]) rPressed = false;
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

    if (CurrentState == State::MAIN_MENU && font) {
        SDL_Color white = {255,255,255,255};
        SDL_Surface* ts = TTF_RenderUTF8_Blended(font, "东方代码乡", white);
        if (ts) { SDL_Texture* tt = SDL_CreateTextureFromSurface(cur_Renderer, ts);
            SDL_Rect tr = {(1920-ts->w*2)/2, 250, ts->w*2, ts->h*2}; SDL_RenderCopy(cur_Renderer, tt, NULL, &tr);
            SDL_FreeSurface(ts); SDL_DestroyTexture(tt); }
        auto drawMenu = [&](int idx, int y, const char* txt) {
            SDL_Color c = (menuSelect==idx)?SDL_Color{255,255,0,255}:SDL_Color{150,150,150,255};
            SDL_Surface* s = TTF_RenderUTF8_Blended(font, txt, c);
            if (s) { SDL_Texture* t = SDL_CreateTextureFromSurface(cur_Renderer, s);
                SDL_Rect r = {(1920-s->w)/2, y, s->w, s->h}; SDL_RenderCopy(cur_Renderer, t, NULL, &r);
                if (menuSelect==idx) { SDL_SetRenderDrawColor(cur_Renderer,255,255,0,255); SDL_Rect p={r.x-50,r.y+5,30,30}; SDL_RenderFillRect(cur_Renderer,&p); }
                SDL_FreeSurface(s); SDL_DestroyTexture(t); }
        };
        drawMenu(0, 550, "开始防火墙检测 (START)");
        drawMenu(1, 650, "音量设置 (VOLUME)");
        drawMenu(2, 750, "断开连接 (QUIT)");
        SDL_Surface* hs = TTF_RenderUTF8_Blended(font, "使用方向键切换，Z键确认", {100,100,100,255});
        if (hs) { SDL_Texture* ht = SDL_CreateTextureFromSurface(cur_Renderer, hs);
            SDL_Rect hr = {(1920-hs->w)/2, 950, hs->w, hs->h}; SDL_RenderCopy(cur_Renderer, ht, NULL, &hr);
            SDL_FreeSurface(hs); SDL_DestroyTexture(ht); }
    }

    if (CurrentState == State::VOLUME_SETTINGS && font) {
        SDL_SetRenderDrawBlendMode(cur_Renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(cur_Renderer,0,0,0,180); SDL_Rect fs={0,0,1920,1080}; SDL_RenderFillRect(cur_Renderer,&fs);
        auto drawItem = [&](int idx, int y, const char* txt) {
            SDL_Color c = (volMenuSelect==idx)?SDL_Color{255,255,0,255}:SDL_Color{150,150,150,255};
            SDL_Surface* s = TTF_RenderUTF8_Blended(font, txt, c);
            if (s) { SDL_Texture* t = SDL_CreateTextureFromSurface(cur_Renderer, s);
                SDL_Rect r = {(1920-s->w)/2, y, s->w, s->h}; SDL_RenderCopy(cur_Renderer, t, NULL, &r);
                SDL_FreeSurface(s); SDL_DestroyTexture(t); }
        };
        SDL_Surface* st = TTF_RenderUTF8_Blended(font, "音量详细设置", {255,255,255,255});
        if (st) { SDL_Texture* tt = SDL_CreateTextureFromSurface(cur_Renderer, st);
            SDL_Rect rt = {(1920-st->w)/2, 300, st->w, st->h}; SDL_RenderCopy(cur_Renderer, tt, NULL, &rt);
            SDL_FreeSurface(st); SDL_DestroyTexture(tt); }
        char buf[64];
        sprintf_s(buf, "BGM: < %d%% >", bgmVolume*100/128); drawItem(0, 450, buf);
        sprintf_s(buf, "SFX: < %d%% >", sfxVolume*100/128); drawItem(1, 550, buf);
        drawItem(2, 700, "确认并返回主菜单");
    }

    if (CurrentState == State::SELECT_CHARACTER) {
        int isz=200, sp=400, cx=960, cy=540;
        SDL_Rect rR={cx-sp/2-isz/2,cy-isz/2,isz,isz}, mR={cx+sp/2-isz/2,cy-isz/2,isz,isz};
        if(tex_PlayerReimu){SDL_SetTextureColorMod(tex_PlayerReimu,menuCursor==0?255:100,menuCursor==0?255:100,menuCursor==0?255:100);SDL_RenderCopy(cur_Renderer,tex_PlayerReimu,NULL,&rR);}
        if(tex_PlayerMarisa){SDL_SetTextureColorMod(tex_PlayerMarisa,menuCursor==1?255:100,menuCursor==1?255:100,menuCursor==1?255:100);SDL_RenderCopy(cur_Renderer,tex_PlayerMarisa,NULL,&mR);}
        SDL_SetRenderDrawColor(cur_Renderer,255,255,255,255); SDL_Rect bd=(menuCursor==0)?rR:mR; bd.x-=5;bd.y-=5;bd.w+=10;bd.h+=10; SDL_RenderDrawRect(cur_Renderer,&bd);
        if(font){SDL_Surface*sf=TTF_RenderUTF8_Blended(font,"选择角色(左箭头 右箭头 + Z)",{255,255,255,255});if(sf){SDL_Texture*tf=SDL_CreateTextureFromSurface(cur_Renderer,sf);SDL_Rect df={cx-sf->w/2,cy-isz/2-80,sf->w,sf->h};SDL_RenderCopy(cur_Renderer,tf,NULL,&df);SDL_FreeSurface(sf);SDL_DestroyTexture(tf);}}
    }

    if (CurrentState == State::PLAYING || CurrentState == State::DIALOGUE) {
        if (player) player->Render(cur_Renderer);
        for (auto& e : Enemies) e->Render(cur_Renderer);
        for (auto& b : playerBullets) if (b->IsActive()) b->Render(cur_Renderer, tex_PlayerBullet);
        for (auto& b : enemyBullets) if (b->IsActive()) b->Render(cur_Renderer, tex_EnemyBullet);
        for (auto& p : powerUps) if (p->IsActive()) p->Render(cur_Renderer);

        if (isSpellActive) {
            SDL_SetRenderDrawBlendMode(cur_Renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(cur_Renderer,0,0,0,150); SDL_Rect full={0,0,1920,1080}; SDL_RenderFillRect(cur_Renderer,&full);

            if (spellUser == CharacterID::REIMU && player && tex_BombReimu) {
                // 灵符「梦想封印」—— 用reimuOrbs追踪位置渲染光玉
                SDL_Color orbColors[8] = {
                    {255,60,60,255},{255,160,30,255},{255,255,50,255},{60,255,100,255},
                    {60,180,255,255},{80,60,255,255},{200,60,255,255},{255,100,180,255},
                };
                int idx = 0;
                for (auto& orb : reimuOrbs) {
                    if (!orb.alive || !orb.spawned) { idx++; continue; }
                    float os = 100.0f + std::sin(spellTimer * 10.0f + idx) * 15.0f;
                    SDL_SetTextureColorMod(tex_BombReimu, orbColors[idx].r, orbColors[idx].g, orbColors[idx].b);
                    SDL_Rect orr = {(int)(orb.x - os/2), (int)(orb.y - os/2), (int)os, (int)os};
                    SDL_RenderCopy(cur_Renderer, tex_BombReimu, NULL, &orr);
                    idx++;
                }
            } else if (spellUser == CharacterID::MARISA && player) {
                // 恋符「Master Spark」—— 激光柱
                const Vector2D& pp = player->GetPosition();
                SDL_SetRenderDrawBlendMode(cur_Renderer, SDL_BLENDMODE_ADD);
                int ox=(shakeTime>0)?(rand()%10-5):0;
                SDL_SetRenderDrawColor(cur_Renderer,100,100,255,150);
                SDL_Rect bA={(int)pp.x-120+ox,0,240,(int)pp.y}; SDL_RenderFillRect(cur_Renderer,&bA);
                SDL_SetRenderDrawColor(cur_Renderer,255,255,255,180);
                SDL_Rect bC={(int)pp.x-50+ox,0,100,(int)pp.y}; SDL_RenderFillRect(cur_Renderer,&bC);
            }

            std::string sn = (spellUser==CharacterID::REIMU)?"灵符「梦想封印」":"恋符「Master Spark」";
            SDL_Surface* ss = TTF_RenderUTF8_Blended(font,sn.c_str(),{255,255,255,255});
            if(ss){SDL_Texture*st=SDL_CreateTextureFromSurface(cur_Renderer,ss);SDL_Rect sr={1800-(int)((3.0f-spellTimer)*400),150,ss->w,ss->h};SDL_RenderCopy(cur_Renderer,st,NULL,&sr);SDL_FreeSurface(ss);SDL_DestroyTexture(st);}
        }

        if (font && player) {
            int ux=108, uy=100;
            std::string ls="Player: "; for(int i=0;i<player->GetLives();i++)ls+="* "; if(player->GetLives()<=0)ls+="None";
            SDL_Surface* sl=TTF_RenderUTF8_Blended(font,ls.c_str(),{255,105,180,255});
            if(sl){SDL_Texture*tl=SDL_CreateTextureFromSurface(cur_Renderer,sl);SDL_Rect rl={ux,uy,sl->w,sl->h};SDL_RenderCopy(cur_Renderer,tl,NULL,&rl);SDL_FreeSurface(sl);SDL_DestroyTexture(tl);}
            char pbuf[64]; sprintf_s(pbuf,"Power: %.2f / %.2f",player->GetTotalPower(),(float)MaxPowerLevel);
            SDL_Surface* sp=TTF_RenderUTF8_Blended(font,pbuf,{255,255,255,255});
            if(sp){SDL_Texture*tp=SDL_CreateTextureFromSurface(cur_Renderer,sp);SDL_Rect rp={ux,uy+40,sp->w,sp->h};SDL_RenderCopy(cur_Renderer,tp,NULL,&rp);SDL_FreeSurface(sp);SDL_DestroyTexture(tp);}
            std::string bs="Spell: "; for(int i=0;i<player->GetBombs();i++)bs+="* ";
            SDL_Surface* sb=TTF_RenderUTF8_Blended(font,bs.c_str(),{100,255,100,255});
            if(sb){SDL_Texture*tb=SDL_CreateTextureFromSurface(cur_Renderer,sb);SDL_Rect rb={ux,uy+80,sb->w,sb->h};SDL_RenderCopy(cur_Renderer,tb,NULL,&rb);SDL_FreeSurface(sb);SDL_DestroyTexture(tb);}
        }

        if (!Enemies.empty() && Enemies[0]->IsActive()) {
            Enemy* boss = Enemies[0].get();
            int barW=600, barH=18, ux=(1900-barW)/2, uy=10;
            float chp=boss->GetCurrentSpellHP(), mhp=boss->GetCurrentSpellMaxHP(), pct=(mhp>0)?(chp/mhp):0;
            if(pct<0)pct=0; if(pct>1)pct=1;
            SDL_SetRenderDrawBlendMode(cur_Renderer,SDL_BLENDMODE_BLEND);
            SDL_Rect bgR={ux,uy,barW,barH}; SDL_SetRenderDrawColor(cur_Renderer,0,0,0,150); SDL_RenderFillRect(cur_Renderer,&bgR);
            SDL_Rect hpR={ux,uy,(int)(barW*pct),barH};
            SDL_SetRenderDrawColor(cur_Renderer,pct<0.25f?255:200,pct<0.25f?50:0,pct<0.25f?50:200,255); SDL_RenderFillRect(cur_Renderer,&hpR);
            SDL_SetRenderDrawColor(cur_Renderer,255,255,255,255); SDL_RenderDrawRect(cur_Renderer,&bgR);
            if (font) {
                int remain=(int)std::ceil(boss->GetSpellTimeRemaining()); if(remain<0)remain=0;
                char tbuf[32]; sprintf_s(tbuf,"TIME: %d",remain);
                SDL_Color tc=(remain<=5)?SDL_Color{255,50,50,255}:SDL_Color{255,255,255,255};
                SDL_Surface*ts=TTF_RenderUTF8_Blended(font,tbuf,tc);
                if(ts){SDL_Texture*tt=SDL_CreateTextureFromSurface(cur_Renderer,ts);SDL_Rect tr={ux+barW+15,uy-2,ts->w,ts->h};SDL_RenderCopy(cur_Renderer,tt,NULL,&tr);SDL_FreeSurface(ts);SDL_DestroyTexture(tt);}
                char pbuf2[64]; sprintf_s(pbuf2,"%d/%d",boss->GetCurrentPhaseIndex()+1,boss->GetTotalPhases());
                SDL_Surface*ps=TTF_RenderUTF8_Blended(font,pbuf2,{200,200,200,255});
                if(ps){SDL_Texture*pt=SDL_CreateTextureFromSurface(cur_Renderer,ps);SDL_Rect pr={ux-80,uy-2,ps->w,ps->h};SDL_RenderCopy(cur_Renderer,pt,NULL,&pr);SDL_FreeSurface(ps);SDL_DestroyTexture(pt);}
            }
        }

        if (CurrentState == State::DIALOGUE && cur_index < DialoueQueue.size()) {
            SDL_SetRenderDrawBlendMode(cur_Renderer,SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(cur_Renderer,0,0,0,150); SDL_Rect fs={0,0,1920,1080}; SDL_RenderFillRect(cur_Renderer,&fs);
            SDL_Rect box={100,750,1720,250}; SDL_SetRenderDrawColor(cur_Renderer,0,0,60,255); SDL_RenderFillRect(cur_Renderer,&box);
            SDL_SetRenderDrawColor(cur_Renderer,200,200,200,255); SDL_RenderDrawRect(cur_Renderer,&box);
            if (font) {
                DialogueLine& line = DialoueQueue[cur_index];
                SDL_Surface* ns = TTF_RenderUTF8_Blended(font, line.name.c_str(), line.color);
                if (ns) { SDL_Texture* nt = SDL_CreateTextureFromSurface(cur_Renderer, ns);
                    SDL_Rect nr = {130, 765, ns->w, ns->h}; SDL_RenderCopy(cur_Renderer, nt, NULL, &nr);
                    SDL_FreeSurface(ns); SDL_DestroyTexture(nt); }
                SDL_Surface* ts = TTF_RenderUTF8_Blended(font, line.Content.c_str(), {255,255,255,255});
                if (ts) { SDL_Texture* tt = SDL_CreateTextureFromSurface(cur_Renderer, ts);
                    SDL_Rect tr = {150, 830, ts->w, ts->h}; SDL_RenderCopy(cur_Renderer, tt, NULL, &tr);
                    SDL_FreeSurface(ts); SDL_DestroyTexture(tt); }
                SDL_Surface* hs = TTF_RenderUTF8_Blended(font, "Press Z >>", {100,255,255,255});
                if (hs) { SDL_Texture* ht = SDL_CreateTextureFromSurface(cur_Renderer, hs);
                    SDL_Rect hr = {1650, 950, hs->w, hs->h}; SDL_RenderCopy(cur_Renderer, ht, NULL, &hr);
                    SDL_FreeSurface(hs); SDL_DestroyTexture(ht); }
            }
        }
    }

    if (CurrentState == State::VICTORY || CurrentState == State::GAME_OVER) {
        if (font) {
            SDL_SetRenderDrawBlendMode(cur_Renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(cur_Renderer,0,0,0,150); SDL_Rect fs={0,0,1920,1080}; SDL_RenderFillRect(cur_Renderer,&fs);
            const char* msg = (CurrentState==State::VICTORY)?"VICTORY!":"满身疮痍";
            SDL_Color co = (CurrentState==State::VICTORY)?SDL_Color{255,255,0,255}:SDL_Color{255,0,0,255};
            SDL_Surface* sf = TTF_RenderUTF8_Blended(font, msg, co);
            if(sf){SDL_Texture* tf=SDL_CreateTextureFromSurface(cur_Renderer,sf);SDL_Rect df={(1920-sf->w)/2,(1080-sf->h)/2-50,sf->w,sf->h};SDL_RenderCopy(cur_Renderer,tf,NULL,&df);SDL_FreeSurface(sf);SDL_DestroyTexture(tf);}
            if(CurrentState==State::GAME_OVER){
                char cbuf[64]; sprintf_s(cbuf,"%d秒后返回菜单",(int)ceil(continueTimer));
                SDL_Color tc=(continueTimer>3)?SDL_Color{255,255,255,255}:SDL_Color{255,50,50,255};
                SDL_Surface*sc=TTF_RenderUTF8_Blended(font,cbuf,tc);
                if(sc){SDL_Texture*tcT=SDL_CreateTextureFromSurface(cur_Renderer,sc);SDL_Rect rc={(1920-sc->w)/2,(1080-sc->h)/2,sc->w,sc->h};SDL_RenderCopy(cur_Renderer,tcT,NULL,&rc);SDL_FreeSurface(sc);SDL_DestroyTexture(tcT);}
            }
            std::string hs = (CurrentState==State::GAME_OVER)?"按下R键不屈续关 / 按下ESC返回主菜单":"按下ESC返回主菜单";
            SDL_Surface* sh = TTF_RenderUTF8_Blended(font, hs.c_str(), {255,255,255,255});
            if(sh){SDL_Texture*th=SDL_CreateTextureFromSurface(cur_Renderer,sh);SDL_Rect rh={(1920-sh->w)/2,(1080-sh->h)/2+80,sh->w,sh->h};SDL_RenderCopy(cur_Renderer,th,NULL,&rh);SDL_FreeSurface(sh);SDL_DestroyTexture(th);}
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
    DestroyTexture(tex_BombReimu); DestroyTexture(tex_BombMarisa);
    if (font) { TTF_CloseFont(font); font = nullptr; }
    if (cur_Renderer) { SDL_DestroyRenderer(cur_Renderer); cur_Renderer = nullptr; }
    if (cur_Window) { SDL_DestroyWindow(cur_Window); cur_Window = nullptr; }
    TTF_Quit(); IMG_Quit(); SDL_Quit();
}
