# touhou_project API

本文档用于帮助开发者理解本项目的代码功能、接口、数据结构、资源约定和主要运行流程。

## 1. 项目概览

`touhou_project` 是一个基于 SDL2 的弹幕射击小游戏。项目使用 Visual Studio C++ 工程组织代码，入口为 `main.cpp`，核心运行逻辑集中在 `Game` 类中。

主要特性：

- 1920 x 1080 固定窗口。
- 主菜单、音量设置、角色选择、剧情对话、战斗、失败、胜利等状态。
- 可选角色：灵梦 `REIMU`、魔理沙 `MARISA`。
- Boss 根据血量进入不同弹幕阶段。
- 玩家射击、P 点火力成长、Bomb/符卡、残机与续关机制。
- SDL_image 加载图片，SDL_ttf 渲染中文文本，SDL_mixer 播放 BGM 和音效。

## 2. 项目结构

```text
touhou_project/
+-- docs/
|   +-- API.md
+-- touhou_project.sln
+-- touhou_project/
    +-- main.cpp
    +-- Game.h / Game.cpp
    +-- Entity.h
    +-- Player.h
    +-- Enemy.h
    +-- Bullet.h / Bullet.cpp
    +-- BulletPattern.h
    +-- PowerUp.h
    +-- *.png
    +-- assets/
    |   +-- audios/
    |   +-- fonts/
    |   +-- sfx/
    +-- touhou_project.vcxproj
```

`touhou_project/x64/` 和根目录 `x64/` 是 Visual Studio 构建输出，不属于源码。

## 3. 构建与依赖

工程文件：`touhou_project/touhou_project.vcxproj`

当前工程依赖：

- SDL2
- SDL2_image
- SDL2_ttf
- SDL2_mixer

当前 Release/Debug x64 配置中包含的库：

```text
SDL2.lib
SDL2main.lib
SDL2_image.lib
SDL2_ttf.lib
SDL2_mixer.lib
```

工程里使用了本机绝对路径，例如：

```text
D:\SDL2-devel-2.32.10-VC\SDL2-2.32.10\include
D:\SDL2_image-devel-2.8.5-VC\SDL2_image-2.8.5\include
D:\SDL_ttf\SDL2_ttf-2.24.0\include
D:\SDL_Mixer\SDL2_mixer-2.8.1\include
```

如果换机器构建，需要同步修改 Visual Studio 的包含目录和库目录，或统一迁移到相对路径 / vcpkg / NuGet。

## 4. 资源路径约定

`Game::Init()` 中直接以相对路径加载资源。运行目录需要能访问这些文件。

根目录图片资源：

| 资源 | 用途 |
| --- | --- |
| `Reimu.png` | 灵梦玩家背身贴图 |
| `Marisa.png` | 魔理沙玩家背身贴图 |
| `Reimu_Enemy.png` | 灵梦 Boss 正面贴图 |
| `Marisa_Enemy.png` | 魔理沙 Boss 正面贴图 |
| `EnemyBullet.png` | 敌方弹幕图集 |
| `PlayerBullet.png` | 玩家弹幕和 P 点图集 |
| `TitleBg.png` | 主菜单 / 角色选择背景 |
| `BattleBg.png` | 战斗 / 对话 / 结算背景 |

字体资源：

| 资源 | 用途 |
| --- | --- |
| `assets/fonts/SimHei.ttf` | 当前 UI 和中文文本字体 |
| `assets/fonts/NotoSerifCJKjp-VF.ttf` | 已纳入资源，但当前代码未直接加载 |

音乐资源：

| 资源 | 用途 |
| --- | --- |
| `assets/audios/Dream Battle.mp3` | 主菜单 BGM |
| `assets/audios/Necro-Fantasy.mp3` | 战斗 BGM |

音效资源：

