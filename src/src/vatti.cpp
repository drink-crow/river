#include "vatti.h"
#include "rmath.h"
#include "scan_line.h"
#include "util.h"

#include <algorithm>

namespace scan_line
{
    using namespace vatti;
    using namespace rmath;

    template<>
    struct all_function<vertex*>
    {
        typedef ::rmath::rect box;
        typedef double ct;
        typedef vertex* key;

        static box get_rect(const key& in)
        {
            return in->next_seg->get_boundary(in->pt);
        }

        static void intersect(const key& r, const key& l, void* user)
        {
            ((clipper*)(user))->intersect(r, l);
        }
    };
}

namespace vatti
{
    using namespace river;
    using namespace rmath;

    // ToDO: 判断没齐全
    bool is_vaild(const Path& p) {
        return p.data.size() > 1;
    }

    /***************************************************
    *  Dx:                    0(90deg)                 *
    *                         |                        *
    *      +inf (180deg) <--- o ---> -inf (0deg)       *
    ***************************************************/
    inline double get_direct(const Point& pt1, const Point& pt2)
    {
        double dy = double(pt2.y - pt1.y);
        if (dy != 0)
            return double(pt2.x - pt1.x) / dy;
        else if (pt2.x > pt1.x)
            return -std::numeric_limits<double>::max();
        else
            return std::numeric_limits<double>::max();
    }

    // 虽然包含曲线内容，但是由于已经做了 Y 轴上单调的拆分，所有线段的方法仍然行得通
    inline void set_direct(edge* e)
    {
        e->dx = get_direct(e->bot, e->top);
    }

    void clipper::add_path(const Paths& paths, PathType polytype, bool is_open)
    {
        size_t total_vertex_count = 0;
        for (auto& path : paths)
        {
            if (path.data.size() >= 2) {
                if (is_vaild(path)) {
                    // auto close
                    total_vertex_count += path.data.size() + 1;
                }
            }
        }

        for (auto& path : paths)
        {
            if (!is_vaild(path)) continue;

            vertex *prev = nullptr, *cur = nullptr;
            vertex* first = new_vertex();
            paths_start.push_back(first);


            // ...===vertext===seg===vertex===seg===vertex...
            auto it = path.data.begin();
            first->pt = (*it)->get_target();
            ++it;
            prev = first;
            for (;it!= path.data.end();++it)
            {
                auto seg = *it;
                cur = new_vertex();
                
                // 打断曲线使其 y 轴单调，且不自交 
                // ToDO: 还要处理曲线等价与一条水平线的情况
                switch (seg->get_type())
                {
                case SegType::CubicTo: {
                    auto cubicto = static_cast<Seg_cubicto*>(seg);

                    bezier_cubic b{ prev->pt, cubicto->ctrl_Point1, cubicto->ctrl_Point2, cubicto->target };
                    
                    double split_t[2];
                    auto break_cnt = b.split_y(split_t, split_t + 1);
                    bezier_cubic tmp[3];
                    b.split(split_t, tmp, break_cnt);
                    switch (break_cnt)
                    {
                    case 2:
                        set_segment(prev, cur, new Seg_cubicto(tmp[0].p1, tmp[0].p2, tmp[0].p3));
                        prev = cur; cur = new_vertex();
                        set_segment(prev, cur, new Seg_cubicto(tmp[1].p1, tmp[1].p2, tmp[1].p3));
                        prev = cur; cur = new_vertex();
                        set_segment(prev, cur, new Seg_cubicto(tmp[2].p1, tmp[2].p2, tmp[2].p3));
                        break;
                    case 1:
                        set_segment(prev, cur, new Seg_cubicto(tmp[0].p1, tmp[0].p2, tmp[0].p3));
                        prev = cur; cur = new_vertex();
                        set_segment(prev, cur, new Seg_cubicto(tmp[1].p1, tmp[1].p2, tmp[1].p3));
                        break;
                    default:
                        set_segment(prev, cur, seg->deep_copy());
                        break;
                    }
                }
                    break;
                default:
                    // ToDO: 别的曲线类型
                    // ToDO: 删去重复的直线
                    set_segment(prev, cur, seg->deep_copy());
                    break;
                }

                prev = cur;
            }
            // ToDO: 将最后一点合并至起点，这里还应该判断时候闭合了，没有的话给它拉一条直线自动闭合
            set_segment(cur->prev, first, cur->prev->next_seg);

#if 0
            {
                auto cur_v = first;
                QPen redpen(Qt::red);
                redpen.setWidth(3);
                do {
                    switch (cur_v->next_seg->get_type())
                    {
                    case SegType::LineTo:
                        debug_util::show_line(QLineF(toqt(cur_v->pt), toqt(cur_v->next_seg->get_target())), redpen);
                        break;
                    case SegType::CubicTo:
                    {
                        auto cubic = (const Seg_cubicto*)(cur_v->next_seg);
                        debug_util::show_cubic(toqt(cur_v->pt), toqt(cubic->ctrl_Point1), toqt(cubic->ctrl_Point2), toqt(cur_v->next->pt), redpen);
                        break;
                    }
                    default:
                        break;
                    }


                    cur_v = cur_v->next;
                } while (cur_v != first);
            }
#endif

            // form local minma

            bool going_down, going_down0;
            {
                // ToDO: 这里要补充往前找第一个不为水平的比较，顺便检查是不是完全水平的一个多边形
                going_down = first->pt.y < first->prev->pt.y;
                going_down0 = going_down;
            }
            prev = first;
            cur = first->next;
            while (cur != first)
            {
                // 水平线会被跳过
                // 水平线的的放置规则在后面的扫描过程处理
                if (cur->pt.y > prev->pt.y && going_down) {
                    going_down = false;
                    add_local_min(prev, polytype, is_open);
                }
                else if (cur->pt.y < prev->pt.y && !going_down) {
                    prev->flags = prev->flags | vertex_flags::local_max;
                    going_down = true;
                }

                prev = cur;
                cur = cur->next;
            }

            // 处理最后的连接，实际判断的点是 first->prev
            if (going_down != going_down0) {
                if (going_down0) {
                    prev->flags = prev->flags | vertex_flags::local_max;
                }
                else {
                    add_local_min(prev, polytype, is_open);
                }
            }
#if 0
        // 图形调试
        QPen redpen(Qt::red);
        QPen bluePen(Qt::blue);
        for (auto& minima : local_minima_list) {
            debug_util::show_point(toqt(minima->pt->pt), redpen);
        }
        
        auto _s = first;
        do {
            if ((_s->flags & vertex_flags::local_max) != vertex_flags::none) {
                debug_util::show_point(toqt(_s->pt), bluePen);
            }

            _s = _s->next;
        } while (_s != first);

#endif
        }
    }

