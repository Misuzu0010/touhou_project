# wait_for_debug

本文件记录当前代码扫描发现的待修复 bug。每条包含复现情况、重要程度和修改建议。

## 本次修复状态

- 状态：已修复 BUG-001 到 BUG-013。
- 修复时间：2026-06-09。
- 回归检查：`powershell -ExecutionPolicy Bypass -File tests\bug_regression_checks.ps1` 通过。
- 构建验证：当前项目已验证可在配置好 SDL2 依赖后使用 `x64 / Release` 成功编译运行；本地环境搭建步骤已统一写入 `README.md`。
- 备注：`victory.mov` 实际文件头是 Ogg 音频，已重命名为 `victory.ogg` 并更新加载路径。

## BUG-001 Player 动画字段未初始化

- 重要程度：高
- 位置：`touhou_project/Player.h`
- 相关代码：`visualTime`、`currentAngle` 在构造函数中未初始化，但在 `Update()` 和 `Render()` 中直接参与计算。

复现情况：

1. 启动游戏。
2. 进入角色选择并开始战斗。
3. 玩家角色初始渲染时可能出现异常倾斜、缩放节奏异常，或在 Debug/静态分析环境中报告使用未初始化变量。

原因：

- `visualTime` 被用于 `std::sin(visualTime)`。
- `currentAngle` 被用于 `SDL_RenderCopyEx()` 的旋转角度。
- 两者没有在 `Player` 构造函数初始化。

修改建议：

```cpp
Player(float x, float y, SDL_Texture* tex)
    : Entity(x, y, 3.0f),
      lives(3),
      powerLevel(0),
      bombs(1),
      powerValue(0.0f),
      attack_point(20),
      texture(tex),
      invincTimer(0.0f),
      isInvincible(false),
      flashTimer(0.0f),
      visualTime(0.0f),
      currentAngle(0.0)
{
}
```

## BUG-002 失效子弹和 P 点不会在游戏过程中清理

- 重要程度：高
- 位置：`touhou_project/Game.cpp`
- 相关代码：`Update()` 每帧仍然遍历 `playerBullets`、`enemyBullets`、`powerUps`，但失效对象只设置 `active = false`，没有在运行中移除。

复现情况：

1. 进入战斗。
2. 长时间按住 `Z` 射击，或进入 Boss 第二阶段产生大量弹幕。
3. 子弹飞出屏幕后变为 `active = false`，但仍留在 vector 中。
4. 游玩几分钟后对象数量持续增加，可能出现帧率下降、内存增长、碰撞/更新循环越来越慢。

原因：

- `Bullet::Update()` 越界后只做 `active = false`。
- `Game::Update()` 中没有执行 `erase/remove_if` 清理。
- `Game::Clean()` 里也只删除 inactive 对象，active 对象仍可能泄漏。

修改建议：

- 在 `Game::Update()` 战斗逻辑末尾定期清理 inactive 对象。
- 清理前先 `delete` 指针，再从 vector 中移除。
- 或改用 `std::unique_ptr` 管理生命周期。

示例方向：

```cpp
auto clearInactiveBullets = [](std::vector<Bullet*>& bullets) {
    bullets.erase(std::remove_if(bullets.begin(), bullets.end(),
        [](Bullet* b) {
            if (!b->active) {
                delete b;
                return true;
            }
            return false;
        }), bullets.end());
};
```

## BUG-003 续关倒计时不会在续关或重新开局时重置

- 重要程度：高
- 位置：`touhou_project/Game.cpp`
- 相关代码：`continueTimer` 只在倒计时归零自动回主菜单时重置。

复现情况 A：

1. 进入战斗并死亡到 `GAME_OVER`。
2. 等待 8 秒。
3. 按 `R` 续关。
4. 再次快速死亡。
5. 新的 `GAME_OVER` 倒计时可能只剩约 2 秒，而不是重新从 10 秒开始。

复现情况 B：

1. 进入 `GAME_OVER` 后等待几秒。
2. 按 `ESC` 返回主菜单。
3. 重新开始一局并死亡。
4. 倒计时沿用上一次剩余时间。

原因：

- `continueTimer` 是 `Game` 成员变量。
- 按 `R` 续关、按 `ESC` 返回主菜单、`InitBattle()` 新开战斗时都没有把它重置为 `10.0f`。

修改建议：

- 在进入 `GAME_OVER` 的瞬间设置 `continueTimer = 10.0f`。
- 在 `InitBattle()` 中也重置一次。
- 按 `R` 续关成功后重置，避免下一次死亡继承旧值。
- 按 `ESC` 从结算界面返回菜单时也重置。

## BUG-004 重新开始战斗时没有清理旧的 P 点

- 重要程度：中
- 位置：`touhou_project/Game.cpp`
- 相关代码：`Game::InitBattle()` 清理了 `player`、`Enemies`、`playerBullets`、`enemyBullets`，但没有清理 `powerUps`。

复现情况：

1. 进入战斗。
2. 等待 P 点生成。
3. 在 P 点还存在时死亡或返回主菜单。
4. 重新选择角色开始战斗。
5. 上一局残留的 P 点可能继续出现在新战斗中，或继续参与更新。

