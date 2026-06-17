#pragma once
#include "Entity.h"
#include "GameTypes.h"
#include "BulletStrategy.h"
#include "Player.h"
#include <SDL.h>
#include <memory>
#include <vector>
#include <string>

// Boss 移动模式枚举 - 每个阶段可配置不同移动策略
enum class BossMoveMode {
    STATIC,     // 静止不动
    SWAY,       // 左右正弦摇摆
    CHASE,      // 水平追踪玩家
    RETREAT     // 后退到屏幕顶部
};

// 符卡阶段配置对象 - 自包含的配置单元（策略模式）
// 只可移动不可拷贝，防止策略被意外共享
struct BossPhaseConfig {
    float spellCardHP;              // 本符卡独立血量
    float spellTime;                // 符卡时间限制（秒），超时自动切换
    std::unique_ptr<BulletStrategy> strategy;
    BossMoveMode moveMode;
    std::vector<DialogueLine> dialogues;
    float sfxCooldown;
    int   sfxIndex;
    bool  dialogueTriggered;

    BossPhaseConfig(float hp, float time,
                    std::unique_ptr<BulletStrategy> strat,
                    BossMoveMode move = BossMoveMode::STATIC,
                    std::vector<DialogueLine> dialogs = {},
                    float sfxCd = 0.12f, int sfx = 0)
        : spellCardHP(hp), spellTime(time), strategy(std::move(strat)),
          moveMode(move),
          dialogues(std::move(dialogs)), sfxCooldown(sfxCd), sfxIndex(sfx),
          dialogueTriggered(false) {}

    BossPhaseConfig(BossPhaseConfig&&) = default;
    BossPhaseConfig& operator=(BossPhaseConfig&&) = default;
    BossPhaseConfig(const BossPhaseConfig&) = delete;
    BossPhaseConfig& operator=(const BossPhaseConfig&) = delete;
};


// Enemy - Boss实体（封装了阶段管理、弹幕调度与移动模式）
class Enemy : public Entity {
private:
    // 符卡独立血条系统
    float currentSpellHP;
    float currentSpellMaxHP;
    float spellTimeRemaining;

    float attack_point = 5;
    SDL_Texture* texture;

    // 阶段管理
    std::vector<BossPhaseConfig> phases;
    int  currentPhaseIndex;
    bool phaseTransitionPending;
    int  pendingPhaseIndex;

    // 射击状态
    float shootTimer;
    float angleOffset;
    float sfxTimer;

    // 移动状态
    float moveTimer;
    float spawnX, spawnY;

public:
    Enemy(float x, float y, SDL_Texture* tex)
        : Entity(x, y, 40), currentSpellHP(6000), currentSpellMaxHP(6000),
          spellTimeRemaining(99), texture(tex),
          currentPhaseIndex(0), phaseTransitionPending(false), pendingPhaseIndex(-1),
          shootTimer(0.0f), angleOffset(0.0f), sfxTimer(0.0f),
          moveTimer(0.0f), spawnX(x), spawnY(y)
    {
        active = true;
    }

    ~Enemy() {}

    // 每帧更新（移动 + 符卡计时器）
    void Update(float deltaTime, Player* player) {
        moveTimer += deltaTime;
        UpdateMovement(deltaTime, player);
        if (!phaseTransitionPending && currentPhaseIndex < (int)phases.size()) {
            spellTimeRemaining -= deltaTime;
        }
    }

    // 阶段配置 - 根据玩家选择分叉到不同Boss
    void SetupPhases(CharacterID playerID) {
        phases.clear();
        currentPhaseIndex = 0;
        if (playerID == CharacterID::REIMU) {
            SetupMarisaPhases();
        } else {
            SetupReimuPhases();
        }
        if (!phases.empty()) {
            currentSpellHP     = phases[0].spellCardHP;
            currentSpellMaxHP  = phases[0].spellCardHP;
            spellTimeRemaining = phases[0].spellTime;
        }
    }

    // 阶段切换检测 - HP归零或时间耗尽触发
    bool UpdatePhase() {
        if (phaseTransitionPending) return false;
        bool shouldTransition = (currentSpellHP <= 0.0f || spellTimeRemaining <= 0.0f);
        if (!shouldTransition) return false;
        if (currentPhaseIndex + 1 >= (int)phases.size()) return false;

        int next = currentPhaseIndex + 1;
        if (!phases[next].dialogueTriggered) {
            phases[next].dialogueTriggered = true;
            currentPhaseIndex = next;
            pendingPhaseIndex = next;
            currentSpellHP     = phases[next].spellCardHP;
            currentSpellMaxHP  = phases[next].spellCardHP;
            spellTimeRemaining = phases[next].spellTime;
            phaseTransitionPending = true;
            return true;
        }
        return false;
    }

    bool HasPhaseTransition() const { return phaseTransitionPending; }
    const std::vector<DialogueLine>& GetTransitionDialogues() const {
        return phases[pendingPhaseIndex].dialogues;
    }
    void ClearPhaseTransition() { phaseTransitionPending = false; }

