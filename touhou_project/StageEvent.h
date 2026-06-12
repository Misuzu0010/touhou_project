#pragma once
#include <functional>
#include <vector>
#include "StageDirector.h"

// ============================================================================
// StageEvent 派生类 —— 遵循指南 §4 建议的 6 种事件类型。
// 每个事件内部维护监听者列表，Execute 时广播给所有 Bind 的监听者。
// 构造函数只含触发条件参数，行为通过 Bind() 注入。
// ============================================================================

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


class DialogueEvent : public StageEvent
{
public:
    explicit DialogueEvent(float triggerTime = 0.0f);
    DialogueEvent& Bind(std::function<void(GameContext&)> listener);

    bool ShouldTrigger(const GameContext& ctx) const override;
    void Execute(GameContext& ctx) override;
    bool isFinished() const override;
private:
    float triggerTime;
    std::vector<std::function<void(GameContext&)>> listeners;
    bool triggered = false;
};


class SpawnItemEvent : public StageEvent
{
public:
    SpawnItemEvent(float x, float y, float triggerTime = 0.0f);
    SpawnItemEvent& Bind(std::function<void(GameContext&)> listener);

    bool ShouldTrigger(const GameContext& ctx) const override;
    void Execute(GameContext& ctx) override;
    bool isFinished() const override;
private:
    float spawnX, spawnY;
    float triggerTime;
    std::vector<std::function<void(GameContext&)>> listeners;
    bool triggered = false;
};


class VictoryEvent : public StageEvent
{
public:
    VictoryEvent();
    VictoryEvent& Bind(std::function<void(GameContext&)> listener);

    bool ShouldTrigger(const GameContext& ctx) const override;
    void Execute(GameContext& ctx) override;
    bool isFinished() const override;
private:
    std::vector<std::function<void(GameContext&)>> listeners;
    bool triggered = false;
};


class DefeatEvent : public StageEvent
{
public:
    DefeatEvent();
    DefeatEvent& Bind(std::function<void(GameContext&)> listener);

    bool ShouldTrigger(const GameContext& ctx) const override;
    void Execute(GameContext& ctx) override;
    bool isFinished() const override;
private:
    std::vector<std::function<void(GameContext&)>> listeners;
    bool triggered = false;
};
