#include "StageDirector.h"

void StageDirector::Update(float deltaTime, GameContext& ctx)
{
    ctx.deltaTime = deltaTime;
    if (!ctx.isPlaying) {
        return;
    }

    stageTime += deltaTime;
    ctx.stageTime = stageTime;

    for (auto it = events.begin(); it != events.end(); ) {
        if ((*it)->ShouldTrigger(ctx)) {
            (*it)->Execute(ctx);
        }

        if ((*it)->isFinished()) {
            it = events.erase(it);
        } else {
            ++it;
        }
    }
}

void StageDirector::Reset()
{
    events.clear();
    stageTime = 0.0f;
}

void StageDirector::AddEvent(std::unique_ptr<StageEvent> event)
{
    if (event) {
        events.push_back(std::move(event));
    }
}