| 资源 | 用途 |
| --- | --- |
| `assets/sfx/player_shoot.wav` | 玩家射击 |
| `assets/sfx/enemy_shoot1.wav` | Boss 常规射击 |
| `assets/sfx/enemy_shoot2.wav` | Boss 暴走射击 |
| `assets/sfx/playerdead.wav` | 玩家彻底失败 |
| `assets/sfx/playerbehitted.wav` | 玩家被击中 |
| `assets/sfx/power_up.wav` | 火力升级 |
| `assets/sfx/select.wav` | 菜单选择 |
| `assets/sfx/reimubomb.wav` | 灵梦 Bomb |
| `assets/sfx/marisabomb.wav` | 魔理沙 Bomb |
| `assets/sfx/victory.mov` | 胜利音效 |

## 5. 程序生命周期

入口：`touhou_project/main.cpp`

```cpp
int main(int argc, char* argv[])
{
    Game game;
    if (game.Init()) {
        game.Run();
    }
    game.Clean();
    return 0;
}
```

生命周期：

1. `Game::Init()` 初始化 SDL、窗口、渲染器、资源、音乐和初始状态。
2. `Game::Run()` 进入主循环。
3. 主循环每帧执行：
   - `HandleEvents()`
   - `Update(deltaTime)`
   - `Render()`
4. `Game::Clean()` 释放音乐、音效、窗口、渲染器和 SDL 子系统。

## 6. 状态机

状态定义在 `Game.h`：

```cpp
enum class State {
    MAIN_MENU,
    VOLUME_SETTINGS,
    SELECT_CHARACTER,
    DIALOGUE,
    PLAYING,
    GAME_OVER,
    VICTORY
};
```

状态说明：

| 状态 | 作用 |
| --- | --- |
| `MAIN_MENU` | 主菜单，包含开始、音量设置、退出 |
| `VOLUME_SETTINGS` | 调节 BGM 和 SFX 音量 |
| `SELECT_CHARACTER` | 左右选择灵梦或魔理沙 |
| `DIALOGUE` | 剧情对话，战斗阶段切换时也会进入 |
| `PLAYING` | 战斗主循环 |
| `GAME_OVER` | 失败界面，支持倒计时续关 |
| `VICTORY` | 胜利界面 |

常见状态流转：

```text
MAIN_MENU
  -> SELECT_CHARACTER
  -> DIALOGUE
  -> PLAYING
  -> DIALOGUE      阶段切换
  -> PLAYING
  -> VICTORY 或 GAME_OVER
```

`DIALOGUE` 使用 `stateBeforeDialogue` 记录对话结束后回到哪个状态。

## 7. 输入控制

| 按键 | 场景 | 功能 |
| --- | --- | --- |
| 方向键 上/下 | 主菜单、音量设置 | 切换选项 |
| 方向键 左/右 | 音量设置 | 调节音量 |
| 方向键 左/右 | 角色选择 | 切换角色 |
| 方向键 | 战斗 | 玩家移动 |
| 左 Shift | 战斗 | 低速移动并显示判定点 |
| Z | 菜单 / 对话 | 确认或推进对话 |
| Z | 战斗 | 玩家射击 |
| X | 战斗 | 使用 Bomb/符卡 |
| R | GAME_OVER | 续关 |
| ESC | 多个界面 | 返回主菜单或退出当前界面 |

按键防抖主要通过 `Update()` 中的静态变量 `keyLock`、`zPressed`、`xPressed`、`escPressed`、`rPressed` 实现。

## 8. 核心数据结构

### Vector2D

位置和速度的二维向量。

```cpp
class Vector2D {
public:
    float x, y;
};
```

### DialogueLine

剧情对话单行数据。

```cpp
struct DialogueLine {
    std::string name;
    std::string Content;
    SDL_Color color;
};
```

字段：

| 字段 | 说明 |
| --- | --- |
| `name` | 说话者名称 |
| `Content` | 对话内容 |
| `color` | 名称显示颜色 |

### EnemyPhase

Boss 阶段配置。

