#include "util.h"

#if RIVER_GRAPHICS_DEBUG

using namespace river;

QPointF toqt(const rmath::vec2& p) {
  return QPointF(p.x, p.y);
}

QLineF toqt(const rmath::line& l)
{
  return QLineF(toqt(l.p0), toqt(l.p1));
}

void draw_path(const rmath::line& l, const QPen& pen, const QPointF& offset)
{
  auto ql = toqt(l);
  ql.translate(offset);
  debug_util::show_line(ql, pen);
}

void draw_path(const rmath::bezier_cubic& c, const QPen& pen, const QPointF& offset)
{
  debug_util::show_cubic(toqt(c.p0), toqt(c.p1), toqt(c.p2), toqt(c.p3), pen);
}

void draw_path(const river::path& show, const QPen& pen, const QPointF& offset)
{
  struct draw_func
  {
    QPen pen;
    QPointF offset;

    static void line(path_lineto_func_para) {
      auto data = static_cast<const draw_func*>(user);
      auto& offset = data->offset;

      debug_util::show_line(QLineF(toqt(from) + offset, toqt(to) + offset), data->pen);
    }
    static void cubic(path_cubicto_func_para) {
      auto data = static_cast<draw_func*>(user);
      auto& offset = data->offset;

      debug_util::show_cubic(toqt(from) + offset, toqt(ctrl1) + offset, toqt(ctrl2) + offset, toqt(to) + offset, data->pen);
    }
  };

  draw_func data{ pen,offset };
  path_traverse_funcs func;
  func.line_to = draw_func::line;
  func.cubic_to = draw_func::cubic;
  show.traverse(func, &data);
}

void draw_path(const river::paths& show, const QPen& pen, const QPointF& offset)
{
  for (auto& p : show) {
    draw_path(p, pen, offset);
  }
}

#endif