    // 弹幕射击 - 委托给当前阶段策略
    void Shoot(std::vector<std::unique_ptr<Bullet>>& enemyBullets,
               Player* player, float deltaTime)
    {
        shootTimer -= deltaTime;
        if (shootTimer <= 0.0f && currentPhaseIndex < (int)phases.size()) {
            auto& phase = phases[currentPhaseIndex];
            if (phase.strategy) {
                phase.strategy->Execute(
                    Position.x, Position.y, angleOffset,
                    enemyBullets, player);
                shootTimer = phase.strategy->GetShootInterval();
            }
        }
    }

    // SFX查询
    bool ShouldPlaySfx(float deltaTime) {
        sfxTimer -= deltaTime;
        if (sfxTimer <= 0.0f) {
            if (currentPhaseIndex < (int)phases.size()) {
                sfxTimer = phases[currentPhaseIndex].sfxCooldown;
            } else {
                sfxTimer = 0.12f;
            }
            return true;
        }
        return false;
    }

    int GetCurrentSfxIndex() const {
        if (currentPhaseIndex < (int)phases.size())
            return phases[currentPhaseIndex].sfxIndex;
        return 0;
    }

    // 移动系统 - 根据当前阶段moveMode更新位置
    void UpdateMovement(float deltaTime, Player* player) {
        if (currentPhaseIndex >= (int)phases.size()) return;
        BossMoveMode mode = phases[currentPhaseIndex].moveMode;
        switch (mode) {
        case BossMoveMode::STATIC:
            break;
        case BossMoveMode::SWAY: {
            float swayAmplitude = 200.0f;
            float swaySpeed = 2.5f;
            Position.x = spawnX + std::sin(moveTimer * swaySpeed) * swayAmplitude;
            break;
        }
        case BossMoveMode::CHASE:
            if (player) {
                float chaseSpeed = 3.0f;
                float targetX = player->GetPosition().x;
                if (targetX < 300) targetX = 300;
                if (targetX > 1620) targetX = 1620;
                Position.x += (targetX - Position.x) * chaseSpeed * deltaTime;
            }
            break;
        case BossMoveMode::RETREAT: {
            float retreatY = 120.0f;
            float retreatSpeed = 2.0f;
            Position.y += (retreatY - Position.y) * retreatSpeed * deltaTime;
            Position.x = spawnX + std::sin(moveTimer * 1.5f) * 100.0f;
            break;
        }
        }
    }

    // 渲染
    void Render(SDL_Renderer* r) override {
        if (texture) {
            int w, h;
            SDL_QueryTexture(texture, NULL, NULL, &w, &h);
            float aspect = (float)w / h;
            int displayH = 250;
            int displayW = (int)(displayH * aspect);
            SDL_Rect dest = {
                (int)Position.x - displayW / 2,
                (int)Position.y - displayH / 2,
                displayW, displayH
            };
            SDL_RenderCopy(r, texture, NULL, &dest);
        } else {
            SDL_SetRenderDrawColor(r, 255, 0, 0, 255);
            SDL_Rect rect = { (int)Position.x - 50, (int)Position.y - 50, 100, 100 };
            SDL_RenderFillRect(r, &rect);
        }
    }

