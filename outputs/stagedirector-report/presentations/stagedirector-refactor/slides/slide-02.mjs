// slide-02.mjs — 重构动机与核心原则
export default async function motivation(presentation, ctx) {
  const slide = presentation.slides.add();
  const { width: w, height: h } = ctx.slideSize;
  const accent = "#00d4aa", bg = "#0f1923", text = "#e8edf0", muted = "#6b8a9e", darkPanel = "#162231";

  slide.add("rect", { x: 0, y: 0, width: w, height: h, fill: bg });
  slide.add("rect", { x: 0, y: 0, width: w, height: 4, fill: accent });

  // Kicker
  slide.add("text", { x: 60, y: 30, width: 200, height: 24, text: "WHY REFACTOR", fontSize: 12, fontWeight: "bold", color: accent, fontFamily: "Arial", letterSpacing: 3 });

  // 标题
  slide.add("text", { x: 60, y: 55, width: w - 120, height: 50, text: "Game.cpp 的 Update 函数越来越臃肿，每次加新关卡逻辑都要改主循环", fontSize: 30, fontWeight: "bold", color: text, fontFamily: "Arial" });

  // 左列 - 重构前痛点
  slide.add("text", { x: 60, y: 135, width: 400, height: 24, text: "重构前", fontSize: 16, fontWeight: "bold", color: "#ff6b6b", fontFamily: "Arial" });
  const pains = [
    "· 阶段切换、道具生成、胜负判定全堆在 Game::Update",
    "· 每加一个新关卡事件要改主循环 switch-case",
    "· 对话、弹幕、时间线逻辑互相耦合",
    "· 硬编码的 currentPhaseIndex 分支判断分散各处"
  ];
  pains.forEach((p, i) => {
    slide.add("text", { x: 60, y: 165 + i * 36, width: 520, height: 30, text: p, fontSize: 15, color: muted, fontFamily: "Arial" });
  });

  // 右列 - 重构后目标
  slide.add("text", { x: 620, y: 135, width: 400, height: 24, text: "重构后", fontSize: 16, fontWeight: "bold", color: accent, fontFamily: "Arial" });
  const goals = [
    "· 事件独立封装，新增事件不改主循环",
    "· 时间/血量/阶段三种触发条件并行",
    "· 暂停冻结：DIALOGUE 时不推进战斗计时",
    "· 一对多广播：一个事件多个监听者同时响应"
  ];
  goals.forEach((g, i) => {
    slide.add("text", { x: 620, y: 165 + i * 36, width: 580, height: 30, text: g, fontSize: 15, color: muted, fontFamily: "Arial" });
  });

  // 底部核心原则卡片
  slide.add("rect", { x: 60, y: 360, width: w - 120, height: 280, fill: darkPanel, rx: 8, ry: 8 });
  slide.add("text", { x: 90, y: 380, width: 300, height: 24, text: "五条核心设计原则", fontSize: 16, fontWeight: "bold", color: accent, fontFamily: "Arial" });

  const principles = [
    ["01 先看状态再看条件", "仅 PLAYING 时推进事件，非战斗状态冻结计时"],
    ["02 时间只是条件之一", "stageTime / bossHp / phaseIndex 均可触发，不唯一依赖时间"],
    ["03 事件独立封装", "每个事件自包含触发条件 + 行为，不污染 Game.cpp"],
    ["04 一次触发", "triggered 标志防止重复执行"],
    ["05 暂停冻结", "DIALOGUE / GAME_OVER / VICTORY 期间 stageTime 不推进"]
  ];
  principles.forEach(([title, desc], i) => {
    const x = 90 + (i % 3) * 370;
    const y = 420 + Math.floor(i / 3) * 90;
    slide.add("rect", { x, y, width: 340, height: 70, fill: "#1a2d3d", rx: 4, ry: 4 });
    slide.add("text", { x: x + 15, y: y + 10, width: 310, height: 22, text: title, fontSize: 14, fontWeight: "bold", color: text, fontFamily: "Arial" });
    slide.add("text", { x: x + 15, y: y + 36, width: 310, height: 22, text: desc, fontSize: 12, color: muted, fontFamily: "Arial" });
  });

  return slide;
}