```cpp
struct EnemyPhase {
    float hpThreshold;
    bool dialogueTriggered;
    std::vector<DialogueLine> dialogues;
};
```

字段：

| 字段 | 说明 |
| --- | --- |
| `hpThreshold` | 血量低于该阈值时触发阶段 |
| `dialogueTriggered` | 阶段对话是否已触发 |
| `dialogues` | 阶段切换对话 |

当前阶段阈值：

| 阶段 | 血量阈值 | 效果 |
| --- | --- | --- |
| 阶段 1 | `<= 4000` | 触发锁弹幕对话并清空敌方子弹 |
| 阶段 2 | `<= 2000` | 触发暴走对话并清空敌方子弹 |

## 9. Entity 基类

文件：`touhou_project/Entity.h`

所有可移动、可碰撞游戏对象的基类。

主要字段：

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `Position` | `Vector2D` | 坐标 |
| `Velocity` | `Vector2D` | 速度 |
| `radius` | `float` | 圆形碰撞半径 |
| `active` | `bool` | 是否有效 |

主要接口：

```cpp
Entity(float x, float y, float r);
virtual ~Entity();
virtual void Update(float deltaTime);
virtual void Render(SDL_Renderer* renderer);
bool CheckCollision(Entity* Other);
```

说明：

- `Update()` 默认按速度更新位置。
- `Render()` 是虚函数，子类可以覆盖。
- `CheckCollision()` 使用圆形碰撞：两对象距离小于半径和即命中。

## 10. Player

文件：`touhou_project/Player.h`

玩家实体，继承自 `Entity`。

初始值：

| 字段 | 初始值 | 说明 |
| --- | --- | --- |
| `lives` | `3` | 残机 |
| `powerLevel` | `0` | 火力等级 |
| `bombs` | `1` | Bomb 数 |
| `powerValue` | `0.0f` | 火力经验 |
| `attack_point` | `20` | 单发玩家子弹对 Boss 伤害 |
| `radius` | `3.0f` | 玩家判定点半径 |

主要接口：

```cpp
Player(float x, float y, SDL_Texture* tex);
void ResetPosition();
void Update(float deltaTime) override;
void Render(SDL_Renderer* renderer) override;
void hit(float damage);
bool Dead_judge();
int CollectPowerUp();
```

行为：

- `Update()` 处理玩家移动、边界限制、无敌闪烁、角色倾斜和呼吸动画。
- 默认速度 `800`，按住左 Shift 时速度为 `400`。
- 玩家边界被限制在 `x: 30..1890`、`y: 30..1050`。
- `ResetPosition()` 将玩家移动到 `(960, 900)`，并给予 `3` 秒无敌。
- `Dead_judge()` 在 `lives < 0` 时判定彻底失败。
- `CollectPowerUp()` 每个 P 点增加 `0.05`，累计到 `1.0` 后 `powerLevel++`。

`CollectPowerUp()` 返回值：

| 返回值 | 说明 |
| --- | --- |
| `0` | 已达到最高等级，不再成长 |
| `1` | 本次拾取导致升级 |
| `2` | 拾取成功但未升级 |

## 11. Enemy

文件：`touhou_project/Enemy.h`

Boss 实体，继承自 `Entity`。

初始值：

| 字段 | 初始值 | 说明 |
| --- | --- | --- |
| `Blood` | `6000` | Boss 血量 |
| `attack_point` | `5` | 预留攻击力字段 |
| `radius` | `40` | 碰撞半径 |

主要接口：

```cpp
Enemy(float x, float y, SDL_Texture* tex);
void Render(SDL_Renderer* r) override;
void hit(float attack);
float GetBlood();
```

行为：

- `Render()` 以高度 `250` 强制缩放 Boss 贴图。
- 如果没有贴图，则绘制红色矩形作为占位。
- `hit()` 扣除血量。

## 12. Bullet

