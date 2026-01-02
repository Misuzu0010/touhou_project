#pragma once
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <vector>
#include <string>
#include "Player.h"
#include "Enemy.h"
#include "Bullet.h"
#include "BulletPattern.h"
#include "PowerUp.h"

enum class State {
    MAIN_MENU,
    SELECT_CHARACTER,
    DIALOGUE,
    PLAYING,
    GAME_OVER,
    VICTORY
};

enum class CharacterID { REIMU = 0, MARISA = 1 };

struct DialogueLine {
    std::string name;
    std::string Content;
    SDL_Color color;
};

struct EnemyPhase {
    float hpThreshold;
    bool dialogueTriggered;
    std::vector<DialogueLine> dialogues;
};

class Game
{
public:
    State CurrentState;
    std::vector<DialogueLine> DialoueQueue;
    int cur_index;

    SDL_Window* cur_Window;
    SDL_Renderer* cur_Renderer;
    bool is_Running;

    // 资源
    SDL_Texture* tex_PlayerReimu;  // 背身
    SDL_Texture* tex_PlayerMarisa; // 背身
    SDL_Texture* tex_Enemy_Reimu;  // 正面
    SDL_Texture* tex_Enemy_Marisa; // 正面
    SDL_Texture* tex_EnemyBullet;
    SDL_Texture* tex_PlayerBullet;
    SDL_Texture* tex_PowerUp;
    TTF_Font* font;

    // 游戏对象
    Player* player;
    std::vector<Enemy*> Enemies;
    std::vector<Bullet*> playerBullets;
    std::vector<Bullet*> enemyBullets;
    std::vector<PowerUp*> powerUps;

    // 逻辑控制
    int menuSelect = 0;
    int menuCursor;
    CharacterID selectedCharID;
    std::vector<EnemyPhase> enemyPhases;
    int currentPhaseIndex;
    State stateBeforeDialogue;

    Uint32 lastTime;
    float shootTimer;
    float enemyShootTimer;
    float powerUpSpawnTimer;
    float powerUpSpawnInterval;

    Game();
    bool Init();
    void Run();
    void Clean();
    void HandleEvents();
    void Update(float DeltaTime);
    void Render();

    void InitBattle(CharacterID playerID);
    void SetupDialogue(CharacterID playerID);
    void SetupEnemyPhases(CharacterID playerID);
    void CheckEnemyPhase();
    void TriggerPhaseDialogue(int phaseIndex);

    // 辅助函数：加载图片并去色
    SDL_Texture* LoadTextureWithColorKey(const char* filename);
};