// slide-06.mjs — 运行时数据流
export default async function dataflow(presentation, ctx) {
  const slide = presentation.slides.add();
  const { width: w, height: h } = ctx.slideSize;
  const accent = "#00d4aa", bg = "#0f1923", text = "#e8edf0", muted = "#6b8a9e", panel = "#162231", flowBg = "#1a2d3d";

  slide.add("rect", { x: 0, y: 0, width: w, height: h, fill: bg });
  slide.add("rect", { x: 0, y: 0, width: w, height: 4, fill: accent });
  slide.add("text", { x: 60, y: 30, width: 300, height: 24, text: "DATA FLOW", fontSize: 12, fontWeight: "bold", color: accent, fontFamily: "Arial", letterSpacing: 3 });
  slide.add("text", { x: 60, y: 55, width: w - 120, height: 50, text: "每帧运行时事件管线：从 Game::Update 到 StageDirector 再到信号响应", fontSize: 28, fontWeight: "bold", color: text, fontFamily: "Arial" });

  // 流程步骤（垂直排列）
  const steps = [
    { num: "1", label: "更新对象", desc: "player / bullets / enemies / powerUps Update + 碰撞检测", color: muted },
    { num: "2", label: "CheckEnemyPhase", desc: "检测 HP 阈值 → 推进 phaseIndex → 同步导演(PhaseChangeEvent)", color: "#ffd700" },
    { num: "3", label: "构建 GameContext", desc: "ctx.state / bossHp / phaseIndex 快照", color: muted },
    { num: "4", label: "stageDirector.Update", desc: "仅 PLAYING 推进 stageTime → 遍历事件 → ShouldTrigger → Execute(Broadcast)", color: accent },
    { num: "5", label: "响应信号", desc: "dialogueRequested → DIALOGUE / itemSpawn → PowerUp / finish → VICTORY|GAME_OVER", color: "#ffd700" },
  ];

  steps.forEach((s, i) => {
    const y = 120 + i * 80;
    // 编号圆圈
    slide.add("ellipse", { cx: 80, cy: y + 28, rx: 18, ry: 18, fill: s.color, opacity: 0.2 });
    slide.add("text", { x: 63, y: y + 16, width: 36, height: 24, text: s.num, fontSize: 16, fontWeight: "bold", color: s.color, fontFamily: "Arial", align: "center" });
    // 连接线
    if (i < 4) {
      slide.add("rect", { x: 79, y: y + 56, width: 2, height: 24, fill: muted, opacity: 0.2 });
    }
    // 标签 + 描述
    slide.add("text", { x: 120, y: y + 10, width: 300, height: 24, text: s.label, fontSize: 16, fontWeight: "bold", color: s.color, fontFamily: "Arial" });
    slide.add("text", { x: 120, y: y + 38, width: 600, height: 24, text: s.desc, fontSize: 13, color: muted, fontFamily: "Arial" });
  });

  // 右侧：状态守卫示意
  slide.add("rect", { x: 700, y: 120, width: 520, height: 280, fill: panel, rx: 6, ry: 6 });
  slide.add("text", { x: 730, y: 140, width: 400, height: 24, text: "核心守卫：暂停冻结", fontSize: 16, fontWeight: "bold", color: accent, fontFamily: "Arial" });
  const guardLines = [
    "void StageDirector::Update(dt, ctx) {",
    "    ctx.deltaTime = dt;",
    "",
    "    // ★ 关键守卫：非 PLAYING 不推进",
    "    if (ctx.state != State::PLAYING)",
    "        return;  // DIALOGUE/VICTORY/GAME_OVER 冻结",
    "",
    "    stageTime += dt;",
    "    ctx.stageTime = stageTime;",
    "    // ... 遍历事件触发 ...",
    "}"
  ];
  guardLines.forEach((line, i) => {
    const c = line.includes("★") || line.includes("return") ? "#ffd700" : line.includes("//") ? muted : text;
    slide.add("text", { x: 730, y: 172 + i * 18, width: 470, height: 16, text: line, fontSize: 12, color: c, fontFamily: "Consolas" });
  });

  // 底部：OnStateChanged
  slide.add("rect", { x: 60, y: 530, width: w - 120, height: 130, fill: panel, rx: 6, ry: 6 });
  slide.add("text", { x: 90, y: 548, width: 400, height: 24, text: "状态切换：Update 末尾检测 oldState != CurrentState → OnStateChanged", fontSize: 15, fontWeight: "bold", color: accent, fontFamily: "Arial" });
  slide.add("text", { x: 90, y: 580, width: w - 180, height: 20, text: "进入 PLAYING → Reset() 清空事件 + 时间归零    |    退出 PLAYING → stageTime 冻结（Update 守卫拦截）", fontSize: 13, color: muted, fontFamily: "Arial" });
  slide.add("text", { x: 90, y: 610, width: w - 180, height: 20, text: "DIALOGUE 恢复 PLAYING → 不 Reset，继续原有时间线    |    GAME_OVER → 续关 / 返回菜单由 Game 处理", fontSize: 13, color: muted, fontFamily: "Arial" });

  return slide;
}
