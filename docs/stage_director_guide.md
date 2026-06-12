# StageDirector 技术指导书

## 1. 目标

本功能不是做一个“按秒触发”的计时器，而是做一个**状态驱动的关卡调度器**。  
它负责根据当前游戏状态和战斗条件，统一触发剧情、弹幕切换、道具生成、胜负结算等事件。

## 2. 核心原则

1. **先看状态，再看条件**：只有在 `PLAYING` 状态下才推进战斗事件。
2. **时间只是条件之一**：`stageTime` 只用于战斗流程内部，不是唯一驱动。
3. **事件独立**：每个事件单独封装，不把一堆判断写死在 `Game.cpp`。
4. **一次触发**：大多数事件只触发一次，避免重复执行。
5. **暂停冻结**：进入 `PAUSED`、`DIALOGUE`、`GAME_OVER`、`VICTORY` 时，不推进战斗计时。

## 3. 建议结构

### 3.1 `GameContext`

作为 `Game` 和 `StageDirector` 之间的只读上下文，传递当前状态和必要战斗信息。

```cpp
struct GameContext
{
    State state;
    float stageTime;
    float deltaTime;
    float bossHp;
    int phaseIndex;
};
```

如果后续需要触发具体动作，可以再补充最小必要引用，不要把整个 `Game` 全部暴露出去。

### 3.2 `StageEvent`

所有事件的抽象基类，统一行为接口。

```cpp
class StageEvent
{
public:
    virtual ~StageEvent() = default;
    virtual bool ShouldTrigger(const GameContext& ctx) const = 0;
    virtual void Execute(GameContext& ctx) = 0;
    virtual bool IsFinished() const = 0;
};
```

### 3.3 `StageDirector`

负责管理事件列表、更新事件、处理状态切换。

```cpp
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
```

## 4. 建议事件类型

1. `TimeEvent`：进入战斗后，满足时间条件触发。
2. `HpThresholdEvent`：Boss 血量低于某值触发。
3. `PhaseChangeEvent`：阶段号变化时触发。
4. `DialogueEvent`：进入某阶段后播放对话。
5. `SpawnItemEvent`：某条件满足时生成道具。
6. `VictoryEvent` / `DefeatEvent`：胜负状态触发。

## 5. 游戏流程

推荐流程如下：

1. `Game::Update()` 先更新当前全局状态。
2. 构造 `GameContext`。
3. 如果状态变化，调用 `StageDirector::OnStateChanged()`。
4. 如果当前是 `PLAYING`，调用 `StageDirector::Update()`。
5. `StageDirector` 判断事件是否触发。
6. 事件只负责发起动作，不负责管理整套战斗主循环。

## 6. 文件职责

- `touhou_project/StageDirector.h`：导演类声明。
- `touhou_project/StageDirector.cpp`：导演逻辑实现。
- `touhou_project/StageEvent.h`：事件基类和派生事件声明。
- `touhou_project/Game.h`：挂接 `StageDirector` 成员。
- `touhou_project/Game.cpp`：构建上下文并调用导演。

## 7. 开发顺序建议

1. 先实现 `GameContext` 和 `StageEvent` 基类。
2. 再实现一个最简单的时间事件和血量事件。
3. 接入 `Game::Update()`，验证状态切换是否正确。
4. 再补对话事件、弹幕切换事件、道具事件。
5. 最后处理暂停、重开、胜利、失败时的重置逻辑。

## 8. 验收标准

1. 关卡事件只在正确状态下运行。
2. 暂停后事件计时冻结。
3. 至少支持 3 种事件触发方式。
4. `Game.cpp` 中不再堆满硬编码判断。
5. 事件系统可以独立扩展，不改主循环也能加新事件。

## 9. 答辩表述建议

可以这样描述：

“我把关卡流程从 `Game.cpp` 中拆出来，设计成状态驱动的 `StageDirector`。  
`Game` 负责提供上下文，`StageDirector` 负责判断何时触发事件，具体事件通过多态实现。  
这样既符合面向对象设计，也让战斗流程更容易扩展和维护。”
