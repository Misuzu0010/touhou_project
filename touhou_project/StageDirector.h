#pragma once
#include <memory>
#include <vector>

struct GameContext
{
    bool isPlaying = true;
    float stageTime = 0.0f;
    float deltaTime = 0.0f;
    float bossHp = 0.0f;
    bool hasBoss = false;
    bool playerDefeated = false;
    int phaseIndex = 0;
    bool phaseChanged = false;

    bool dialogueRequested = false;
    bool finishRequested = false;
    bool finishIsVictory = false;
    bool itemSpawnRequested = false;
    float itemSpawnX = 0.0f;
    float itemSpawnY = 0.0f;
};

class StageEvent
{
public:
    virtual ~StageEvent() = default;
    virtual bool ShouldTrigger(const GameContext& ctx) const = 0;
    virtual void Execute(GameContext& ctx) = 0;
    virtual bool isFinished() const = 0;
};

class StageDirector
{
public:
    void Update(float deltaTime, GameContext& ctx);
    void Reset();
    void AddEvent(std::unique_ptr<StageEvent> event);

private:
    std::vector<std::unique_ptr<StageEvent>> events;
    float stageTime = 0.0f;
};
