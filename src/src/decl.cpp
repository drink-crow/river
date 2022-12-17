#include "boost/token_functions.hpp"
#include "dcel.h"
#include <algorithm>
#include <cstddef>
#include <qbrush.h>
#include <qline.h>
#include <qnamespace.h>
#include <qpainterpath.h>
#include <qpen.h>
#include <qpoint.h>
#include <qsize.h>
#include <vector>
#include <cmath>

#include "debug_util.h"

#include "QGraphicsPathItem"
#include "QGraphicsRectItem"
#include "river.h"
#include "rmath.h"
#include "scan_line.h"
#include "util.h"

namespace dcel {

#if RIVER_GRAPHICS_DEBUG

    QPointF qstart(const half_edge* e) {
        return toqt(e->start->p);
    }
    QPointF qend(const half_edge* e) {
        return toqt(e->end->p);
    }

    QLineF qline(const half_edge* e, bool inverse = false)
    {
        if (!inverse) {
            return QLineF(qstart(e), qend(e));
        }
        else {
            return QLineF(qend(e), qstart(e));
        }
    }

    void qline_arrow(const half_edge* e, const QPen& pen = QPen())
    {
        debug_util::show_line(qline(e), pen);
        vec2 center = (e->start->p + e->end->p) / 2;
        
        vec2 v = (e->end->p - e->start->p).normalizate()*5;
        debug_util::show_line(QLineF(toqt(center),toqt(v.rotated(pi * 3/4) + center)), pen);
        debug_util::show_line(QLineF(toqt(center), toqt(v.rotated(-pi * 3 / 4) + center)), pen);
    }

#endif

    bool vec2_compare_func(const ::rmath::vec2& r, const ::rmath::vec2& l)
    {
        if (r.x < l.x) return true;
        if (r.x == l.x) return r.y < l.y;
        return false;
    }

    bool point_between_two_point(const vec2& p, const vec2& p0, const vec2& p1)
    {
        return ((p0.x < p.x) && (p.x < p1.x))
            || ((p0.y < p.y) && (p.y < p1.y));
    }

    void intersect(line_half_edge* l, line_half_edge* r)
    {
        const vec2& p0 = l->start->p;
        const vec2& p1 = l->end->p;
        const vec2& p2 = r->start->p;
        const vec2& p3 = r->end->p;

        const vec2 a = p1 - p0;
        const vec2 b = p2 - p3;
        const vec2 c = p0 - p2;

        const double denominator = a.y * b.x - a.x * b.y;
        if (denominator == 0 || !std::isfinite(denominator))
        {
            // 此时平行
            if (dist2_point_line(p0, p2, p3) == 0)
            {
                // 此时共线
                if (p0 != p2 && p0 != p3 && point_between_two_point(p0, p2, p3)) {
                    // p0 在 r 内
                    r->add_break_point(p0);
                }
                if (p1 != p2 && p1 != p3 && point_between_two_point(p1, p2, p3)) {
                    // p1 在 r 内
                    r->add_break_point(p1);
                }

                if (p2 != p0 && p2 != p1 && point_between_two_point(p2, p0, p1)) {
                    // p2 在 l 内
                    l->add_break_point(p2);
                }
                if (p3 != p0 && p3 != p1 && point_between_two_point(p3, p0, p1)) {
                    // p3 在 l 内
                    l->add_break_point(p3);
                }
            }

            return;
        }

        const double reciprocal = 1 / denominator;
        const double na = (b.y * c.x - b.x * c.y) * reciprocal;

        bool in_l = false;
        vec2 intersection;
        if (0 < na && na < 1) {
            // 交点位于 l 内
            in_l = true;
            intersection = (p0 + a * na);
            l->add_break_point(intersection);
        }

        const double nb = (a.x * c.y - a.y * c.x) * reciprocal;
        if (0 < nb && nb < 1) {
            // 交点也位于 r 内

            // 确保计算出来打断的点在数值上完全一致
            if (in_l) {
                r->add_break_point(intersection);
            }
            else {
                r->add_break_point(p2 + (p3-p2) * nb);
            }
        }
    }

    void point::add_edge(half_edge *e, bool is_emit)
    {
        edges.push_back({e, is_emit});
    }

    void point::remove(half_edge *e)
    {
        edges.erase(std::remove_if(edges.begin(), edges.end(), find_private_edge(e)), edges.end());
    }

    void point::sort()
    {
        struct sort_p
        {
            private_edge edge;
            sort_data data;
        };

        std::vector<sort_p> sort_vector;
        for(auto& e:edges){
            if(e.is_emit) {
                sort_vector.push_back({e,e.edge->get_sort_data()});
            }else {
                sort_vector.push_back({e,e.edge->twin->get_sort_data()});
            }
        }

        constexpr auto sort_algw = [](const sort_p& l, const sort_p& r) -> bool
        {
            auto le = l.edge;
            auto re = r.edge;

            if(le.edge->get_seg_type() == seg_type::LINETO && re.edge->get_seg_type() == seg_type::LINETO)
            {
                if(l.data.angle < r.data.angle) return true;
                if(l.data.angle > r.data.angle) return false;

                if(l.edge.is_emit && !(r.edge.is_emit)) return true;
                if(!(l.edge.is_emit) && r.edge.is_emit) return false;

                return l.edge.edge > r.edge.edge;
            }

            throw "out of sort condition";
            return false;
        };

        std::sort(sort_vector.begin(),sort_vector.end(),sort_algw);
        edges.clear();
        for(auto &e:sort_vector){
            edges.push_back(e.edge);
        }
    }

