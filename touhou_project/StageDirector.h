#pragma once
#include <memory>
#include <vector>

// 游戏主状态机（原在 Game.h，移至此处避免循环依赖：Game.h 包含本头文件即可获取 State）
enum class State {
    MAIN_MENU,
    VOLUME_SETTINGS,
    SELECT_CHARACTER,
    DIALOGUE,
    PLAYING,
    GAME_OVER,
    VICTORY
};

/*
GameContext: 每帧的游戏快照，作为 Game 和 StageDirector 之间的只读上下文。
根据指南 §3.1：传递当前状态和必要战斗信息，不要把整个 Game 暴露出去。
*/
struct GameContext 
{
    State state = State::PLAYING;
    float stageTime = 0.0f;   // 战斗流程计时（仅 PLAYING 时推进）
    float deltaTime = 0.0f;   // 当前帧时间增量
    float bossHp = 0.0f;
    bool hasBoss = false;
    bool playerDefeated = false;
    int phaseIndex = 0;
    bool phaseChanged = false;

    // ---- 事件信号字段（StageEvent::Execute 写入, Game::Update 读取并响应）----
    bool dialogueRequested = false;
    bool finishRequested = false;
    bool finishIsVictory = false;
    bool itemSpawnRequested = false;
    float itemSpawnX = 0.0f;
    float itemSpawnY = 0.0f;
};

/*
StageEvent: 所有事件的抽象基类。
根据指南 §3.2：统一行为接口，ShouldTrigger 为 const 查询方法。
*/
class StageEvent
{
public:
    virtual ~StageEvent() = default;
    virtual bool ShouldTrigger(const GameContext& ctx) const = 0;
    virtual void Execute(GameContext& ctx) = 0;
    virtual bool isFinished() const = 0;
};

/*
StageDirector: 关卡导演，管理事件列表、更新事件、处理状态切换。
根据指南 §3.3 和 §5：仅 PLAYING 时推进时间和执行事件。
*/
class StageDirector
{
public:
    void Update(float deltaTime, GameContext& ctx);
    void OnStateChanged(State oldState, State newState);
    void Reset();
    void AddEvent(std::unique_ptr<StageEvent> event);

private:
    std::vector<std::unique_ptr<StageEvent>> events;
    float stageTime = 0.0f;
};