    void clipper::process()
    {
        process_intersect();

        // build init scan line list
        std::sort(local_minima_list.begin(), local_minima_list.end(),
            [](local_minima* const& l, local_minima* const& r) {
                if (l->vert->pt.y != r->vert->pt.y) { return l->vert->pt.y < r->vert->pt.y; }
                else { return l->vert->pt.x < r->vert->pt.x; }
            });

        for (auto it = local_minima_list.rbegin(); it != local_minima_list.rend(); ++it) {
            scanline_list.push((*it)->vert->pt.y);
        }

        cur_locmin_it = local_minima_list.begin();


        num y;
        if (!pop_scanline(y)) return;

        while (true)
        {
            insert_local_minima_to_ael(y);
        }
    }

    vertex* clipper::new_vertex()
    {
        auto v =  vertex_pool.malloc();
        v->flags = vertex_flags::none;
        v->next_seg = nullptr;
        v->next = nullptr;
        v->prev = nullptr;

        return v;
    }

    void clipper::process_intersect()
    {
        break_info_list.clear();

        // 先求交点
        scan_line::scan_line<vertex*> scaner;
        for (auto& start : paths_start) {
            auto cur = start;
            do {
                scaner.add_segment(cur);
                cur = cur->next;
            } while (cur != start);
        }
        scaner.process(this);

        // 处理得到的打断信息

        std::sort(break_info_list.begin(), break_info_list.end(), 
            [](break_info const& l, break_info const& r) {return l.vert < r.vert; });
        break_info_list.erase(std::unique(break_info_list.begin(), break_info_list.end()), break_info_list.end());

        auto start = break_info_list.begin();
        auto end = start;

        while (start != break_info_list.end()) {
            auto start_v = start->vert;
            auto cur_v = start_v;
            auto end_v = cur_v->next;

            while (end != break_info_list.end() && end->vert == cur_v) {
                ++end;
            }

            auto start_p = cur_v->pt;
            auto start_seg = cur_v->next_seg;
            auto end_p = cur_v->next->pt;
            switch (start_seg->get_type())
            {
            case SegType::LineTo:
            {
                std::vector<break_info> cur_break_list(start, end);
                auto dx = end_p.x >= start_p.x;
                auto dy = end_p.y >= start_p.y;
                std::sort(cur_break_list.begin(), cur_break_list.end(),
                    [dx, dy](break_info const& li, break_info const& ri) {
                        auto l = li.break_point;
                        auto r = ri.break_point;

                        if (dx && r.x < l.x) return true;
                        if ((!dx) && r.x > l.x) return true;
                        if (dy && r.y < l.y) return true;
                        if ((!dy) && r.y > l.y) return true;

                        return false;
                    });

                ((Seg_lineto*)start_seg)->target = cur_break_list[0].break_point;
                auto next = new_vertex();
                set_segment(start_v, next, start_seg);
                cur_v = next;
                for (size_t i = 1; i < cur_break_list.size(); ++i) {
                    set_segment(cur_v, new_vertex(), new Seg_lineto(cur_break_list[i].break_point));
                    cur_v = cur_v->next;
                }
                auto end_flags = end_v->flags;
                set_segment(cur_v, end_v, new Seg_lineto(end_v->pt));
                end_v->flags = end_flags;
                break;
            }
            case SegType::CubicTo:
            {
                std::vector<break_info> cur_break_list(start, end);
                std::sort(cur_break_list.begin(), cur_break_list.end(),
                    [](break_info const& li, break_info const& ri) {
                        return li.t < ri.t;
                    });


                std::vector<bezier_cubic> split_c(cur_break_list.size() + 1);
                std::vector<double> split_t;
                for (auto& info : cur_break_list) split_t.push_back(info.t);
                auto first_cubicto = (Seg_cubicto*)start_seg;
                first_cubicto->get_cubic(start_p).split(split_t.data(), split_c.data(), split_t.size());

                first_cubicto->ctrl_Point1 = split_c[0].p1;
                first_cubicto->ctrl_Point2 = split_c[0].p2;
                first_cubicto->target = split_c[0].p3;
                auto next = new_vertex();
                set_segment(start_v, next, first_cubicto);
                cur_v = next;
                for (size_t i = 1; i < cur_break_list.size(); ++i) {
                    set_segment(cur_v, new_vertex(), new Seg_cubicto(
                        split_c[i].p1, split_c[i].p2, cur_break_list[i].break_point));
                    cur_v = cur_v->next;
                }
                auto end_flags = end_v->flags;
                set_segment(cur_v, end_v, new Seg_cubicto(split_c.back().p1, split_c.back().p2, end_v->pt));
                end_v->flags = end_flags;

                break;
            }
            default:
                break;
            }

            start = end;
        }

#if 0
        // 图形测试打断是否正确处理
        for(auto &first : paths_start)
        {
            auto cur_v = first;
            QPen redpen(Qt::red);
            redpen.setWidth(3);
            do {
                switch (cur_v->next_seg->get_type())
                {
                case SegType::LineTo:
                    debug_util::show_line(QLineF(toqt(cur_v->pt), toqt(cur_v->next_seg->get_target())), redpen);
                    break;
                case SegType::CubicTo:
                {
                    auto cubic = (const Seg_cubicto*)(cur_v->next_seg);
                    debug_util::show_cubic(toqt(cur_v->pt), toqt(cubic->ctrl_Point1), toqt(cubic->ctrl_Point2), toqt(cur_v->next->pt), redpen);
                    break;
                }
                default:
                    break;
                }


                cur_v = cur_v->next;
            } while (cur_v != first);
        }
#endif
    }