    half_edge* point::get_next(const half_edge* e) const
    {
        auto it = std::find_if(edges.begin(), edges.end(), find_private_edge(e));
        if(it == edges.end()) {
            return nullptr;
        }

        constexpr auto next = [](decltype(it)& iter, const decltype(edges)& container ){
            ++iter;
            if(iter == container.end()) {
                iter = container.begin();
            }
        };

        next(it, edges);
        while(it->is_emit == false || it->edge->prev){
            next(it, edges);
        }

        return it->edge;
    }

    half_edge* point::get_prev(const half_edge* e) const
    {
        auto it = std::find_if(edges.begin(), edges.end(), find_private_edge(e));
        if(it == edges.end()) {
            return nullptr;
        }

        if(it == edges.begin()){
            return edges.back().edge;
        }else {
            --it;
            return it->edge;
        }
    }

    std::vector<half_edge*> line_half_edge::process_break_point(dcel* parent)
    {
        std::vector<vec2> bp{ get_break_points().begin(),get_break_points().end()};
        if (bp.empty()) return {};

        struct compare
        {
            compare(const vec2& start, const vec2& end)
                : dx(end.x >= start.x), dy(end.y >= start.y) {}

            bool operator()(const vec2& r, const vec2& l) const {
                if (dx && r.x < l.x) return true;
                if ((!dx) && r.x > l.x) return true;
                if (dy && r.y < l.y) return true;
                if ((!dy) && r.y > l.y) return true;

                return false;
            }

            const bool dx;
            const bool dy;
        };

        std::sort(bp.begin(), bp.end(), compare(start->p,end->p));

        std::vector<half_edge*> res;

        auto cur_start = start;
        for (auto p : bp)
        {
            auto cur_end = parent->find_or_add_point(p.x, p.y);
            auto line = parent->new_line_edge(cur_start, cur_end);
            res.push_back(line);

            cur_start = cur_end;
        }
        if(bp.size() > 0) {
            res.push_back(parent->new_line_edge(cur_start, end));
        }

        return res;
    }

    half_edge* line_half_edge::build_twin(dcel* parent)
    {
        auto out = parent->new_line_edge(end, start);
        out->twin = this;
        this->twin = out;
        return out;
    }

    sort_data line_half_edge::get_sort_data() const
    {
        sort_data data;

        auto l = end->p - start->p;
        data.angle = std::atan2(l.y, l.x);
        if(data.angle == -rmath::pi){
            data.angle = -data.angle;
        }

        return data;
    }

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
        // 过滤掉点
        if (x1 == x2 && y1 == y2) return;

        point* start_p = find_or_add_point(x1, y1);
        point* end_p = find_or_add_point(x2, y2);

        // 只添加半边，剩下半边等待求交点后再补充
        new_line_edge(start_p, end_p);
    }

    void dcel::process()
    {
        process_break();
        build_twin();
        build_face();
    }

    void dcel::process_break()
    {
        scan_line::scan_line scan_line;
        for(auto e:edges){
            scan_line.add_segment(e);
        }
        scan_line.process();

        // 这里很重要，应为对 edges 插入删除会扰乱 set 的遍历
        std::vector<half_edge*> tmp_vector(edges.begin(), edges.end());

        for (auto edge:tmp_vector)
        {
            // 这里可能会对 set 进行插入
            auto replace = edge->process_break_point(this);
            if (replace.empty()) {
                continue;
            }

            // 在 set 中移除旧的 edge
            edges.erase(edge);
            edge->start->remove(edge);
            edge->end->remove(edge);
            delete edge;
        }
    }

    void dcel::build_twin()
    {
        // 这里很重要，应为对 edges 插入删除会扰乱 set 的遍历
        std::vector<half_edge*> tmp_vector(edges.begin(), edges.end());
        for(auto e : tmp_vector)
        {
            e->build_twin(this);
        }
    }

    void dcel::build_face()
    {
        for(auto p : point_set)
        {
            p->sort();
        }


        for(auto e:edges)
        {
            if(e->next != nullptr){
                continue;
            }

            auto f = new face;
            faces.push_back(f);
            f->start = e;

            auto cur_e = e;
            point* loop_start = e->start;
            while(cur_e->next == nullptr) {
                cur_e->face = f;
                auto next = cur_e->end->get_next(cur_e);
                {
                    //// 图形调试get_next边的选择
                    //qline_arrow(cur_e, QPen(Qt::red));
                    //for (auto _e : cur_e->end->edges) {
                    //    if (_e.is_emit && _e.edge->prev == nullptr) {
                    //        debug_util::show_line(qline(_e.edge));
                    //    }
                    //}
                    //qline_arrow(next, QPen(Qt::blue));
                }
                cur_e->next = next;
                next->prev = cur_e;

                if(next->end == loop_start){
                    next->next = e;
                    e->prev = next;
                    break;
                }
                cur_e = cur_e->next;
            }
        }
    }

    line_half_edge* dcel::new_line_edge(point* start, point* end)
    {
        auto line = new line_half_edge;

        line->start = start;
        line->end = end;
        line->type = current_set_type;

        edges.insert(line);
        start->add_edge(line, true);
        end->add_edge(line, false);
        return line;
    }

    point* dcel::find_or_add_point(num x, num y)
    {
        point* res;

        auto it = point_set.find(vec2(x,y));
        if (it == point_set.end()) {
            res = new point(x,y);
            point_set.insert(res);
        }
        else {
            res = *it;
        }

        return res;
    }

}
