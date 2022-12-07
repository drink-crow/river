#include "dcel.h"

namespace dcel {

    void dcel::set_current_set_type(set_type new_type)
    {
        current_set_type = new_type;
    }

    set_type dcel::get_current_set_type() const
    {
        return current_set_type;
    }

    void dcel::add_line(num x1, num y1, num x2, num y2)
    {
        point* start_p = find_or_add_point(x1, y1);
        point* end_p = find_or_add_point(x2, y2);

        // 添加所有线时只构建半边
        auto l1 = new_line_edge(start_p, end_p);
    }

    line_half_edge* dcel::new_line_edge(point* start, point* end)
    {
        auto line = new line_half_edge;

        line->start = start;
        line->end = end;
        line->type = current_set_type;

        edges.insert(line);
        start->edges.push_back(line);

        return line;
    }

    point* dcel::find_or_add_point(num x, num y)
    {
        point tmp_p;
        point* res;

        tmp_p.x = x; tmp_p.y = y;
        auto it = point_set.find(&tmp_p);
        if (it == point_set.end()) {
            res = new point;
            point_set.insert(res);
        }
        else {
            res = *it;
        }

        return res;
    }

}