    void clipper::set_segment(vertex* prev, vertex* mem, Seg* move_seg)
    {
        prev->next = mem;
        prev->next_seg = move_seg;

        mem->prev = prev;
        mem->pt = move_seg->get_target();
        mem->flags = vertex_flags::none;
    }

    bool clipper::pop_scanline(num& y)
    {
        if (scanline_list.empty()) return false;
        y = scanline_list.top();
        scanline_list.pop();
        while (!scanline_list.empty() && y == scanline_list.top())
            scanline_list.pop();  // Pop duplicates.
        return true;
    }

    void clipper::insert_local_minima_to_ael(num y)
    {
        local_minima* lmin = nullptr;
        edge *left_bound, *right_bound;

        while(pop_local_minima(y, &lmin))
        {
            // 先设置左右，一会再调换
            left_bound = new edge();
            left_bound->bot = lmin->vert->pt;
            left_bound->curr_x = left_bound->bot.x;
            left_bound->wind_cnt = 0,
                left_bound->wind_cnt2 = 0,
                left_bound->wind_dx = -1,
                left_bound->vertex_top = lmin->vert->prev;  // ie descending
            left_bound->top = left_bound->vertex_top->pt;
            left_bound->out_poly = nullptr;
            left_bound->local_min = lmin;
            set_direct(left_bound);

            right_bound = new edge();
            right_bound->bot = lmin->vert->pt;
            right_bound->curr_x = right_bound->bot.x;
            right_bound->wind_cnt = 0,
                right_bound->wind_cnt2 = 0,
                right_bound->wind_dx = 1,
                right_bound->vertex_top = lmin->vert->next;  // ie ascending
            right_bound->top = right_bound->vertex_top->pt;
            right_bound->out_poly = nullptr;
            right_bound->local_min = lmin;
            set_direct(right_bound);
        }
    }

