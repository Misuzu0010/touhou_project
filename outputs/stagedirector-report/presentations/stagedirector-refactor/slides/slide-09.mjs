// slide-09.mjs — 技术亮点
export default async function highlights(presentation, ctx) {
  const slide = presentation.slides.add();
  const { width: w, height: h } = ctx.slideSize;
  const accent = "#00d4aa", bg = "#0f1923", text = "#e8edf0", muted = "#6b8a9e", panel = "#162231";

  slide.add("rect", { x: 0, y: 0, width: w, height: h, fill: bg });
  slide.add("rect", { x: 0, y: 0, width: w, height: 4, fill: accent });
  slide.add("text", { x: 60, y: 30, width: 300, height: 24, text: "HIGHLIGHTS", fontSize: 12, fontWeight: "bold", color: accent, fontFamily: "Arial", letterSpacing: 3 });
  slide.add("text", { x: 60, y: 55, width: w - 120, height: 50, text: "拥抱现代 C++：std::function 多播委托、const 正确性、智能指针生命周期", fontSize: 30, fontWeight: "bold", color: text, fontFamily: "Arial" });

  const highlights = [
    {
      title: "多播委托（Multicast Delegate）",
      items: [
        "std::vector<std::function<void(GameContext&)>> 存储监听者列表",
        "Bind() 返回 *this 支持链式 .Bind(A).Bind(B).Bind(C)",
        "Execute 时 Broadcast 遍历所有监听者，互不干扰",
        "对比单一回调：同一事件可驱动对话 + 道具 + 音效等多个系统"
      ]
    },
    {
      title: "const 正确性（Const Correctness）",
      items: [
        "ShouldTrigger 声明为 const 查询方法，语义明确",
        "基类 + 7 派生类全部遵循 const 契约",
        "mutable 无滥用 — triggered 是内部缓存状态"
      ]
    },
    {
      title: "RAII 与智能指针",
      items: [
        "std::unique_ptr<StageEvent> 表达 StageDirector 对事件的所有权",
        "std::unique_ptr<Player/Enemy/Bullet> 全项目统一",
        "erase(remove_if) 惯用法清理失效对象"
      ]
    },
    {
      title: "循环依赖解耦",
      items: [
        "State 枚举从 Game.h 迁至 StageDirector.h",
        "Game.h 包含 StageDirector.h 即可获取完整类型",
        "前向声明 → 完整定义迁移，消除编译期不完整类型错误"
      ]
    }
  ];

  highlights.forEach((h, i) => {
    const x = 60 + (i % 2) * 590;
    const y = 120 + Math.floor(i / 2) * 280;
    slide.add("rect", { x, y, width: 560, height: 260, fill: panel, rx: 8, ry: 8 });
    slide.add("rect", { x, y, width: 560, height: 4, fill: accent });
    slide.add("text", { x: x + 20, y: y + 18, width: 520, height: 24, text: h.title, fontSize: 17, fontWeight: "bold", color: accent, fontFamily: "Arial" });
    h.items.forEach((item, j) => {
      slide.add("text", { x: x + 20, y: y + 60 + j * 48, width: 520, height: 40, text: "▸ " + item, fontSize: 13, color: muted, fontFamily: "Arial", lineHeight: 1.5 });
    });
  });

  return slide;
}
