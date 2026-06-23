// slide-05.mjs — 7种事件类型一览
export default async function eventTypes(presentation, ctx) {
  const slide = presentation.slides.add();
  const { width: w, height: h } = ctx.slideSize;
  const accent = "#00d4aa", bg = "#0f1923", text = "#e8edf0", muted = "#6b8a9e", panel = "#162231";

  slide.add("rect", { x: 0, y: 0, width: w, height: h, fill: bg });
  slide.add("rect", { x: 0, y: 0, width: w, height: 4, fill: accent });
  slide.add("text", { x: 60, y: 30, width: 300, height: 24, text: "EVENT TYPES", fontSize: 12, fontWeight: "bold", color: accent, fontFamily: "Arial", letterSpacing: 3 });
  slide.add("text", { x: 60, y: 55, width: w - 120, height: 50, text: "7 种事件类型覆盖关卡全部调度需求，指南 §4 建议 + 胜负分离", fontSize: 30, fontWeight: "bold", color: text, fontFamily: "Arial" });

  const events = [
    { name: "TimeEvent", trigger: "stageTime >= T", signal: "—（纯广播）", desc: "定时触发，5s/10s/15s 不同行为由 Bind 注入" },
    { name: "HpThresholdEvent", trigger: "bossHp <= 阈值", signal: "—（纯广播）", desc: "Boss 血量低于 4000/2000 时触发符卡宣言" },
    { name: "PhaseChangeEvent", trigger: "phaseIndex >= N", signal: "—（纯广播）", desc: "阶段切换时重置弹幕模式、播放特效" },
    { name: "DialogueEvent", trigger: "立即触发", signal: "dialogueRequested", desc: "切换至 DIALOGUE 状态，MarkFinished 标记结束" },
    { name: "SpawnItemEvent", trigger: "立即触发", signal: "itemSpawn+X/Y", desc: "指定坐标生成 PowerUp，Game 端读取创建" },
    { name: "VictoryEvent", trigger: "立即触发", signal: "finish+isVictory=true", desc: "胜利结算，停止 BGM、切换 VICTORY 状态" },
    { name: "DefeatEvent", trigger: "立即触发", signal: "finish+isVictory=false", desc: "失败结算，切换 GAME_OVER 状态" }
  ];

  events.forEach((ev, i) => {
    const x = 60 + (i % 2) * 590;
    const y = 125 + Math.floor(i / 2) * 82;
    slide.add("rect", { x, y, width: 560, height: 70, fill: panel, rx: 6, ry: 6 });
    slide.add("rect", { x, y, width: 4, height: 70, fill: accent });
    slide.add("text", { x: x + 18, y: y + 8, width: 200, height: 22, text: ev.name, fontSize: 16, fontWeight: "bold", color: accent, fontFamily: "Consolas" });
    slide.add("text", { x: x + 230, y: y + 8, width: 180, height: 20, text: "触发: " + ev.trigger, fontSize: 11, color: muted, fontFamily: "Consolas" });
    slide.add("text", { x: x + 420, y: y + 8, width: 130, height: 20, text: ev.signal, fontSize: 11, color: "#ffd700", fontFamily: "Consolas" });
    slide.add("text", { x: x + 18, y: y + 38, width: 520, height: 22, text: ev.desc, fontSize: 12, color: muted, fontFamily: "Arial" });
  });

  // 底部统计
  slide.add("rect", { x: 60, y: 565, width: w - 120, height: 100, fill: panel, rx: 6, ry: 6 });
  const stats = ["3 种触发方式：时间 / 血量 / 阶段", "4 种自动信号：dialogue / itemSpawn / finish", "7 个事件类 = 1 基类 + 6 派生 + 胜负分离"];
  stats.forEach((s, i) => {
    slide.add("text", { x: 100, y: 588 + i * 28, width: 500, height: 22, text: "▸ " + s, fontSize: 14, color: text, fontFamily: "Arial" });
  });

  return slide;
}
