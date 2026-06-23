#include "StageEvent.h"
#include <utility>

namespace {
void Broadcast(std::vector<std::function<void(GameContext&)>>& listeners, GameContext& ctx)
{
    for (auto& fn : listeners) {
        if (fn) {
            fn(ctx);
        }
    }
}
}

TimeEvent::TimeEvent(float triggerTime) : triggerTime(triggerTime) {}
TimeEvent& TimeEvent::Bind(std::function<void(GameContext&)> listener)
{
    listeners.push_back(std::move(listener));
    return *this;
}
bool TimeEvent::ShouldTrigger(const GameContext& ctx) const
{
    return !triggered && ctx.stageTime >= triggerTime;
}
void TimeEvent::Execute(GameContext& ctx)
{
    triggered = true;
    Broadcast(listeners, ctx);
}
bool TimeEvent::isFinished() const { return triggered; }

HpThresholdEvent::HpThresholdEvent(float hpThreshold) : hpThreshold(hpThreshold) {}
HpThresholdEvent& HpThresholdEvent::Bind(std::function<void(GameContext&)> listener)
{
    listeners.push_back(std::move(listener));
    return *this;
}
bool HpThresholdEvent::ShouldTrigger(const GameContext& ctx) const
{
    return !triggered && ctx.hasBoss && ctx.bossHp <= hpThreshold;
}
void HpThresholdEvent::Execute(GameContext& ctx)
{
    triggered = true;
    Broadcast(listeners, ctx);
}
bool HpThresholdEvent::isFinished() const { return triggered; }

PhaseChangeEvent::PhaseChangeEvent(int targetPhase) : targetPhase(targetPhase) {}
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
bool PhaseChangeEvent::isFinished() const { return triggered; }
