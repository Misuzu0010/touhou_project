#include "StageEvent.h"
#include <utility>

// ============================================================================
// 广播辅助 —— 遍历所有监听者
// ============================================================================
namespace {
    void Broadcast(std::vector<std::function<void(GameContext&)>>& listeners, GameContext& ctx) {
        for (auto& fn : listeners) {
            if (fn) fn(ctx);
        }
    }
}


// ============================================================================
// TimeEvent
// ============================================================================

TimeEvent::TimeEvent(float triggerTime)
    : triggerTime(triggerTime)
{
}

TimeEvent& TimeEvent::Bind(std::function<void(GameContext&)> listener)
{
    listeners.push_back(std::move(listener));
    return *this;
}

bool TimeEvent::ShouldTrigger(const GameContext& ctx) const
{
    return !triggered && !ctx.phaseChanged && ctx.stageTime >= triggerTime;
}

void TimeEvent::Execute(GameContext& ctx)
{
    triggered = true;
    Broadcast(listeners, ctx);
}

bool TimeEvent::isFinished() const
{
    return triggered;
}


// ============================================================================
// HpThresholdEvent
// ============================================================================

HpThresholdEvent::HpThresholdEvent(float hpThreshold)
    : hpThreshold(hpThreshold)
{
}

HpThresholdEvent& HpThresholdEvent::Bind(std::function<void(GameContext&)> listener)
{
    listeners.push_back(std::move(listener));
    return *this;
}

bool HpThresholdEvent::ShouldTrigger(const GameContext& ctx) const
{
    return !triggered && !ctx.phaseChanged && ctx.hasBoss && ctx.bossHp <= hpThreshold;
}

void HpThresholdEvent::Execute(GameContext& ctx)
{
    triggered = true;
    Broadcast(listeners, ctx);
}

bool HpThresholdEvent::isFinished() const
{
    return triggered;
}


// ============================================================================
// PhaseChangeEvent
// ============================================================================

PhaseChangeEvent::PhaseChangeEvent(int targetPhase)
    : targetPhase(targetPhase)
{
}

PhaseChangeEvent& PhaseChangeEvent::Bind(std::function<void(GameContext&)> listener)
{
    listeners.push_back(std::move(listener));
    return *this;
}

bool PhaseChangeEvent::ShouldTrigger(const GameContext& ctx) const
{
    return !triggered && ctx.phaseChanged && ctx.phaseIndex >= targetPhase;
}

void PhaseChangeEvent::Execute(GameContext& ctx)
{
    triggered = true;
    Broadcast(listeners, ctx);
}

bool PhaseChangeEvent::isFinished() const
{
    return triggered;
}


// ============================================================================
// DialogueEvent —— 自动设置 dialogueRequested，然后广播
// ============================================================================

DialogueEvent::DialogueEvent(float triggerTime)
    : triggerTime(triggerTime)
{
}

DialogueEvent& DialogueEvent::Bind(std::function<void(GameContext&)> listener)
{
    listeners.push_back(std::move(listener));
    return *this;
}

bool DialogueEvent::ShouldTrigger(const GameContext& ctx) const
{
    return !triggered && !ctx.phaseChanged && ctx.stageTime >= triggerTime;
}

void DialogueEvent::Execute(GameContext& ctx)
{
    triggered = true;
    ctx.dialogueRequested = true;
    Broadcast(listeners, ctx);
}

bool DialogueEvent::isFinished() const
{
    return triggered;
}


// ============================================================================
// SpawnItemEvent —— 自动设置 itemSpawn 信号，然后广播
// ============================================================================

SpawnItemEvent::SpawnItemEvent(float x, float y, float triggerTime)
    : spawnX(x)
    , spawnY(y)
    , triggerTime(triggerTime)
{
}

SpawnItemEvent& SpawnItemEvent::Bind(std::function<void(GameContext&)> listener)
{
    listeners.push_back(std::move(listener));
    return *this;
}

bool SpawnItemEvent::ShouldTrigger(const GameContext& ctx) const
{
    return !triggered && !ctx.phaseChanged && ctx.stageTime >= triggerTime;
}

void SpawnItemEvent::Execute(GameContext& ctx)
{
    triggered = true;
    ctx.itemSpawnRequested = true;
    ctx.itemSpawnX = spawnX;
    ctx.itemSpawnY = spawnY;
    Broadcast(listeners, ctx);
}

bool SpawnItemEvent::isFinished() const
{
    return triggered;
}


// ============================================================================
// VictoryEvent —— 自动设置 finish 信号（胜利），然后广播
// ============================================================================

VictoryEvent::VictoryEvent()
{
}

VictoryEvent& VictoryEvent::Bind(std::function<void(GameContext&)> listener)
{
    listeners.push_back(std::move(listener));
    return *this;
}

bool VictoryEvent::ShouldTrigger(const GameContext& ctx) const
{
    return !triggered && !ctx.phaseChanged && ctx.hasBoss && ctx.bossHp <= 0.0f;
}

void VictoryEvent::Execute(GameContext& ctx)
{
    triggered = true;
    ctx.finishRequested = true;
    ctx.finishIsVictory = true;
    Broadcast(listeners, ctx);
}

bool VictoryEvent::isFinished() const
{
    return triggered;
}


// ============================================================================
// DefeatEvent —— 自动设置 finish 信号（失败），然后广播
// ============================================================================

DefeatEvent::DefeatEvent()
{
}

DefeatEvent& DefeatEvent::Bind(std::function<void(GameContext&)> listener)
{
    listeners.push_back(std::move(listener));
    return *this;
}

bool DefeatEvent::ShouldTrigger(const GameContext& ctx) const
{
    return !triggered && !ctx.phaseChanged && ctx.playerDefeated;
}

void DefeatEvent::Execute(GameContext& ctx)
{
    triggered = true;
    ctx.finishRequested = true;
    ctx.finishIsVictory = false;
    Broadcast(listeners, ctx);
}

bool DefeatEvent::isFinished() const
{
    return triggered;
}