文件：`touhou_project/Bullet.h`、`touhou_project/Bullet.cpp`

子弹实体，继承自 `Entity`。

### BulletType

```cpp
enum class BulletType {
    VIRUS,
    LOCK,
    SHURIKEN,
    PLAYER_TALISMAN,
    PLAYER_STAR
};
```

| 类型 | 来源 | 贴图含义 |
| --- | --- | --- |
| `VIRUS` | 敌人 | 病毒球 |
| `LOCK` | 敌人 | 锁 |
| `SHURIKEN` | 敌人 | 手里剑 |
| `PLAYER_TALISMAN` | 玩家灵梦 | 符札 |
| `PLAYER_STAR` | 玩家魔理沙 | 星星 |

### BulletState

```cpp
enum class BulletState {
    NORMAL,
    WAITING,
    AIMING,
    UNFOLDING
};
```

| 状态 | 说明 |
| --- | --- |
| `NORMAL` | 正常按速度移动 |
| `WAITING` | 停顿，等待后瞄准玩家 |
| `AIMING` | 当前未使用 |
| `UNFOLDING` | 从中心展开，结束后进入 `WAITING` |

主要接口：

```cpp
Bullet(float x, float y, float speed_x, float speed_y, BulletType type = BulletType::VIRUS);
Bullet(float x, float y, float waitTime, float speed, Player* player, BulletType type = BulletType::VIRUS);
void Update(float deltaTime) override;
void Render(SDL_Renderer* renderer, SDL_Texture* texture);
```

行为：

- 速度构造函数用于普通直线弹。
- 等待构造函数用于停顿后朝玩家发射的追踪弹。
- `UNFOLDING` 会先展开一段距离，再切换为 `WAITING`。
- 子弹越界后 `active = false`，边界为 `x: -100..2000`、`y: -100..1150`。
- 渲染时根据 `BulletType` 从图集中切片，并按速度方向旋转。

## 13. BulletPattern

文件：`touhou_project/BulletPattern.h`

弹幕生成工具类，不持有状态，直接向 `std::vector<Bullet*>` 中追加子弹。

接口：

```cpp
void ShootRotatingRing(
    float now_x,
    float now_y,
    float now_v,
    int bullet_cnt,
    float offset_angle,
    std::vector<Bullet*>& bullets,
    BulletType type = BulletType::VIRUS
);
```

生成圆环弹幕。`bullet_cnt` 控制密度，`offset_angle` 控制旋转偏移。

```cpp
void ShootSpiral(
    float now_x,
    float now_y,
    float speed,
    float angle_start,
    int count,
    float angle_step,
    std::vector<Bullet*>& bullets,
    BulletType type
);
```

生成螺旋喷射弹幕。适合高频 Boss 暴走阶段。

```cpp
void ShootStopAndGo(
    float now_x,
    float now_y,
    float waitTime,
    float attackSpeed,
    Player* player,
    int bullet_cnt,
    std::vector<Bullet*>& bullets,
    BulletType type = BulletType::LOCK
);
```

生成先展开、再停顿、最后瞄准玩家的弹幕。当前用于金锁阶段。

## 14. PowerUp

文件：`touhou_project/PowerUp.h`

P 点道具实体，继承自 `Entity`。

主要接口：

```cpp
PowerUp(float x, float y, SDL_Texture* tex);
void Update(float deltaTime) override;
void Render(SDL_Renderer* renderer);
```

行为：

- 初始向上弹起：`Velocity.y = -400`。
- 横向速度为 `-150..150` 的随机值。
- 重力为 `800`。
- 最大下落速度限制为 `300`。
- 左右边界 `x: 10..1910` 反弹。
- 掉出 `y > 1150` 后失效。
- 渲染时复用 `PlayerBullet.png` 图集第一行第一列，显示为 `40 x 40`。

## 15. Game

文件：`touhou_project/Game.h`、`touhou_project/Game.cpp`

`Game` 是项目的总控制类，负责：

