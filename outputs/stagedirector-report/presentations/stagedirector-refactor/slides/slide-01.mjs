// slide-01.mjs — 封面
export default async function cover(presentation, ctx) {
  const slide = presentation.slides.add();
  const { width: w, height: h } = ctx.slideSize;

  // 深色背景
  slide.add("rect", { x: 0, y: 0, width: w, height: h, fill: "#0f1923" });

  // 顶部装饰线
  slide.add("rect", { x: 0, y: 0, width: w, height: 4, fill: "#00d4aa" });

  // 主标题
  slide.add("text", {
    x: 80, y: 180, width: w - 160, height: 100,
    text: "东方STG关卡事件系统重构",
    fontSize: 52, fontWeight: "bold", color: "#e8edf0",
    fontFamily: "Arial"
  });

  // 副标题
  slide.add("text", {
    x: 80, y: 290, width: w - 160, height: 60,
    text: "StageDirector — 状态驱动的关卡调度器架构设计",
    fontSize: 26, color: "#00d4aa",
    fontFamily: "Arial"
  });

  // 分隔线
  slide.add("rect", { x: 80, y: 370, width: 200, height: 2, fill: "#00d4aa" });

  // 标签
  slide.add("text", {
    x: 80, y: 400, width: 400, height: 40,
    text: "C++17 · SDL2 · 多播委托模式 · 13项验收标准全通过",
    fontSize: 16, color: "#6b8a9e",
    fontFamily: "Arial"
  });

  // 右下角装饰方块
  slide.add("rect", { x: w - 200, y: h - 200, width: 120, height: 120, fill: "#00d4aa", opacity: 0.08 });

  return slide;
}