    bool clipper::pop_local_minima(num y, local_minima** out)
    {
        if (cur_locmin_it == local_minima_list.end() || (*cur_locmin_it)->vert->pt.y != y) return false;
        *out = (*cur_locmin_it);
        ++cur_locmin_it;
        return true;
    }

    void clipper::intersect(vertex* const& l, vertex* const& r)
    {

        auto rseg = r->next_seg;
        auto lseg = l->next_seg;

        auto rt = r->next_seg->get_type();
        auto lt = l->next_seg->get_type();

        uint32_t t = flags(rt) & flags(lt);

        switch (t)
        {
        case flags(SegType::LineTo):
        {
            auto res = rmath::intersect(l->pt, lseg->get_target(), r->pt, rseg->get_target());
            if (res.count == 1) {
                auto& data = res.data[0];
                break_info_list.push_back({ l,data.p,data.t0 });
                break_info_list.push_back({ r,data.p,data.t1 });
            }
            else if (res.count == 2) {
                for (int i = 0; i < res.count; ++i) {
                    auto& data = res.data[i];
                    if (0 < data.t0 && data.t0 < 1) break_info_list.push_back({ l,data.p,data.t0 });
                    if (0 < data.t1 && data.t1 < 1) break_info_list.push_back({ r,data.p,data.t1 });
                }
            }
            break;
        }
        case flags(SegType::LineTo)& flags(SegType::CubicTo):
        {
            vertex* line, * cubic;
            if (lt == SegType::LineTo) { line = l; cubic = r; }
            else { line = r; cubic = l; }

            rmath::line _line{ line->pt, line->next_seg->get_target() };
            auto _cubic_to = (const Seg_cubicto*)(cubic->next_seg);
            rmath::bezier_cubic _curve{ cubic->pt, _cubic_to->ctrl_Point1,_cubic_to->ctrl_Point2,_cubic_to->target };

            auto res = rmath::intersect(_line, _curve);
            for (int i = 0; i < res.count; ++i) {
                auto& data = res.data[i];
                if (0 < data.t0 && data.t0 < 1) break_info_list.push_back({ line,data.p,data.t0 });
                if (0 < data.t1 && data.t1 < 1) break_info_list.push_back({ cubic,data.p,data.t1 });
            }
            break;
        }
        default:
            break;
        }

    }

    void clipper::add_local_min(vertex* vert, PathType pt, bool is_open)
    {
        //make sure the vertex is added only once ...
        if ((vertex_flags::local_min & vert->flags) != vertex_flags::none) return;

        vert->flags = (vert->flags | vertex_flags::local_min);
        local_minima_list.push_back(new local_minima(vert, pt, is_open));
    }
}
