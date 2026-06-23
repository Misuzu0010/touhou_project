#pragma once
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#include <memory>
#include <vector>
#include <string>
#include "Player.h"
#include "Enemy.h"
#include "Bullet.h"
#include "BulletPattern.h"
#include "PowerUp.h"
#include "SpellCard.h"
#include "Item.h"
#include "PowerItem.h"
#include "BombItem.h"
#include "LifeItem.h"
#include "StageDirector.h"

// 游戏状态：决定 Update 与 Render 使用哪一套流程。
enum class State {
    MAIN_MENU,
    VOLUME_SETTINGS,
    HELP,
    SELECT_CHARACTER,
    DIALOGUE,
    PLAYING,
    PAUSED,
    GAME_OVER,
    VICTORY
};

enum class CharacterID { REIMU = 0, MARISA = 1 };

enum class PatternType {
    Ring,
    DelayedAim,
    Spiral
};

// 单行对话数据：name 和 Content 使用 UTF-8 文本，color 控制说话人颜色。
struct DialogueLine {
    std::string name;
    std::string Content;
    SDL_Color color;
};

// Boss 血量阶段配置：hpThreshold 触发阶段，dialogues 存放阶段对话。
struct EnemyPhase {
    float hpThreshold = 0.0f;
    bool dialogueTriggered = false;
    PatternType patternType = PatternType::Ring;
    float shootInterval = 0.5f;
    std::vector<DialogueLine> dialogues;
};

// Game 管理 SDL 资源、游戏对象、状态流转和主循环。
class Game
{
public:
    Game();
    ~Game();
    bool Init();
    void Run();
    void Clean();

private:
    // 当前对话队列与正在显示的对话下标。
    State CurrentState = State::MAIN_MENU;
    std::vector<DialogueLine> DialoueQueue;
    int cur_index = 0;

    // SDL 窗口、渲染器和主循环运行标志。
    SDL_Window* cur_Window = nullptr;
    SDL_Renderer* cur_Renderer = nullptr;
    bool is_Running = false;

    // 全局贴图资源：在 Game::Init 加载，在 Game::Clean 释放。
    SDL_Texture* tex_PlayerReimu = nullptr;
    SDL_Texture* tex_PlayerMarisa = nullptr;
    SDL_Texture* tex_Enemy_Reimu = nullptr;
    SDL_Texture* tex_Enemy_Marisa = nullptr;
    SDL_Texture* tex_EnemyBullet = nullptr;
    SDL_Texture* tex_PlayerBullet = nullptr;
    SDL_Texture* tex_PowerUp = nullptr;
    SDL_Texture* tex_BackgroundMenu = nullptr;
    SDL_Texture* tex_BackgroundBattle = nullptr;

    // 用于菜单、对话和战斗 UI 的字体资源。
    TTF_Font* font = nullptr;

    // 音乐、音效与音量设置。
    Mix_Music* bgm_Menu = nullptr;
    Mix_Music* bgm_Battle = nullptr;
    Mix_Chunk* se_Shoot = nullptr;
    Mix_Chunk* se_EnemyShoot1 = nullptr;
    Mix_Chunk* se_EnemyShoot2 = nullptr;
    Mix_Chunk* se_Dead = nullptr;
    Mix_Chunk* se_Victory = nullptr;
    Mix_Chunk* se_PowerUp = nullptr;
    Mix_Chunk* se_Select = nullptr;
    Mix_Chunk* se_BeHitted = nullptr;
    Mix_Chunk* se_REIMUBomb = nullptr;
    Mix_Chunk* se_MARISABomb = nullptr;
    int bgmVolume = 32;
    int sfxVolume = 32;
    int volMenuSelect = 0;

    // 战斗对象使用 unique_ptr 管理，清理时只需 reset 或 clear。
    std::unique_ptr<Player> player;
    std::vector<std::unique_ptr<Enemy>> Enemies;
    std::vector<std::unique_ptr<Bullet>> playerBullets;
    std::vector<std::unique_ptr<Bullet>> enemyBullets;
    std::vector<std::unique_ptr<PowerUp>> powerUps;
    std::vector<std::unique_ptr<Item>> items;
    std::unique_ptr<SpellCard> activeSpell;
    std::unique_ptr<StageDirector> stageDirector;

    // 菜单选择、角色选择与 Boss 阶段状态。
    int menuSelect = 0;
    int menuCursor = 0;
    int pauseMenuSelect = 0;
    CharacterID selectedCharID = CharacterID::REIMU;
    std::vector<EnemyPhase> enemyPhases;
    int currentPhaseIndex = 0;
    int lastPhaseIndex = 0;
    State stateBeforeDialogue = State::PLAYING;

    // 各类计时器：射击、掉落、续关、屏幕震动和音效冷却。
    Uint32 lastTime = 0;
    float shootTimer = 0.0f;
    float enemyShootTimer = 0.0f;
    float powerUpSpawnTimer = 0.0f;
    float powerUpSpawnInterval = 3.0f;
    float continueTimer = 10.0f;
    float shakeTime = 0.0f;
    float angleOffset = 0.0f;
    float enemySeCooldown = 0.0f;

    void HandleEvents();
    void Update(float DeltaTime);
    void Render();

    void InitBattle(CharacterID playerID);
    void SetupDialogue(CharacterID playerID);
    void SetupEnemyPhases(CharacterID playerID);
    void SetupStageDirector();
    void CheckEnemyPhase();
    void HandleStageSignals(GameContext& ctx);
    void SpawnRandomItem(float x, float y);
    void SpawnBossRewardItems(float x, float y);
    void UpdateBulletPattern(float DeltaTime);

    // 运行状态、对象清理和资源释放辅助函数。
    void ResetRunState();
    void ClearBattleObjects();
    void ClearEnemyBullets();
    void PlaySfx(Mix_Chunk* chunk);
    void DestroyTexture(SDL_Texture*& texture);
    void FreeChunk(Mix_Chunk*& chunk);
    void FreeMusic(Mix_Music*& music);

    // 带白色透明色键的贴图加载辅助函数。
    SDL_Texture* LoadTextureWithColorKey(const char* filename);
};
