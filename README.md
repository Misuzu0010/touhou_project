# touhou_project

基于 SDL2 开发的弹幕射击小游戏。

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

## 协作注意事项

- 不要把本机生成目录提交到 Git，例如 `.vs/`、`x64/`、`Debug/`、`Release/`。
- 不要提交个人工程配置文件，例如 `*.user`。
- 如果本机 SDL 路径不同，优先设置环境变量，不要直接修改 `.vcxproj`。
- 如果新增资源文件，需要确认它们没有被 `.gitignore` 排除。
