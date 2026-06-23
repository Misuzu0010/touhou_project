# StageDirector 集成说明

## 1. 目标

`StageDirector` 用来把“战斗进行到某个时间点 / 某个血量阈值 / 某个阶段切换后触发事件”这类逻辑，从 `Game.cpp` 的主循环里拆出来，避免条件判断继续堆积。

## 2. 当前接入方式

1. `Game::InitBattle()` 中创建并重置 `StageDirector`。
2. 战斗处于 `PLAYING` 状态时，由 `StageDirector::Update()` 维护 `stageTime` 并检查事件。
3. 当前已接入三类事件：
   - `TimeEvent`：到达指定战斗时间后触发。
   - `PhaseChangeEvent`：Boss 阶段切换时触发。
   - `HpThresholdEvent`：Boss 血量低于指定阈值时触发。
4. 事件通过 `GameContext` 回传信号，`Game::HandleStageSignals()` 负责消费这些信号并执行游戏内效果。

## 3. 关联文件

- `touhou_project/StageDirector.h`
- `touhou_project/StageDirector.cpp`
- `touhou_project/StageEvent.h`
- `touhou_project/StageEvent.cpp`
- `touhou_project/Game.h`
- `touhou_project/Game.cpp`

## 4. 当前用途

这次整合里，`StageDirector` 主要负责战斗中额外道具掉落的调度，让 `ui-state-polish` 分支在合并 `item-system`、`enemy-pattern-system`、`spellcard-system` 后，仍然保持主循环职责清晰。后续如果要扩展演出、额外对话或关卡事件，也可以继续复用 `StageEvent` 体系。
