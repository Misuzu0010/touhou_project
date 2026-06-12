#include "StageDirector.h"
#include "StageEvent.h"

void StageDirector::Update(float deltaTime, GameContext& ctx)
{
    ctx.deltaTime = deltaTime;

    // 指南 §2.5 暂停冻结：仅在 PLAYING 状态下推进战斗计时和事件
    if (ctx.state != State::PLAYING)
        return;

    stageTime += deltaTime;
    ctx.stageTime = stageTime;

    // 遍历事件队列：检查触发条件 → 执行（广播） → 移除已完成事件
    for (auto it = events.begin(); it != events.end(); )
    {
        bool stopAfterThisEvent = false;

        if ((*it)->ShouldTrigger(ctx))
        {
            (*it)->Execute(ctx);
            stopAfterThisEvent = ctx.finishRequested;
        }

        if ((*it)->isFinished())
        {
            it = events.erase(it);
        }
        else
        {
            ++it;
        }

        if (stopAfterThisEvent)
        {
            break;
        }
    }
}

void StageDirector::OnStateChanged(State oldState, State newState)
{
    (void)oldState;
    (void)newState;

    // 状态切换只影响 Update 是否推进时间。
    // 新战斗的事件表和时间线由 Game::SetupStageEvents 显式 Reset。
}

void StageDirector::Reset()
{
    events.clear();
    stageTime = 0.0f;
}

void StageDirector::AddEvent(std::unique_ptr<StageEvent> event)
{
    if (event)
    {
        events.push_back(std::move(event));
    }
}
