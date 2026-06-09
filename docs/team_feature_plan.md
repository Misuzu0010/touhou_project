# C++ 面向对象程序设计大作业协作计划书

## 1. 项目定位

本项目是一个基于 SDL2 的东方风格弹幕射击游戏。当前版本已经具备主菜单、角色选择、剧情对话、玩家移动、玩家射击、Boss、敌弹、碰撞、道具、音效和结算等基础功能。

本次协作目标不是把项目做成大型完整游戏，而是在现有基础上做少量但质量较高的功能拓展。四个分支都应围绕 C++ 面向对象程序设计课程要求展开，重点体现封装、继承、多态、组合、资源管理和模块划分。

## 2. 总体协作原则

1. 每个人只在自己的功能分支开发，不直接向 `master` 提交代码。
2. 每个分支都必须保持可编译、可运行、可演示。
3. 每个功能都要控制范围，优先完成稳定的小功能，不追求堆功能数量。
4. 所有新增类和关键函数需要写中文注释，方便答辩和后续维护。
5. 修改 `Game.cpp` 前需要确认自己负责的区域，避免多人同时改同一段逻辑。
6. 合并前必须本地编译通过，并简单试玩对应功能。

## 3. 分支总览

| 分支名 | 任务名称 | 主要 OOP 展示点 | 难度 |
|---|---|---|---|
| `feature/item-system` | 道具系统扩展 | 继承、多态、基类指针、对象容器 | 中等 |
| `feature/enemy-pattern-system` | Boss 阶段与弹幕策略 | 封装、策略对象、阶段配置 | 中等 |
| `feature/spellcard-system` | 符卡/技能系统 | 抽象类、继承、多态、组合 | 中等 |
| `feature/ui-state-polish` | 暂停、帮助与界面状态整理 | 状态机、职责划分、菜单类封装 | 中等 |

## 4. 分支一：`feature/item-system`

### 4.1 目标

把当前单一的 P 点道具扩展成一个小型道具体系，让项目能够展示继承和多态。玩家拾取不同道具时触发不同效果。

### 4.2 建议设计

新增一个道具基类 `Item`，继承自 `Entity`。所有具体道具都继承 `Item`，并重写拾取效果。

建议类结构：

```cpp
class Item : public Entity
{
public:
    virtual ~Item() = default;
    virtual void Apply(Player& player) = 0;
    virtual void Update(float deltaTime) override;
    virtual void Render(SDL_Renderer* renderer) override;
};
```

建议派生类：

| 类名 | 效果 |
|---|---|
| `PowerItem` | 增加玩家火力 |
| `BombItem` | 增加玩家 Bomb 数量 |
| `LifeItem` | 增加玩家残机 |

### 4.3 主要修改范围

- 新增 `touhou_project/Item.h`
- 可选新增 `touhou_project/Item.cpp`
- 修改 `touhou_project/Player.h`，补充增加 Bomb、残机等接口
- 修改 `touhou_project/Game.h`，把 `powerUps` 调整为更通用的道具容器
- 修改 `touhou_project/Game.cpp`，处理道具生成、更新、碰撞和拾取逻辑

### 4.4 验收标准

1. 游戏中至少存在 3 种道具。
2. 不同道具拾取后效果不同。
3. 道具对象通过基类指针或 `std::unique_ptr<Item>` 管理。
4. 玩家拾取道具时不需要在外部直接访问玩家 public 数据成员。
5. 编译通过，拾取逻辑稳定，不崩溃。

### 4.5 答辩说明点

本分支可以重点说明：通过 `Item` 抽象基类统一道具行为，用派生类实现不同效果，`Game` 只需要管理 `Item` 指针，不需要关心具体道具类型。

## 5. 分支二：`feature/enemy-pattern-system`

### 5.1 目标

让 Boss 根据血量阶段使用不同弹幕，使战斗过程更有变化。同时把弹幕生成逻辑整理得更清楚，减少散落在 `Game.cpp` 中的判断。

### 5.2 建议设计

保留现有 `BulletPattern` 的基础上，增加阶段弹幕配置。推荐使用枚举或轻量策略类表示不同弹幕。

建议枚举：

```cpp
enum class PatternType
{
    Ring,
    Fan,
    Spiral,
    DelayedAim
};
```

可以在 `EnemyPhase` 中增加字段：

```cpp
PatternType patternType;
float shootInterval;
```

也可以进一步设计弹幕策略基类：