    // 符卡血条与时间接口
    void hit(float attack) { currentSpellHP -= attack; }
    float GetCurrentSpellHP()     const { return currentSpellHP; }
    float GetCurrentSpellMaxHP()  const { return currentSpellMaxHP; }
    float GetSpellTimeRemaining() const { return spellTimeRemaining; }
    float GetSpellTimeTotal()     const {
        if (currentPhaseIndex < (int)phases.size())
            return phases[currentPhaseIndex].spellTime;
        return 99.0f;
    }
    float GetTotalRemainingHP() const {
        float total = currentSpellHP;
        for (int i = currentPhaseIndex + 1; i < (int)phases.size(); ++i)
            total += phases[i].spellCardHP;
        return total;
    }
    bool IsAlive() const {
        if (currentSpellHP > 0.0f && spellTimeRemaining > 0.0f) return true;
        return currentPhaseIndex + 1 < (int)phases.size();
    }
    int GetCurrentPhaseIndex() const { return currentPhaseIndex; }
    int GetTotalPhases()       const { return (int)phases.size(); }

private:
    // ==================== 灵梦 Boss 阶段配置 ====================
    void SetupReimuPhases() {
        // 0: 结界「常置阵」 800HP / 60s
        phases.push_back(BossPhaseConfig(800.0f, 60.0f,
            std::make_unique<RotatingRingStrategy>(36, 250.0f, BulletType::VIRUS),
            BossMoveMode::STATIC, {}, 0.14f, 0));
        // 1: 追踪符「梦想封印 散」 900HP / 75s
        phases.push_back(BossPhaseConfig(900.0f, 75.0f,
            std::make_unique<StopAndGoStrategy>(1.2f, 500.0f, 20, BulletType::VIRUS),
            BossMoveMode::SWAY,
            {{"系统","警告：入侵者突破了外层结界！",{180,180,180,255}},
             {"灵梦","区区魔法使，看我封印你！",{255,80,80,255}}},
            0.14f, 0));
        // 2: 爆破「阴阳鬼神玉」 1000HP / 75s
        phases.push_back(BossPhaseConfig(1000.0f, 75.0f,
            std::make_unique<BurstStrategy>(48, 380.0f, 190.0f, BulletType::VIRUS),
            BossMoveMode::RETREAT,
            {{"系统","警告：第二结界正在被侵蚀！",{180,180,180,255}},
             {"灵梦","阴阳鬼神玉——爆发！",{255,50,50,255}}},
            0.10f, 0));
        // 3: 境界「二重大结界」 900HP / 60s
        phases.push_back(BossPhaseConfig(900.0f, 60.0f,
            std::make_unique<CrossPatternStrategy>(260.0f, 4, BulletType::VIRUS),
            BossMoveMode::CHASE,
            {{"系统","严重警告：核心结界濒临崩溃！",{255,80,80,255}},
             {"灵梦","二重大结界！你逃不掉的！",{255,0,0,255}}},
            0.08f, 0));
        // 4: 神灵「梦想天生」 700HP / 60s
        phases.push_back(BossPhaseConfig(700.0f, 60.0f,
            std::make_unique<RainStrategy>(14, 370.0f, 440.0f, BulletType::VIRUS),
            BossMoveMode::SWAY,
            {{"系统","紧急：核心灵力即将耗尽！",{255,0,0,255}},
             {"灵梦","梦想天生——神明的裁决！！",{255,0,0,255}}},
            0.06f, 1));
        // 5: 终符「博丽大结界」 500HP / 50s
        phases.push_back(BossPhaseConfig(500.0f, 50.0f,
            std::make_unique<SpiralStrategy>(480.0f, 7, 0.2f, BulletType::SHURIKEN),
            BossMoveMode::SWAY,
            {{"系统","致命：最终结界启动——！！",{255,0,0,255}},
             {"灵梦","博丽大结界！这是我的全部了！！",{255,0,0,255}}},
            0.04f, 1));
    }

    // ==================== 魔理沙 Boss 阶段配置 ====================
    void SetupMarisaPhases() {
        // 0: 星符「Meteoric Shower」 800HP / 60s
        phases.push_back(BossPhaseConfig(800.0f, 60.0f,
            std::make_unique<WideShotStrategy>(9, 350.0f, 0.785f, BulletType::SHURIKEN),
            BossMoveMode::STATIC, {}, 0.12f, 0));
        // 1: 魔符「Star Dust Reverie」 900HP / 75s
        phases.push_back(BossPhaseConfig(900.0f, 75.0f,
            std::make_unique<CrossPatternStrategy>(320.0f, 5, BulletType::SHURIKEN),
            BossMoveMode::CHASE,
            {{"系统","警告：DDoS流量激增至10Gbps！",{180,180,180,255}},
             {"魔理沙","嘿嘿，这只是开胃菜DAZE！",{255,200,50,255}}},
            0.10f, 0));
        // 2: 恋符「Master Spark」 1000HP / 75s
        phases.push_back(BossPhaseConfig(1000.0f, 75.0f,
            std::make_unique<BurstStrategy>(48, 430.0f, 220.0f, BulletType::SHURIKEN),
            BossMoveMode::SWAY,
            {{"系统","警告：僵尸网络500+节点已激活！",{200,200,200,255}},
             {"魔理沙","恋符「Master Spark」——发射！！",{255,200,50,255}}},
            0.10f, 0));
        // 3: 锁符「Data Lockdown」 900HP / 60s
        phases.push_back(BossPhaseConfig(900.0f, 60.0f,
            std::make_unique<StopAndGoStrategy>(0.8f, 620.0f, 30, BulletType::LOCK),
            BossMoveMode::RETREAT,
            {{"系统","严重警告：数据加密攻击开始！",{255,80,80,255}},
             {"魔理沙","你的数据全部锁死啦！觉悟吧！",{255,50,50,255}}},
            0.10f, 0));
        // 4: 彗星「Blazing Comet」 700HP / 60s
        phases.push_back(BossPhaseConfig(700.0f, 60.0f,
            std::make_unique<SpiralStrategy>(580.0f, 7, 0.16f, BulletType::SHURIKEN),
            BossMoveMode::CHASE,
            {{"系统","紧急：系统资源即将枯竭！",{255,0,0,255}},
             {"魔理沙","彗星「Blazing Comet」！！",{255,200,50,255}}},
            0.05f, 1));
        // 5: 魔炮「Final Spark」 500HP / 50s
        phases.push_back(BossPhaseConfig(500.0f, 50.0f,
            std::make_unique<RainStrategy>(18, 430.0f, 500.0f, BulletType::SHURIKEN),
            BossMoveMode::SWAY,
            {{"系统","致命：最终攻击协议已启动——！！",{255,0,0,255}},
             {"魔理沙","魔炮「Final Spark」——全力全开DAZE！！",{255,100,0,255}}},
            0.03f, 1));
    }
};
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 