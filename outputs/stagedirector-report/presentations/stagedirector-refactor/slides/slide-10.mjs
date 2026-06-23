// slide-10.mjs — 总结
export default async function summary(presentation, ctx) {
  const slide = presentation.slides.add();
  const { width: w, height: h } = ctx.slideSize;
  const accent = "#00d4aa", bg = "#0f1923", text = "#e8edf0", muted = "#6b8a9e", panel = "#162231";

  slide.add("rect", { x: 0, y: 0, width: w, height: h, fill: bg });
  slide.add("rect", { x: 0, y: 0, width: w, height: 4, fill: accent });

  // 顶部
  slide.add("text", { x: 60, y: 30, width: 300, height: 24, text: "SUMMARY", fontSize: 12, fontWeight: "bold", color: accent, fontFamily: "Arial", letterSpacing: 3 });
  slide.add("text", { x: 60, y: 60, width: w - 120, height: 60, text: "状态驱动的关卡调度器 — 让关卡流程从 Game.cpp 中解放", fontSize: 36, fontWeight: "bold", color: text, fontFamily: "Arial" });

  // 三个关键成果
  const results = [
    { title: "解耦", desc: "Game 只提供上下文，StageDirector 管理事件时间线，Game.cpp 不再堆满关卡判断逻辑", icon: "◇" },
    { title: "可扩展", desc: "新增事件类型只需派生 StageEvent + SetupStageEvents 注册，不改主循环一行代码", icon: "◆" },
    { title: "现代 C++", desc: "std::function 多播委托、const 正确性、unique_ptr RAII、前向声明解耦循环依赖", icon: "◈" }
  ];
  results.forEach((r, i) => {
    const x = 60 + i * 400;
    slide.add("rect", { x, y: 150, width: 360, height: 180, fill: panel, rx: 8, ry: 8 });
    slide.add("text", { x: x + 30, y: 175, width: 300, height: 30, text: r.icon, fontSize: 20, color: accent, fontFamily: "Arial" });
    slide.add("text", { x: x + 30, y: 210, width: 300, height: 30, text: r.title, fontSize: 24, fontWeight: "bold", color: text, fontFamily: "Arial" });
    slide.add("text", { x: x + 30, y: 250, width: 300, height: 60, text: r.desc, fontSize: 13, color: muted, fontFamily: "Arial", lineHeight: 1.5 });
  });

  // 技术栈
  slide.add("rect", { x: 60, y: 370, width: w - 120, height: 80, fill: panel, rx: 6, ry: 6 });
  slide.add("text", { x: 90, y: 385, width: 200, height: 22, text: "技术栈", fontSize: 14, fontWeight: "bold", color: accent, fontFamily: "Arial" });
  slide.add("text", { x: 90, y: 415, width: w - 180, height: 22, text: "C++17 · SDL2 · SDL_image · SDL_ttf · SDL_mixer · Visual Studio 2022 · MSBuild · x64 Debug", fontSize: 14, color: muted, fontFamily: "Consolas" });

  // 底部：文件结构
  slide.add("rect", { x: 60, y: 475, width: w - 120, height: 200, fill: panel, rx: 6, ry: 6 });
  slide.add("text", { x: 90, y: 495, width: 400, height: 24, text: "项目文件结构（StageDirector 模块）", fontSize: 16, fontWeight: "bold", color: accent, fontFamily: "Arial" });

  const tree = [
    "touhou_project/",
    "  StageDirector.h    — 导演类 + GameContext + StageEvent 基类 + State 枚举",
    "  StageDirector.cpp  — Update(状态守卫) / OnStateChanged / Reset / AddEvent",
    "  StageEvent.h       — 7 个派生事件类声明（委托模式）",
    "  StageEvent.cpp     — 7 个事件类实现（Broadcast 多播）",
    "  Game.h             — 挂接 stageDirector 成员 + SetupStageEvents",
    "  Game.cpp           — InitBattle 注册事件 / Update 驱动导演 / 信号响应"
  ];
  tree.forEach((line, i) => {
    slide.add("text", { x: 100, y: 526 + i * 20, width: 700, height: 18, text: line, fontSize: 12, color: line.startsWith("touhou") ? text : muted, fontFamily: "Consolas" });
  });

  return slide;
}
