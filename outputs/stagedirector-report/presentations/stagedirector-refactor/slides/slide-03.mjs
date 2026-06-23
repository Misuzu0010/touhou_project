// slide-03.mjs — 核心架构三件套
export default async function architecture(presentation, ctx) {
  const slide = presentation.slides.add();
  const { width: w, height: h } = ctx.slideSize;
  const accent = "#00d4aa", bg = "#0f1923", text = "#e8edf0", muted = "#6b8a9e", panel = "#162231";

  slide.add("rect", { x: 0, y: 0, width: w, height: h, fill: bg });
  slide.add("rect", { x: 0, y: 0, width: w, height: 4, fill: accent });
  slide.add("text", { x: 60, y: 30, width: 300, height: 24, text: "ARCHITECTURE", fontSize: 12, fontWeight: "bold", color: accent, fontFamily: "Arial", letterSpacing: 3 });
  slide.add("text", { x: 60, y: 55, width: w - 120, height: 50, text: "三件套：GameContext 传数据，StageEvent 封装行为，StageDirector 调度时间线", fontSize: 28, fontWeight: "bold", color: text, fontFamily: "Arial" });

  // 三个组件卡片
  const cards = [
    { title: "GameContext", role: "只读快照 / 信号总线", items: ["state · stageTime · deltaTime", "bossHp · phaseIndex", "dialogueRequested ✦", "itemSpawnRequested ✦", "finishRequested ✦"], x: 60 },
    { title: "StageEvent", role: "抽象基类 + 7 派生类", items: ["ShouldTrigger(ctx) const", "Execute(ctx) + Broadcast", "isFinished() const", "Bind(listener) 链式", "一对多委托监听"], x: 440 },
    { title: "StageDirector", role: "时间线调度器", items: ["Update(dt, ctx)", "仅 PLAYING 推进", "OnStateChanged()", "Reset() · AddEvent()", "遍历 → 触发 → 移除"], x: 820 }
  ];

  cards.forEach(c => {
    slide.add("rect", { x: c.x, y: 130, width: 350, height: 340, fill: panel, rx: 8, ry: 8 });
    slide.add("rect", { x: c.x, y: 130, width: 350, height: 4, fill: accent });
    slide.add("text", { x: c.x + 20, y: 150, width: 310, height: 30, text: c.title, fontSize: 22, fontWeight: "bold", color: accent, fontFamily: "Arial" });
    slide.add("text", { x: c.x + 20, y: 184, width: 310, height: 22, text: c.role, fontSize: 13, color: muted, fontFamily: "Arial" });
    c.items.forEach((item, i) => {
      slide.add("text", { x: c.x + 20, y: 220 + i * 34, width: 310, height: 26, text: "▸ " + item, fontSize: 14, color: text, fontFamily: "Arial" });
    });
  });

  // 箭头连接
  slide.add("text", { x: 415, y: 280, width: 30, height: 30, text: "→", fontSize: 28, color: accent, fontFamily: "Arial" });
  slide.add("text", { x: 795, y: 280, width: 30, height: 30, text: "→", fontSize: 28, color: accent, fontFamily: "Arial" });

  // 底部数据流示意
  slide.add("rect", { x: 60, y: 500, width: w - 120, height: 160, fill: panel, rx: 6, ry: 6 });
  slide.add("text", { x: 90, y: 518, width: 200, height: 22, text: "数据流向", fontSize: 14, fontWeight: "bold", color: accent, fontFamily: "Arial" });
  const flow = "Game::Update → 构建 GameContext → stageDirector.Update(dt, ctx) → 事件 ShouldTrigger → Execute(Broadcast) → ctx 信号字段 → Game 读取响应";
  slide.add("text", { x: 90, y: 550, width: w - 180, height: 90, text: flow, fontSize: 15, color: muted, fontFamily: "Arial", lineHeight: 1.6 });

  return slide;
}