原因：

- `powerUps` vector 没有在 `InitBattle()` 中 delete 和 clear。

修改建议：

```cpp
for (auto& p : powerUps) delete p;
powerUps.clear();
```

建议和其他战斗对象一起在 `InitBattle()` 开头清理。

## BUG-005 P 点矩形拾取判定计算了但没有使用

- 重要程度：中
- 位置：`touhou_project/Game.cpp`
- 相关代码：P 点拾取逻辑中计算了 `isColliding`，但最终仍使用 `player->CheckCollision(p)`。

复现情况：

1. 进入战斗并等待 P 点掉落。
2. 让 P 点碰到角色贴图边缘，但不接近玩家中心判定点。
3. 视觉上 P 点已经贴到角色身上，但不会被拾取。

原因：

- 代码中写了 AABB 判定：

```cpp
bool isColliding = (diffX < pickUpWidth / 2) && (diffY < pickUpHeight / 2);
```

- 但实际判断仍然是：

```cpp
if (p->active && player && player->CheckCollision(p)) {
```

玩家判定点半径只有 `3.0f`，P 点半径 `20`，实际拾取范围比注释预期小很多。

修改建议：

```cpp
if (p->active && player && isColliding) {
    ...
}
```

或者直接把拾取判定封装为 `Player::CanCollect(PowerUp*)`，避免碰撞规则散落。

## BUG-006 魔理沙 Bomb 音效判断使用了错误指针

- 重要程度：中
- 位置：`touhou_project/Game.cpp`
- 相关代码：

```cpp
if(se_REIMUBomb && spellUser == CharacterID::MARISA) Mix_PlayChannel(-1, se_MARISABomb, 0);
```

复现情况：

1. 删除、重命名或损坏 `assets/sfx/marisabomb.wav`。
2. 保留 `assets/sfx/reimubomb.wav`。
3. 选择魔理沙进入战斗。
4. 按 `X` 使用 Bomb。
5. 代码会因为 `se_REIMUBomb` 存在而调用 `Mix_PlayChannel()` 播放 `se_MARISABomb`，但此时 `se_MARISABomb` 可能是 `nullptr`。

原因：

- 魔理沙 Bomb 的空指针检查写成了灵梦音效变量。

修改建议：

```cpp
if (se_REIMUBomb && spellUser == CharacterID::REIMU) {
    Mix_PlayChannel(-1, se_REIMUBomb, 0);
}
if (se_MARISABomb && spellUser == CharacterID::MARISA) {
    Mix_PlayChannel(-1, se_MARISABomb, 0);
}
```

## BUG-007 部分音效加载失败后仍可能被无保护播放

- 重要程度：中
- 位置：`touhou_project/Game.cpp`
- 相关代码：`Game::Init()` 只是打印 `SFX Load Failed!`，但游戏仍继续运行；部分播放点没有空指针保护。

复现情况：

1. 删除或重命名 `assets/sfx/player_shoot.wav`。
2. 启动游戏并进入战斗。
3. 按 `Z` 射击。
4. `Mix_PlayChannel(-1, se_Shoot, 0)` 会收到空指针，可能导致无声、SDL_mixer 报错，严重时可能崩溃。

原因：

- `se_Shoot` 播放时没有判断是否为 `nullptr`。
- 初始化阶段没有在关键资源加载失败时阻止进入游戏。

修改建议：

- 所有 `Mix_PlayChannel()` 调用前都检查对应 `Mix_Chunk*`。
- 或者把关键音效视为必须资源，加载失败时让 `Init()` 返回 `false`。
- 对可选音效则统一封装：

```cpp
void PlaySfx(Mix_Chunk* chunk) {
    if (chunk) Mix_PlayChannel(-1, chunk, 0);
}
```

## BUG-008 胜利音效使用 `.mov` 文件通过 `Mix_LoadWAV()` 加载

- 重要程度：中
- 位置：`touhou_project/Game.cpp`
- 相关代码：

```cpp
se_Victory = Mix_LoadWAV("assets\\sfx\\victory.mov");
```

复现情况：

1. 启动游戏。
2. 如果 SDL_mixer 当前构建不支持该 `.mov` 文件作为 chunk 加载，初始化阶段会打印 `SFX Load Failed!`。
3. 击败 Boss 后没有胜利音效。

原因：

- `Mix_LoadWAV()` 更适合加载 wav/ogg 等 SDL_mixer 支持的音频 chunk。
- `.mov` 是视频容器格式，不适合作为短音效资源。

修改建议：

- 将 `victory.mov` 转为 `victory.wav` 或 `victory.ogg`。
- 修改加载路径。
- 如果需要播放视频，应使用单独的视频播放方案，而不是 `Mix_LoadWAV()`。

## BUG-009 BGM 音量最大只能调到 50%

- 重要程度：低
- 位置：`touhou_project/Game.cpp`
- 相关代码：BGM 右调条件为 `bgmVolume < 64`，但显示百分比按 `128` 计算。

复现情况：

