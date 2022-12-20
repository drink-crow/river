#include "vatti.h"
#include "rmath.h"

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
                
                // 打断曲线使其 y 轴单调递增 
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

            // ToDO: 还有最后的没处理
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
    void clipper::add_local_min(vertex* vert, PathType pt, bool is_open)
    {
        //make sure the vertex is added only once ...
        if ((vertex_flags::local_min & vert->flags) != vertex_flags::none) return;

        vert->flags = (vert->flags | vertex_flags::local_min);
        local_minima_list.push_back(new local_minima(vert, pt, is_open));
    }
}
