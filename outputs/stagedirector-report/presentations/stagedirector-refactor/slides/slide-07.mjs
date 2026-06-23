// slide-07.mjs — 验收标准逐条对照
export default async function acceptance(presentation, ctx) {
  const slide = presentation.slides.add();
  const { width: w, height: h } = ctx.slideSize;
  const accent = "#00d4aa", bg = "#0f1923", text = "#e8edf0", muted = "#6b8a9e", panel = "#162231", pass = "#00d4aa", fail = "#ff6b6b";

  slide.add("rect", { x: 0, y: 0, width: w, height: h, fill: bg });
  slide.add("rect", { x: 0, y: 0, width: w, height: 4, fill: accent });
  slide.add("text", { x: 60, y: 30, width: 300, height: 24, text: "ACCEPTANCE", fontSize: 12, fontWeight: "bold", color: accent, fontFamily: "Arial", letterSpacing: 3 });
  slide.add("text", { x: 60, y: 55, width: w - 120, height: 50, text: "指南 §8 全部 5 项验收标准 PASS，额外指南 §1-§7 共 13 项全通过", fontSize: 30, fontWeight: "bold", color: text, fontFamily: "Arial" });

  const checks = [
    { id: "§8.1", item: "关卡事件仅在正确状态运行", how: "Update 第一行 if(state != PLAYING) return", status: true },
    { id: "§8.2", item: "暂停后事件计时冻结", how: "DIALOGUE 期间不推进 stageTime", status: true },
    { id: "§8.3", item: "至少支持 3 种触发方式", how: "Time / HpThreshold / PhaseChange = 3种", status: true },
    { id: "§8.4", item: "Game.cpp 不再堆硬编码判断", how: "事件逻辑在 StageEvent 子类 + lambdas", status: true },
    { id: "§8.5", item: "事件系统可独立扩展", how: "新增子类 + SetupStageEvents 注册即可", status: true },
    { id: "§2.1", item: "先看状态再看条件", how: "Update 入口状态守卫", status: true },
    { id: "§2.5", item: "暂停冻结原则", how: "非 PLAYING 直接 return", status: true },
    { id: "§3.1", item: "GameContext 含 deltaTime", how: "已补充 float deltaTime 字段", status: true },
    { id: "§3.2", item: "ShouldTrigger 为 const 查询", how: "基类+所有派生类均已 const", status: true },
    { id: "§4.6", item: "胜负事件分离", how: "VictoryEvent + DefeatEvent 独立类", status: true },
    { id: "§5.3", item: "状态变化通知导演", how: "Update 末尾 OnStateChanged 调用", status: true },
    { id: "§5.4", item: "仅 PLAYING 调 Update", how: "Update 在 PLAYING 分支内调用", status: true },
    { id: "§5.5", item: "阶段切换同步导演", how: "CheckEnemyPhase 内传入 phaseCtx", status: true },
  ];

  // 头部统计
  slide.add("rect", { x: 60, y: 120, width: w - 120, height: 44, fill: panel, rx: 6, ry: 6 });
  slide.add("text", { x: 90, y: 130, width: w - 180, height: 24, text: `13 / 13 项全通过      编译状态：MSBuild x64 Debug — 0 错误 0 警告`, fontSize: 16, fontWeight: "bold", color: pass, fontFamily: "Arial" });

  checks.forEach((c, i) => {
    const col = i < 7 ? 0 : 1;
    const row = i < 7 ? i : i - 7;
    const x = 60 + col * 600;
    const y = 180 + row * 62;

    slide.add("rect", { x, y, width: 570, height: 54, fill: panel, rx: 4, ry: 4 });
    slide.add("rect", { x, y, width: 4, height: 54, fill: c.status ? pass : fail });
    slide.add("text", { x: x + 16, y: y + 6, width: 60, height: 18, text: c.id, fontSize: 11, fontWeight: "bold", color: accent, fontFamily: "Consolas" });
    slide.add("text", { x: x + 80, y: y + 6, width: 460, height: 18, text: c.item, fontSize: 13, color: text, fontFamily: "Arial" });
    slide.add("text", { x: x + 80, y: y + 28, width: 460, height: 18, text: c.how, fontSize: 11, color: muted, fontFamily: "Consolas" });
  });

  return slide;
}