- SDL 子系统初始化与退出。
- 资源加载和释放。
- 主循环。
- 状态机更新。
- 输入处理。
- 战斗对象创建与清理。
- 对话、Boss 阶段、弹幕、碰撞和结算。
- UI 和场景渲染。

### 主要字段

| 字段 | 说明 |
| --- | --- |
| `CurrentState` | 当前游戏状态 |
| `DialoueQueue` | 当前对话队列，字段名存在拼写问题但代码正常使用 |
| `cur_index` | 当前对话行索引 |
| `cur_Window` | SDL 窗口 |
| `cur_Renderer` | SDL 渲染器 |
| `is_Running` | 主循环是否继续 |
| `player` | 玩家对象 |
| `Enemies` | Boss 列表，当前主要使用 `Enemies[0]` |
| `playerBullets` | 玩家子弹 |
| `enemyBullets` | 敌方子弹 |
| `powerUps` | P 点道具 |
| `menuSelect` | 主菜单选项 |
| `menuCursor` | 角色选择光标 |
| `selectedCharID` | 当前玩家角色 |
| `enemyPhases` | Boss 阶段配置 |
| `currentPhaseIndex` | 当前 Boss 阶段索引 |
| `shootTimer` | 玩家射击冷却 |
| `enemyShootTimer` | Boss 射击冷却 |
| `powerUpSpawnTimer` | P 点生成计时器 |
| `continueTimer` | 失败续关倒计时 |
| `spellTimer` | Bomb/符卡持续时间 |
| `isSpellActive` | 符卡是否激活 |
| `spellUser` | 当前符卡使用角色 |

### 主要接口

```cpp
Game();
bool Init();
void Run();
void Clean();
void HandleEvents();
void Update(float DeltaTime);
void Render();
void InitBattle(CharacterID playerID);
void SetupDialogue(CharacterID playerID);
void SetupEnemyPhases(CharacterID playerID);
void CheckEnemyPhase();
SDL_Texture* LoadTextureWithColorKey(const char* filename);
```

### Init()

初始化流程：

1. `SDL_Init(SDL_INIT_VIDEO)`
2. `IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG)`
3. `TTF_Init()`
4. `Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048)`
5. 创建 `1920 x 1080` 窗口和硬件加速 VSync 渲染器。
6. 加载贴图、字体、BGM、音效。
7. 设置音量并循环播放主菜单 BGM。

### InitBattle(CharacterID playerID)

初始化一场战斗：

- 清理旧玩家、敌人、玩家子弹和敌方子弹。
- 根据玩家角色生成玩家与对手 Boss：
  - 玩家选择灵梦时，Boss 是魔理沙。
  - 玩家选择魔理沙时，Boss 是灵梦。
- 玩家出生点 `(960, 900)`。
- Boss 出生点 `(960, 200)`。
- 初始化开场对话和 Boss 阶段。
- 重置射击与 P 点计时器。

### SetupDialogue(CharacterID playerID)

设置开场对话，并进入 `DIALOGUE` 状态。对话结束后回到 `PLAYING`。

### SetupEnemyPhases(CharacterID playerID)

配置 Boss 阶段，目前有两个阈值：

- `4000` 血：防火墙突破 / 加密锁阶段。
- `2000` 血：内核异常 / 暴走阶段。

### CheckEnemyPhase()

每帧检查 Boss 当前血量。如果满足阶段阈值：

- 更新 `currentPhaseIndex`。
- 切换到 `DIALOGUE`。
- 加载阶段对话。
- 清空所有敌方子弹，实现阶段转换消弹。

### Update(float DeltaTime)

根据 `CurrentState` 分发逻辑。

战斗中主要流程：

1. 更新玩家。
2. 处理玩家射击。
3. 随机生成 P 点。
4. 处理 Bomb/符卡。
5. 执行 Boss AI 和弹幕生成。
6. 更新子弹和道具。
7. 检查玩家子弹打 Boss。
8. 检查敌方子弹打玩家。
9. 检查拾取 P 点。
10. 检查 Boss 阶段。
11. 检查胜利。

