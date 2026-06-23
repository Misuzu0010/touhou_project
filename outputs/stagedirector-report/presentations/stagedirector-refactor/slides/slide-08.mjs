// slide-08.mjs — 代码变更统计
export default async function metrics(presentation, ctx) {
  const slide = presentation.slides.add();
  const { width: w, height: h } = ctx.slideSize;
  const accent = "#00d4aa", bg = "#0f1923", text = "#e8edf0", muted = "#6b8a9e", panel = "#162231";

  slide.add("rect", { x: 0, y: 0, width: w, height: h, fill: bg });
  slide.add("rect", { x: 0, y: 0, width: w, height: 4, fill: accent });
  slide.add("text", { x: 60, y: 30, width: 300, height: 24, text: "CODE METRICS", fontSize: 12, fontWeight: "bold", color: accent, fontFamily: "Arial", letterSpacing: 3 });
  slide.add("text", { x: 60, y: 55, width: w - 120, height: 50, text: "重构涉及 5 个文件、7 个事件类、事件系统独立于主循环可扩展", fontSize: 30, fontWeight: "bold", color: text, fontFamily: "Arial" });

  // 4个统计卡片
  const cards = [
    { value: "5", label: "修改文件", sub: "h / .cpp", x: 60 },
    { value: "7", label: "事件类型", sub: "1基类+6派生+胜负分离", x: 355 },
    { value: "13/13", label: "验收通过", sub: "指南 §1-§8 全项", x: 650 },
    { value: "0", label: "编译错误", sub: "x64 Debug MSBuild", x: 945 }
  ];
  cards.forEach(c => {
    slide.add("rect", { x: c.x, y: 130, width: 260, height: 140, fill: panel, rx: 8, ry: 8 });
    slide.add("text", { x: c.x + 20, y: 150, width: 220, height: 50, text: c.value, fontSize: 48, fontWeight: "bold", color: accent, fontFamily: "Arial" });
    slide.add("text", { x: c.x + 20, y: 210, width: 220, height: 22, text: c.label, fontSize: 15, color: text, fontFamily: "Arial" });
    slide.add("text", { x: c.x + 20, y: 234, width: 220, height: 20, text: c.sub, fontSize: 12, color: muted, fontFamily: "Arial" });
  });

  // 文件变更详情表
  slide.add("rect", { x: 60, y: 300, width: w - 120, height: 360, fill: panel, rx: 6, ry: 6 });
  slide.add("text", { x: 90, y: 316, width: 400, height: 24, text: "文件变更明细", fontSize: 16, fontWeight: "bold", color: accent, fontFamily: "Arial" });

  const files = [
    { name: "StageDirector.h", change: "State 完整定义迁入 · GameContext 加 deltaTime · ShouldTrigger const", type: "重构" },
    { name: "StageDirector.cpp", change: "Update 加状态守卫 · OnStateChanged 完整实现 · 写入 ctx.deltaTime", type: "实现" },
    { name: "StageEvent.h", change: "7 个事件类声明 · std::function → vector<function> 多播 · Bind 链式", type: "重构" },
    { name: "StageEvent.cpp", change: "7 个事件类完整实现 · Broadcast 遍历 · 基础信号 + 监听者广播", type: "新建" },
    { name: "Game.h", change: "挂接 StageDirector 成员 · SetupStageEvents 声明 · State 定义移除", type: "修改" },
    { name: "Game.cpp", change: "Update 加 OnStateChanged · CheckEnemyPhase 同步导演 · 3 信号响应激活", type: "集成" },
  ];

  // 表头
  slide.add("text", { x: 90, y: 350, width: 260, height: 20, text: "文件", fontSize: 12, fontWeight: "bold", color: muted, fontFamily: "Consolas" });
  slide.add("text", { x: 370, y: 350, width: 500, height: 20, text: "变更内容", fontSize: 12, fontWeight: "bold", color: muted, fontFamily: "Arial" });
  slide.add("text", { x: 900, y: 350, width: 100, height: 20, text: "类型", fontSize: 12, fontWeight: "bold", color: muted, fontFamily: "Arial" });

  files.forEach((f, i) => {
    const y = 376 + i * 40;
    slide.add("rect", { x: 90, y, width: w - 180, height: 1, fill: muted, opacity: 0.1 });
    slide.add("text", { x: 90, y: y + 6, width: 260, height: 20, text: f.name, fontSize: 13, color: text, fontFamily: "Consolas" });
    slide.add("text", { x: 370, y: y + 6, width: 510, height: 20, text: f.change, fontSize: 12, color: muted, fontFamily: "Arial" });
    const tColor = f.type === "新建" ? accent : f.type === "重构" ? "#ffd700" : muted;
    slide.add("text", { x: 900, y: y + 6, width: 100, height: 20, text: f.type, fontSize: 12, fontWeight: "bold", color: tColor, fontFamily: "Arial" });
  });

  return slide;
}
