#pragma once
#include <functional>
#include <vector>
#include "StageDirector.h"

class TimeEvent : public StageEvent
{
public:
    explicit TimeEvent(float triggerTime);
    TimeEvent& Bind(std::function<void(GameContext&)> listener);
    bool ShouldTrigger(const GameContext& ctx) const override;
    void Execute(GameContext& ctx) override;
    bool isFinished() const override;
private:
    float triggerTime;
    std::vector<std::function<void(GameContext&)>> listeners;
    bool triggered = false;
};

class HpThresholdEvent : public StageEvent
{
public:
    explicit HpThresholdEvent(float hpThreshold);
    HpThresholdEvent& Bind(std::function<void(GameContext&)> listener);
    bool ShouldTrigger(const GameContext& ctx) const override;
    void Execute(GameContext& ctx) override;
    bool isFinished() const override;
private:
    float hpThreshold;
    std::vector<std::function<void(GameContext&)>> listeners;
    bool triggered = false;
};

class PhaseChangeEvent : public StageEvent
{
public:
    explicit PhaseChangeEvent(int targetPhase);
    PhaseChangeEvent& Bind(std::function<void(GameContext&)> listener);
    bool ShouldTrigger(const GameContext& ctx) const override;
    void Execute(GameContext& ctx) override;
    bool isFinished() const override;
private:
    int targetPhase;
    std::vector<std::function<void(GameContext&)>> listeners;
    bool triggered = false;
};