### Render()

根据状态绘制：

- 主菜单背景和菜单文字。
- 音量设置遮罩和选项。
- 角色选择界面。
- 战斗背景、玩家、Boss、子弹、P 点。
- Bomb/符卡特效。
- 残机、Power、Spell UI。
- Boss 血条和阶段标记。
- 对话遮罩和对话框。
- 失败 / 胜利结算界面。

## 16. 战斗规则和关键数值

| 项目 | 当前值 |
| --- | --- |
| 窗口大小 | `1920 x 1080` |
| 玩家出生点 | `(960, 900)` |
| Boss 出生点 | `(960, 200)` |
| Boss 初始血量 | `6000` |
| 玩家残机 | `3` |
| 玩家 Bomb | `1` |
| 玩家普通速度 | `800` |
| 玩家低速速度 | `400` |
| 玩家射击间隔 | `0.07s` |
| 玩家子弹基础速度 | `-600` |
| 玩家单发伤害 | `20` |
| P 点每次成长 | `0.05` |
| P 点生成间隔 | 随机 `4.0..5.9s` |
| Bomb 持续时间 | `3.0s` |
| Bomb 无敌时间 | `3.5s` |
| 被击中后无敌时间 | `3.0s` |
| 失败续关倒计时 | `10.0s` |

Boss 弹幕：

| 阶段 | 条件 | 弹幕 |
| --- | --- | --- |
| 阶段 0 | 初始 | `VIRUS` 圆环弹，36 发，速度 300，间隔 0.5s |
| 阶段 1 | 血量 <= 4000 | `LOCK` 展开停顿追踪弹，24 发，速度 600，间隔 0.8s |
| 阶段 2 | 血量 <= 2000 | `SHURIKEN` 螺旋弹，每次 5 发，速度 500，间隔 0.05s |

## 17. Bomb/符卡逻辑

按 `X` 且 `player->bombs > 0` 时触发。

通用效果：

- 消耗 1 个 Bomb。
- `isSpellActive = true`。
- 持续 `3.0s`。
- 玩家获得 `3.5s` 无敌。
- 每帧对 Boss 造成 `10` 伤害。

灵梦：

- 符卡名：`灵符「梦想封印」`
- 效果：全屏敌方子弹失效。
- 渲染：8 个粉色方块模拟光玉。

魔理沙：

- 符卡名：`恋符「Master Spark」`
- 效果：清除玩家 X 轴附近 `100` 像素范围内的敌方子弹。
- 渲染：纵向激光矩形。

## 18. 碰撞规则

默认碰撞来自 `Entity::CheckCollision()`：

```text
distance(PositionA, PositionB) < radiusA + radiusB
```

当前碰撞关系：

- 玩家子弹 vs Boss：命中后 Boss 扣血，玩家子弹失效。
- 敌方子弹 vs 玩家：玩家非无敌时命中，玩家残机减 1，当前屏幕敌弹失效。
- 玩家 vs P 点：当前实际代码仍使用圆形碰撞 `player->CheckCollision(p)`。

注意：`Game::Update()` 中计算了 P 点 AABB 拾取范围 `pickUpWidth` / `pickUpHeight`，但最终判断仍是 `player->CheckCollision(p)`。如果希望角色边缘也能拾取，需要把最终判断改为 `isColliding`。

## 19. 内存和资源管理

当前对象多使用裸指针：

```cpp
Player* player;
std::vector<Enemy*> Enemies;
std::vector<Bullet*> playerBullets;
std::vector<Bullet*> enemyBullets;
std::vector<PowerUp*> powerUps;
```

对象创建方式：

- `new Player(...)`
- `new Enemy(...)`
- `new Bullet(...)`
- `new PowerUp(...)`

对象失效方式：

