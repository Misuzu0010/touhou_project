// slide-04.mjs — 委托模式详解
export default async function delegatePattern(presentation, ctx) {
  const slide = presentation.slides.add();
  const { width: w, height: h } = ctx.slideSize;
  const accent = "#00d4aa", bg = "#0f1923", text = "#e8edf0", muted = "#6b8a9e", panel = "#162231", code = "#1a2d3d";

  slide.add("rect", { x: 0, y: 0, width: w, height: h, fill: bg });
  slide.add("rect", { x: 0, y: 0, width: w, height: 4, fill: accent });
  slide.add("text", { x: 60, y: 30, width: 300, height: 24, text: "DELEGATE PATTERN", fontSize: 12, fontWeight: "bold", color: accent, fontFamily: "Arial", letterSpacing: 3 });
  slide.add("text", { x: 60, y: 55, width: w - 120, height: 50, text: "多播委托：一个事件广播，多个监听者同时响应 —— 比单一回调更灵活", fontSize: 30, fontWeight: "bold", color: text, fontFamily: "Arial" });

  // 左：UE 对照表
  slide.add("rect", { x: 60, y: 130, width: 520, height: 220, fill: panel, rx: 6, ry: 6 });
  slide.add("text", { x: 85, y: 148, width: 300, height: 24, text: "UE 委托 ↔ C++ 实现对照", fontSize: 15, fontWeight: "bold", color: accent, fontFamily: "Arial" });
  const rows = [
    ["DECLARE_MULTICAST_DELEGATE", "std::vector<std::function<void(GameContext&)>>"],
    ["AddDynamic(this, &AMyActor::OnEvt)", "event->Bind([](GameContext& ctx) { ... })"],
    ["Broadcast()", "Execute() 内遍历 listeners 逐个调用"],
    ["触发条件判断", "ShouldTrigger(ctx) const → 条件满足才广播"],
    ["一次注册，多次响应", "链式 .Bind(A).Bind(B).Bind(C)"]
  ];
  rows.forEach(([ue, cpp], i) => {
    slide.add("text", { x: 85, y: 184 + i * 34, width: 230, height: 26, text: ue, fontSize: 12, color: "#ffd700", fontFamily: "Consolas" });
    slide.add("text", { x: 330, y: 184 + i * 34, width: 230, height: 26, text: cpp, fontSize: 12, color: muted, fontFamily: "Consolas" });
  });

  // 右：代码示例
  slide.add("rect", { x: 610, y: 130, width: 610, height: 220, fill: code, rx: 6, ry: 6 });
  slide.add("text", { x: 635, y: 148, width: 300, height: 24, text: "实际用法", fontSize: 15, fontWeight: "bold", color: accent, fontFamily: "Arial" });
  const codeLines = [
    "auto hp2000 = std::make_unique<HpThresholdEvent>(2000.0f);",
    "",
    "hp2000->Bind([](GameContext& ctx) {",
    "    ctx.dialogueRequested = true;     // 监听者1：对话",
    "})",
    ".Bind([](GameContext& ctx) {",
    "    ctx.itemSpawnRequested = true;    // 监听者2：掉道具",
    "    ctx.itemSpawnX = 960; ctx.itemSpawnY = 400;",
    "});",
    "",
    "stageDirector.AddEvent(std::move(hp2000));"
  ];
  codeLines.forEach((line, i) => {
    const c = line.includes("//") ? muted : line.includes("Bind") || line.includes("make_unique") ? accent : text;
    slide.add("text", { x: 635, y: 176 + i * 17, width: 560, height: 16, text: line, fontSize: 11, color: c, fontFamily: "Consolas" });
  });

  // 底部：三层行为机制
  slide.add("rect", { x: 60, y: 380, width: w - 120, height: 280, fill: panel, rx: 6, ry: 6 });
  slide.add("text", { x: 90, y: 400, width: 300, height: 24, text: "Execute 执行流程：两层行为叠加", fontSize: 16, fontWeight: "bold", color: accent, fontFamily: "Arial" });

  const layers = [
    { label: "1. 自动信号", desc: "事件类自身写 ctx 信号字段", ex: "SpawnItemEvent → ctx.itemSpawnRequested + x/y", c: "#ffd700" },
    { label: "2. 广播监听", desc: "遍历所有 Bind 的 std::function 逐个调用", ex: "每个 listener 独立执行，互不干扰", c: accent },
  ];
  layers.forEach((l, i) => {
    slide.add("text", { x: 90, y: 440 + i * 60, width: 200, height: 22, text: l.label, fontSize: 15, fontWeight: "bold", color: l.c, fontFamily: "Arial" });
    slide.add("text", { x: 300, y: 440 + i * 60, width: 500, height: 22, text: l.desc, fontSize: 14, color: text, fontFamily: "Arial" });
    slide.add("text", { x: 300, y: 462 + i * 60, width: 500, height: 22, text: "例：" + l.ex, fontSize: 12, color: muted, fontFamily: "Consolas" });
  });

  // 关键优势
  slide.add("text", { x: 90, y: 580, width: w - 180, height: 40, text: "▸ 优势：新增监听者只需 .Bind()，不改事件类本身。同一 HpThresholdEvent，不同关卡绑定不同监听者组合", fontSize: 14, color: text, fontFamily: "Arial" });

  return slide;
}
