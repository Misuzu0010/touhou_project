# touhou_project

基于 SDL2 开发的东方风格弹幕射击小游戏。

当前版本已经整合了以下核心功能：

- 主菜单、角色选择、帮助界面与暂停菜单。
- 玩家普通射击、Bomb / 符卡系统。
- Boss 三段血量阶段与差异化弹幕。
- 多态道具系统，以及基于事件的关卡掉落调度。

## 开发环境

推荐使用 Windows + Visual Studio 2022 / Build Tools：

- 安装“使用 C++ 的桌面开发”工作负载。
- 安装 Windows SDK。
- 当前工程使用 C++17。
- 当前工程平台建议选择 `x64`。

## SDL 依赖

项目需要以下 SDL2 相关库：

- SDL2
- SDL2_image
- SDL2_ttf
- SDL2_mixer

工程会从 `Directory.Build.props` 读取依赖路径。默认值保留为当前开发机路径，协作者可以通过环境变量覆盖：

```powershell
setx SDL2_DIR "D:\libs\SDL2-2.32.10"
setx SDL2_IMAGE_DIR "D:\libs\SDL2_image-2.8.5"
setx SDL2_TTF_DIR "D:\libs\SDL2_ttf-2.24.0"
setx SDL2_MIXER_DIR "D:\libs\SDL2_mixer-2.8.1"
```

设置后重新打开终端或 Visual Studio，使环境变量生效。

每个目录下应包含对应的 `include` 和 `lib\x64` 子目录，例如：

```text
D:\libs\SDL2-2.32.10\include
D:\libs\SDL2-2.32.10\lib\x64
```

## 构建方式

### Visual Studio

1. 打开 `touhou_project.sln`。
2. 选择 `x64` 平台。
3. 选择 `Debug` 或 `Release`。
4. 点击“生成解决方案”。

### 命令行

```powershell
MSBuild.exe touhou_project.sln /p:Configuration=Release /p:Platform=x64 /m
```

如果 `MSBuild.exe` 不在 `PATH` 中，可以从 Visual Studio Developer PowerShell 执行，或使用本机 MSBuild 完整路径。

## 运行资源

运行时需要保留 `touhou_project` 目录中的图片、字体和音频资源：

- 根目录图片：`Reimu.png`、`Marisa.png`、`TitleBg.png`、`BattleBg.png` 等。
- 字体：`touhou_project/assets/fonts/SimHei.ttf`。
- 音频：`touhou_project/assets/audios/` 和 `touhou_project/assets/sfx/`。

## 操作说明

- 主菜单：方向键上下切换，`Z` 确认。
- 角色选择：左右方向键切换角色，`Z` 开始战斗，`ESC` 返回主菜单。
- 帮助界面：可从主菜单进入，`Z` 或 `ESC` 返回主菜单。
- 战斗中移动：方向键移动，按住 `Left Shift` 进入低速移动并显示判定点。
- 战斗中攻击：按住 `Z` 持续射击，`X` 释放符卡 / Bomb。
- 暂停功能：战斗中按 `ESC` 进入暂停界面，可继续游戏或返回主菜单。
- 结算界面：`GAME OVER` 时可按 `R` 续关，`ESC` 返回主菜单；胜利后按 `ESC` 返回主菜单。

## 当前实现说明

- Boss 初始阶段会使用环形弹幕，进入中段后切换为延迟瞄准弹幕，低血量阶段切换为高速螺旋弹幕。
- 战斗内已接入 `StageDirector`，会根据时间、Boss 血量和阶段切换触发额外掉落事件。
- 新增道具通过 `Item` 抽象基类统一管理，当前包含火力、Bomb、残机三类拾取物。

## 协作注意事项

- 不要把本机生成目录提交到 Git，例如 `.vs/`、`x64/`、`Debug/`、`Release/`。
- 不要提交个人工程配置文件，例如 `*.user`。
- 如果本机 SDL 路径不同，优先设置环境变量，不要直接修改 `.vcxproj`。
- 如果新增资源文件，需要确认它们没有被 `.gitignore` 排除。
