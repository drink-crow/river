#include "util.h"

#if RIVER_GRAPHICS_DEBUG

using namespace river;

QPointF toqt(const rmath::vec2& p) {
    return QPointF(p.x, p.y);
}

void draw_path(const river::Path& show, const QPen& pen, const QPointF& offset)
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

void draw_path(const river::Paths& show, const QPen& pen, const QPointF& offset)
{
    for (auto& p : show) {
        draw_path(p, pen, offset);
    }
}

#endif