```cpp
class BulletPatternStrategy
{
public:
    virtual ~BulletPatternStrategy() = default;
    virtual void Generate(std::vector<std::unique_ptr<Bullet>>& bullets,
                          const Vector2D& origin,
                          Player* player) = 0;
};
```

如果时间有限，优先使用 `PatternType` 枚举方案；如果时间充足，再升级为策略类方案。

### 5.3 主要修改范围

- 修改 `touhou_project/BulletPattern.h`
- 修改 `touhou_project/Game.h` 中的 `EnemyPhase`
- 修改 `touhou_project/Game.cpp` 中 Boss 阶段切换和敌弹生成逻辑
- 必要时修改 `touhou_project/Bullet.h` 和 `touhou_project/Bullet.cpp`

### 5.4 验收标准

1. Boss 至少有 3 个血量阶段。
2. 每个阶段使用不同弹幕。
3. 弹幕数量合理，不造成明显卡顿。
4. 阶段切换后弹幕表现能明显变化。
5. 编译通过，敌弹更新和清理正常。

### 5.5 答辩说明点

本分支可以重点说明：通过阶段配置把 Boss 行为数据化，通过弹幕函数或策略对象封装不同攻击方式，降低 `Game.cpp` 的复杂度。

## 6. 分支三：`feature/spellcard-system`

### 6.1 目标

把玩家 Bomb / 符卡逻辑整理成独立的面向对象技能系统。不同角色拥有不同符卡效果，`Game` 通过统一接口调用当前符卡。

### 6.2 建议设计

新增抽象基类 `SpellCard`：

```cpp
class SpellCard
{
public:
    virtual ~SpellCard() = default;

    virtual void Activate() = 0;
    virtual void Update(float deltaTime) = 0;
    virtual void Render(SDL_Renderer* renderer) = 0;
    virtual bool IsFinished() const = 0;
};
```

建议派生类：

| 类名 | 效果 |
|---|---|
| `ReimuSpellCard` | 清除敌弹，并对 Boss 造成范围伤害 |
| `MarisaSpellCard` | 持续高伤害直线攻击 Boss |

`Game` 中建议使用：

```cpp
std::unique_ptr<SpellCard> activeSpell;
```

符卡需要访问敌人、敌弹、玩家等对象时，可以通过构造函数传入必要引用，或者设计一个简单上下文结构 `SpellContext`。

### 6.3 主要修改范围

- 新增 `touhou_project/SpellCard.h`
- 可选新增 `touhou_project/SpellCard.cpp`
- 修改 `touhou_project/Game.h`，增加当前符卡对象
- 修改 `touhou_project/Game.cpp`，把 Bomb 触发、符卡更新和符卡渲染迁移到符卡类
- 必要时修改 `touhou_project/Player.h`，补充消耗 Bomb、查询角色等接口

### 6.4 验收标准

1. 灵梦和魔理沙至少各有一种不同符卡效果。
2. 符卡通过 `SpellCard` 抽象基类统一调用。
3. `Game` 不直接写大量角色符卡细节，只负责创建、更新和渲染当前符卡。
4. 符卡结束后能够正确释放或置空。
5. 编译通过，Bomb 数量消耗正常。

### 6.5 答辩说明点

本分支可以重点说明：`SpellCard` 是抽象接口，具体符卡通过继承实现不同逻辑，运行时通过基类指针实现多态调用，符合面向对象的开闭原则。

## 7. 分支四：`feature/ui-state-polish`

### 7.1 目标

完善游戏流程，让项目演示时更完整。新增暂停界面和帮助界面，并适当整理菜单状态逻辑。

### 7.2 建议设计

在现有 `State` 枚举基础上增加：

```cpp
PAUSED,
HELP
```

为了体现面向对象，可以新增一个简单菜单类：

```cpp
class MenuPage
{
public:
    void MoveUp();
    void MoveDown();
    int GetSelectedIndex() const;
    void Render(SDL_Renderer* renderer, TTF_Font* font) const;
};
```

主菜单、暂停菜单和帮助界面都可以复用或参考这个类，避免所有菜单选项都散落在 `Game.cpp` 中。

### 7.3 主要修改范围

- 修改 `touhou_project/Game.h` 中的 `State`
- 修改 `touhou_project/Game.cpp` 中事件处理和渲染逻辑
- 可选新增 `touhou_project/MenuPage.h`
- 可选新增 `touhou_project/MenuPage.cpp`
- 修改 `README.md`，补充操作说明

### 7.4 验收标准

