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

// 游戏主状态机。Update 和 Render 会根据该状态分发到不同界面/逻辑。
enum class State {
    MAIN_MENU,
    VOLUME_SETTINGS,
    SELECT_CHARACTER,
    DIALOGUE,
    PLAYING,
    GAME_OVER,
    VICTORY
};

enum class CharacterID { REIMU = 0, MARISA = 1 };

// 单句剧情文本。name 和 Content 使用 UTF-8 字符串，color 用于说话者名称颜色。
struct DialogueLine {
    std::string name;
    std::string Content;
    SDL_Color color;
};

// Boss 血量阶段配置。血量低于 hpThreshold 时触发 dialogues，并切换弹幕阶段。
struct EnemyPhase {
    float hpThreshold;
    bool dialogueTriggered;
    std::vector<DialogueLine> dialogues;
};

// 游戏总控类：负责 SDL 初始化、资源加载、主循环、状态机、战斗对象和渲染。
class Game
{
public:
    Game();
    bool Init();
    void Run();
    void Clean();

private:
    // 当前状态和对话进度。DialoueQueue 保留原字段名，避免大范围重命名。
    State CurrentState = State::MAIN_MENU;
    std::vector<DialogueLine> DialoueQueue;
    int cur_index = 0;

    // SDL 窗口和渲染器由 Game 创建和销毁。
    SDL_Window* cur_Window = nullptr;
    SDL_Renderer* cur_Renderer = nullptr;
    bool is_Running = false;

    // 图片资源。所有 SDL_Texture 都由 Game::Init 加载，由 Game::Clean 释放。
    SDL_Texture* tex_PlayerReimu = nullptr;  // 背身
    SDL_Texture* tex_PlayerMarisa = nullptr; // 背身
    SDL_Texture* tex_Enemy_Reimu = nullptr;  // 正面
    SDL_Texture* tex_Enemy_Marisa = nullptr; // 正面
    SDL_Texture* tex_EnemyBullet = nullptr;
    SDL_Texture* tex_PlayerBullet = nullptr;
    SDL_Texture* tex_PowerUp = nullptr;
    SDL_Texture* tex_BackgroundMenu = nullptr;
    SDL_Texture* tex_BackgroundBattle = nullptr;

    // 字体资源。用于菜单、对话框、UI 和结算界面文字。
    TTF_Font* font = nullptr;

    // 背景音乐。Mix_Music 适合长音频循环播放。
    Mix_Music* bgm_Menu = nullptr;   // 主菜单背景音乐
    Mix_Music* bgm_Battle = nullptr; // 战斗背景音乐

    // 短音效。播放时统一通过 PlaySfx 做空指针保护。
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
    int bgmVolume = 32; // 背景音乐音量，范围 0-128。
    int sfxVolume = 32; // 音效音量，范围 0-128。
    int volMenuSelect = 0; // 音量菜单光标：0 调节 BGM，1 调节音效，2 返回。


    // 游戏对象使用 unique_ptr 表达所有权，容器 clear 时自动释放对象。
    std::unique_ptr<Player> player;
    std::vector<std::unique_ptr<Enemy>> Enemies;
    std::vector<std::unique_ptr<Bullet>> playerBullets;
    std::vector<std::unique_ptr<Bullet>> enemyBullets;
    std::vector<std::unique_ptr<PowerUp>> powerUps;

    // 菜单、角色选择和 Boss 阶段控制状态。
    int menuSelect = 0;
    int menuCursor = 0;
    CharacterID selectedCharID = CharacterID::REIMU;
    std::vector<EnemyPhase> enemyPhases;
    int currentPhaseIndex = 0;
    State stateBeforeDialogue = State::PLAYING;

    // 帧时间和玩法计时器。
    Uint32 lastTime = 0;
    float shootTimer = 0.0f;        // 玩家射击冷却。
    float enemyShootTimer = 0.0f;   // Boss 下一波弹幕计时。
    float powerUpSpawnTimer = 0.0f; // 下一批 P 点生成计时。
    float powerUpSpawnInterval = 3.0f; // 预留字段，当前生成间隔在 Update 中随机重置。
    float continueTimer = 10.0f; // 续关倒计时，初始 10 秒。
    float spellTimer = 0.0f;     // 符卡剩余持续时间。
    bool isSpellActive = false;  // 当前是否处于符卡演出和伤害阶段。
    CharacterID spellUser = CharacterID::REIMU; // 当前符卡释放者。
    float shakeTime = 0.0f; // 震屏剩余时间。
    void HandleEvents();
    void Update(float DeltaTime);
    void Render();

    void InitBattle(CharacterID playerID);
    void SetupDialogue(CharacterID playerID);
    void SetupEnemyPhases(CharacterID playerID);
    void CheckEnemyPhase();

    // 战斗状态和资源生命周期辅助函数。
    void ResetRunState();
    void ClearBattleObjects();
    void ClearEnemyBullets();
    void PlaySfx(Mix_Chunk* chunk);
    void DestroyTexture(SDL_Texture*& texture);
    void FreeChunk(Mix_Chunk*& chunk);
    void FreeMusic(Mix_Music*& music);

    // 加载图片并将纯白色键处理为透明。
    SDL_Texture* LoadTextureWithColorKey(const char* filename);
};