1. 进入主菜单。
2. 进入音量设置。
3. 选择 BGM。
4. 一直按右键。
5. UI 最多显示 `50%`，无法调到 `100%`。

原因：

- `bgmVolume` 注释和显示逻辑都以 `0..128` 为范围。
- 但右键调节上限写成了 `64`。

修改建议：

```cpp
if (key[SDL_SCANCODE_RIGHT] && bgmVolume < 128) {
    bgmVolume += 4;
    Mix_VolumeMusic(bgmVolume);
}
```

并建议使用 `std::clamp()` 防止越界。

## BUG-010 Power 最大值显示和实际成长上限不一致

- 重要程度：低
- 位置：`touhou_project/Player.h`、`touhou_project/Game.cpp`
- 相关代码：`CollectPowerUp()` 允许 `powerLevel` 到 4，但 UI 显示 `Power: %.2f / 3.00`。

复现情况：

1. 进入战斗。
2. 持续拾取 P 点直到火力等级超过 3。
3. UI 可能显示类似 `Power: 4.00 / 3.00`。

原因：

- 代码中最高等级判断是：

```cpp
if (powerLevel >= 4) return 0;
```

- UI 文本写死为：

```cpp
sprintf_s(powerBuf, "Power:  %.2f / 3.00", totalPower);
```

修改建议：

- 如果最高火力是 3.00，改为 `powerLevel >= 3`。
- 如果最高火力是 4.00，UI 改成 `/ 4.00`。
- 建议定义常量，例如 `constexpr int MaxPowerLevel = 4;`。

## BUG-011 `Clean()` 资源释放顺序和释放范围不完整

- 重要程度：中
- 位置：`touhou_project/Game.cpp`
- 相关代码：`Game::Clean()`

复现情况：

1. 进入战斗并生成玩家、Boss、子弹、P 点。
2. 退出游戏。
3. 使用 Visual Studio Diagnostic Tools、AddressSanitizer、CRT leak check 或类似工具检查内存。
4. 可能看到纹理、字体、玩家、敌人、active 子弹、active P 点未释放。

原因：

- `SDL_DestroyRenderer(cur_Renderer)` 在 `SDL_DestroyTexture()` 前执行，释放顺序不合理。
- 只销毁了 `tex_BackgroundMenu` 和 `tex_BackgroundBattle`，没有销毁玩家、敌人、子弹贴图。
- 没有调用 `TTF_CloseFont(font)`。
- 只清理 inactive 的 bullet/powerUp，active 对象不会被 delete。
- 没有 delete `player` 和 `Enemies`。

修改建议：

- 先释放所有由 renderer 创建的 texture，再销毁 renderer。
- 调用 `TTF_CloseFont(font)`。
- delete 并 clear 所有实体 vector。
- delete `player`。
- 最后再 `Mix_CloseAudio()`、`TTF_Quit()`、`IMG_Quit()`、`SDL_Quit()`。

建议顺序：

```text
停止音频
释放 music/chunk
释放 game object
释放 texture/font
销毁 renderer/window
Quit SDL 子系统
```

## BUG-012 角色选择界面的 TextureColorMod 可能污染战斗贴图

- 重要程度：低
- 位置：`touhou_project/Game.cpp`
- 相关代码：角色选择界面通过 `SDL_SetTextureColorMod()` 把未选中角色贴图变暗。

复现情况：

1. 进入角色选择。
2. 快速按方向键切换角色并立刻按 `Z` 开始。
3. 如果进入战斗前没有再渲染一次选中状态，玩家贴图可能继承选择界面的变暗色调。

原因：

- `SDL_SetTextureColorMod()` 修改的是 texture 对象本身。
- 同一张 `tex_PlayerReimu` / `tex_PlayerMarisa` 也被战斗中的 `Player` 复用。

修改建议：

- 在 `InitBattle()` 创建玩家前把玩家贴图颜色恢复为白色：

```cpp
SDL_SetTextureColorMod(tex_PlayerReimu, 255, 255, 255);
SDL_SetTextureColorMod(tex_PlayerMarisa, 255, 255, 255);
```

- 或在角色选择渲染后立即恢复 texture color mod。

## BUG-013 玩家显示 0 残机时仍要再中弹一次才 Game Over

- 重要程度：低
- 位置：`touhou_project/Player.h`、`touhou_project/Game.cpp`
- 相关代码：`Dead_judge()` 使用 `lives < 0`，UI 在 `lives <= 0` 时显示 `None`。

复现情况：

1. 初始 `lives = 3`。
2. 被敌弹命中 3 次后，`lives = 0`。
3. UI 显示 `None`。
4. 此时游戏仍继续，玩家需要第 4 次中弹才进入 `GAME_OVER`。

原因：

- `Dead_judge()`：

```cpp
bool Dead_judge() { return lives < 0; }
```

- UI 逻辑：

```cpp
if (player->lives <= 0) lifeStr += "None";
```

修改建议：

- 如果 `lives` 表示剩余生命数，则改成 `lives <= 0`。
- 如果 `lives` 表示额外残机数，则 UI 文案不要在 0 时显示 `None`，可以显示 `Last` 或当前生命状态。