1. 主菜单中可以进入帮助界面。
2. 游戏进行中按 ESC 可以暂停。
3. 暂停界面可以继续游戏或返回主菜单。
4. 帮助界面能显示基本操作说明。
5. 菜单选择逻辑稳定，不影响正常战斗。

### 7.5 答辩说明点

本分支可以重点说明：通过状态枚举控制游戏流程，通过菜单类封装选项移动和渲染逻辑，避免界面状态全部混在主循环里。

## 8. 文件冲突风险与边界划分

四个分支都会不同程度接触 `Game.cpp` 和 `Game.h`，因此需要提前约定修改边界。

| 分支 | 主要负责区域 | 尽量避免修改 |
|---|---|---|
| `feature/item-system` | 道具生成、道具容器、拾取效果 | Boss 弹幕生成、菜单流程 |
| `feature/enemy-pattern-system` | Boss 阶段、敌弹生成、弹幕函数 | 玩家符卡、菜单流程 |
| `feature/spellcard-system` | Bomb 触发、符卡对象、符卡效果 | Boss 阶段配置、普通道具逻辑 |
| `feature/ui-state-polish` | 菜单、暂停、帮助、状态切换 | 战斗数值、弹幕细节 |

如果必须修改别人负责区域，应先在小组内说明修改原因，并尽量保持改动小而明确。

## 9. 推荐开发流程

每个人从最新 `master` 创建自己的分支：

```powershell
git switch master
git pull origin master
git switch -c feature/item-system
git push -u origin feature/item-system
```

其他分支同理：

```powershell
git switch -c feature/enemy-pattern-system
git push -u origin feature/enemy-pattern-system
```

```powershell
git switch -c feature/spellcard-system
git push -u origin feature/spellcard-system
```

```powershell
git switch -c feature/ui-state-polish
git push -u origin feature/ui-state-polish
```

建议提交粒度：

1. 新增类或接口，单独提交一次。
2. 接入 `Game` 主流程，单独提交一次。
3. 修复问题和补充注释，单独提交一次。
4. 最终测试通过后提交一次整理提交。

提交信息示例：

```text
feat: add item base class and derived items
feat: add boss phase bullet patterns
feat: add spell card polymorphism system
feat: add pause and help states
fix: handle item cleanup after collection
docs: update feature explanation
```

## 10. 推荐合并顺序

建议按以下顺序合并到 `master`：

1. `feature/ui-state-polish`
2. `feature/item-system`
3. `feature/enemy-pattern-system`
4. `feature/spellcard-system`

原因：

- UI 状态先合并，可以给后续功能提供更完整的演示入口。
- 道具系统相对独立，尽早合并有利于联调。
- 弹幕系统会影响 Boss 战斗节奏，建议在基础流程稳定后合并。
- 符卡系统需要和敌弹、Boss、玩家状态交互，放在最后更方便统一测试。

## 11. 统一验收要求

每个分支完成后都需要满足以下要求：

1. 项目可以成功编译。
2. 游戏可以正常进入主菜单和战斗。
3. 新功能至少能在一次试玩中稳定复现。
4. 没有明显崩溃、空指针访问或对象重复释放问题。
5. 新增类、关键成员变量和关键函数有中文注释。
6. 不新增本机绝对路径。
7. 不把编译产物、缓存文件、用户本地配置提交到仓库。

## 12. 答辩展示建议

答辩时可以按以下顺序介绍：

1. 项目整体结构：`Game` 负责主循环和状态调度，`Entity` 是游戏对象基类。
2. 封装改进：核心数据成员改为 `private` 或 `protected`，通过接口访问。
3. 道具系统：展示 `Item` 基类和多个派生道具。
4. Boss 弹幕：展示阶段配置和不同弹幕生成方式。
5. 符卡系统：展示 `SpellCard` 抽象类和角色符卡多态。
6. UI 状态：展示暂停、帮助、菜单状态切换。
7. 运行演示：选择角色、进入战斗、拾取道具、Boss 切阶段、释放符卡、暂停返回。

## 13. 时间安排建议

| 时间 | 目标 |
|---|---|
| 第 1 天 | 每个人创建分支，阅读相关文件，确定类设计 |
| 第 2 天 | 完成新增类和基础接口 |
| 第 3 天 | 接入游戏主流程，实现可运行版本 |
| 第 4 天 | 修复问题，补充中文注释，完成本地测试 |
| 第 5 天 | 按顺序合并分支，统一试玩，准备答辩说明 |

如果时间不足，优先保证每个分支都有一个完整可演示的核心功能，再考虑增加视觉效果或额外细节。