- 子弹和道具越界后设置 `active = false`。
- 命中后设置 `active = false`。
- 阶段切换时直接删除并清空敌方子弹。

当前注意事项：

- `Update()` 中没有持续 `erase/remove_if` 清理失效对象，失效子弹和 P 点会留在 vector 中直到 `Clean()` 或阶段切换。
- `Clean()` 释放了音乐、音效、渲染器、窗口和部分 SDL 子系统，但没有显式释放所有纹理和 `player` / `Enemies` 中的对象。
- 若继续扩展，建议用 `std::unique_ptr` 或集中对象池管理，减少手动 `new/delete` 风险。

## 20. 扩展指南

### 新增角色

需要修改：

1. `CharacterID` 增加新枚举值。
2. `Game::Init()` 加载新角色贴图。
3. `Game::Render()` 的角色选择界面增加显示逻辑。
4. `Game::InitBattle()` 增加玩家和 Boss 对应关系。
5. `Game::SetupDialogue()` 增加开场对话。
6. 玩家子弹类型可能需要在 `BulletType` 和 `Bullet::Render()` 中扩展。

### 新增 Boss 阶段

需要修改：

1. `Game::SetupEnemyPhases()` 增加 `EnemyPhase` 配置。
2. `Game::Update()` 的 Boss AI 根据新的 `currentPhaseIndex` 添加弹幕逻辑。
3. `Game::Render()` 的 Boss 血条阶段标记需要增加对应阈值线。

### 新增弹幕类型

需要修改：

1. `BulletType` 增加枚举。
2. `Bullet::Render()` 增加贴图切片和显示大小逻辑。
3. `BulletPattern` 增加生成函数，或复用已有函数传入新类型。
4. `Game::Update()` 的 Boss AI 调用新弹幕。

### 新增资源

建议遵守当前资源路径：

```text
assets/audios/  BGM
assets/sfx/     音效
assets/fonts/   字体
```

如果是运行时直接加载的根目录图片，需要确保编译后的工作目录中也能找到这些图片。

## 21. 已知代码注意点

- `DialoueQueue` 字段名拼写应为 `DialogueQueue`，但当前项目中一致使用旧拼写，改名需要同步所有引用。
- `BulletState::AIMING` 当前未使用。
- `Player::Dead_judge()` 使用 `lives < 0`，所以 UI 显示 0 残机时还不一定立刻彻底失败。
- 音量设置中 BGM 右调上限是 `64`，但显示百分比按 `128` 计算，所以最大显示约 50%。
- `Game::Init()` 只初始化了 `SDL_INIT_VIDEO`，音频由 `Mix_OpenAudio()` 打开；如果后续需要 SDL 音频事件，可考虑加入 `SDL_INIT_AUDIO`。
- `LoadTextureWithColorKey()` 只将纯白 `RGB(255,255,255)` 设为透明，不会处理接近白色或灰色背景。
- `victory.mov` 通过 `Mix_LoadWAV()` 加载，是否稳定取决于 SDL_mixer 对该文件格式的支持。
- 工程依赖路径是本机绝对路径，协作开发时建议统一依赖管理。

## 22. 快速定位表

| 想改的功能 | 优先查看文件 |
| --- | --- |
| 主循环 / 状态机 | `Game.cpp`, `Game.h` |
| 玩家移动 / 判定 / 火力 | `Player.h` |
| Boss 血量 / 渲染 | `Enemy.h` |
| 子弹行为 / 图集切片 | `Bullet.cpp`, `Bullet.h` |
| Boss 弹幕模式 | `BulletPattern.h`, `Game.cpp` |
| P 点物理和渲染 | `PowerUp.h` |
| 资源加载 | `Game::Init()` |
| 菜单和 UI 渲染 | `Game::Render()` |
| 对话和阶段切换 | `SetupDialogue()`, `SetupEnemyPhases()`, `CheckEnemyPhase()` |
