#include "vatti.h"
#include "rmath.h"
#include "scan_line.h"
#include "util.h"

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

            constexpr auto add_segment = [](vertex* prev, vertex* mem, Seg* move_seg) {
                prev->next = mem;
                prev->next_seg = move_seg;

                mem->prev = prev;
                mem->pt = move_seg->get_target();
                mem->flags = vertex_flags::none;
            };

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
                        add_segment(prev, cur, new Seg_cubicto(tmp[0].p1, tmp[0].p2, tmp[0].p3));
                        prev = cur; cur = new_vertex();
                        add_segment(prev, cur, new Seg_cubicto(tmp[1].p1, tmp[1].p2, tmp[1].p3));
                        prev = cur; cur = new_vertex();
                        add_segment(prev, cur, new Seg_cubicto(tmp[2].p1, tmp[2].p2, tmp[2].p3));
                        break;
                    case 1:
                        add_segment(prev, cur, new Seg_cubicto(tmp[0].p1, tmp[0].p2, tmp[0].p3));
                        prev = cur; cur = new_vertex();
                        add_segment(prev, cur, new Seg_cubicto(tmp[1].p1, tmp[1].p2, tmp[1].p3));
                        break;
                    default:
                        add_segment(prev, cur, seg->deep_copy());
                        break;
                    }
                }
                    break;
                default:
                    // ToDO: 别的曲线类型
                    // ToDO: 删去重复的直线
                    add_segment(prev, cur, seg->deep_copy());
                    break;
                }

                prev = cur;
            }
            // ToDO: 将最后一点合并至起点，这里还应该判断时候闭合了，没有的话给它拉一条直线自动闭合
            add_segment(cur->prev, first, cur->prev->next_seg);

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
    vertex* clipper::new_vertex()
    {
        auto v =  vertex_pool.malloc();
        v->flags = vertex_flags::none;
        v->next_seg = nullptr;
        v->next = nullptr;
        v->prev = nullptr;

        return v;
    }

    void clipper::process()
    {
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
                break_info_list.push_back({ l,data.p });
                break_info_list.push_back({ r,data.p });
            }
            else if (res.count == 2) {
                for (int i = 0; i < res.count; ++i) {
                    auto& data = res.data[i];
                    if (0 < data.t0 && data.t0 < 1) break_info_list.push_back({ l,data.p });
                    if (0 < data.t1 && data.t1 < 1) break_info_list.push_back({ r,data.p });
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
