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

class StageDirector;
struct GameContext;

// ???????Update ? Render ?????????????/???
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

// ???????name ? Content ?? UTF-8 ????color ??????????
struct DialogueLine {
    std::string name;
    std::string Content;
    SDL_Color color;
};

// Boss ??????????? hpThreshold ??? dialogues?????????
struct EnemyPhase {
    float hpThreshold = 0.0f;
    bool dialogueTriggered = false;
    PatternType patternType = PatternType::Ring;
    float shootInterval = 0.5f;
    std::vector<DialogueLine> dialogues;
};

// ???????? SDL ?????????????????????????
class Game
{
public:
    Game();
    bool Init();
    void Run();
    void Clean();

private:
    // ??????????DialoueQueue ????????????????
    State CurrentState = State::MAIN_MENU;
    std::vector<DialogueLine> DialoueQueue;
    int cur_index = 0;

    // SDL ??????? Game ??????
    SDL_Window* cur_Window = nullptr;
    SDL_Renderer* cur_Renderer = nullptr;
    bool is_Running = false;

    // ??????? SDL_Texture ?? Game::Init ???? Game::Clean ???
    SDL_Texture* tex_PlayerReimu = nullptr;
    SDL_Texture* tex_PlayerMarisa = nullptr;
    SDL_Texture* tex_Enemy_Reimu = nullptr;
    SDL_Texture* tex_Enemy_Marisa = nullptr;
    SDL_Texture* tex_EnemyBullet = nullptr;
    SDL_Texture* tex_PlayerBullet = nullptr;
    SDL_Texture* tex_PowerUp = nullptr;
    SDL_Texture* tex_BackgroundMenu = nullptr;
    SDL_Texture* tex_BackgroundBattle = nullptr;

    // ??????????????UI ????????
    TTF_Font* font = nullptr;

    // ????????
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

    // ?????? unique_ptr ???????? clear ????????
    std::unique_ptr<Player> player;
    std::vector<std::unique_ptr<Enemy>> Enemies;
    std::vector<std::unique_ptr<Bullet>> playerBullets;
    std::vector<std::unique_ptr<Bullet>> enemyBullets;
    std::vector<std::unique_ptr<PowerUp>> powerUps;
    std::vector<std::unique_ptr<Item>> items;
    std::unique_ptr<SpellCard> activeSpell;
    std::unique_ptr<StageDirector> stageDirector;

    // ???????? Boss ???????
    int menuSelect = 0;
    int menuCursor = 0;
    int pauseMenuSelect = 0;
    CharacterID selectedCharID = CharacterID::REIMU;
    std::vector<EnemyPhase> enemyPhases;
    int currentPhaseIndex = 0;
    int lastPhaseIndex = 0;
    State stateBeforeDialogue = State::PLAYING;

    // ??????????
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

    // ????????????????
    void ResetRunState();
    void ClearBattleObjects();
    void ClearEnemyBullets();
    void PlaySfx(Mix_Chunk* chunk);
    void DestroyTexture(SDL_Texture*& texture);
    void FreeChunk(Mix_Chunk*& chunk);
    void FreeMusic(Mix_Music*& music);

    // ????????????????
    SDL_Texture* LoadTextureWithColorKey(const char* filename);
};
