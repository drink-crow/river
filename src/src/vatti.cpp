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

    // retrun true if a->b->c is ccw
    inline double cross_product(const Point& a, const Point& b, const Point& c) {
        return (b - a).cross(c - b);
    }

    // 取edge构造的直线中，y = currentY 时 x 的值
    // ToDo 显然的不适应曲线内容
    inline num top_x(const edge* ae, const num currentY)
    {
        if ((currentY == ae->top.y) || (ae->top.x == ae->bot.x)) return ae->top.x;
        else if (currentY == ae->bot.y) return ae->bot.x;
        else return ae->bot.x + static_cast<int64_t>(std::nearbyint(ae->dx * (currentY - ae->bot.y)));
        // nb: std::nearbyint (or std::round) substantially *improves* performance here
        // as it greatly improves the likelihood of edge adjacency in ProcessIntersectList().
    }

    // 切记要提前处理了等价水平的曲线
    inline bool is_horizontal(const edge& e)
    {
        return (e.top.y == e.bot.y);
    }

    inline bool is_horizontal(const edge* e) {
        return is_horizontal(*e);
    }

    inline bool horz_is_heading_right(const edge& e)
    {
        return e.dx == -std::numeric_limits<double>::max();
    }

    inline bool horz_is_heading_left(const edge& e)
    {
        return e.dx == std::numeric_limits<double>::max();
    }

    inline void swap_edge(edge** a, edge** b) {
        edge* e = *a;
        *a = *b;
        *b = e;
    }

    inline PathType get_poly_type(const edge* e) {
        return e->local_min->polytype;
    }

    // always false
    inline bool is_open(const edge* e) {
        return false;
    }

    /***************************************************
    *  Dx:                    0(90deg)                 *
    *                         |                        *
    *      -inf (180deg) <--- o ---> +inf (0deg)       *
    ***************************************************/
    inline double get_direct(const Point& pt1, const Point& pt2)
    {
        double dy = double(pt2.y - pt1.y);
        if (dy != 0)
            return double(pt2.x - pt1.x) / dy;
        else if (pt2.x > pt1.x)
            return std::numeric_limits<double>::max();
        else
            return -std::numeric_limits<double>::max();
    }

    // ToDo 对于曲线内容行不通
    inline void set_direct(edge* e)
    {
        e->dx = get_direct(e->bot, e->top);
    }

    // 获取Y轴正方向上的下一个点
    inline vertex* next_vertex(const edge* e)
    {
        if (e->wind_dx < 0)
            return e->vertex_top->next;
        else
            return e->vertex_top->prev;
    }

    inline vertex* prev_prev_vertex(const edge* ae)
    {
        if (ae->wind_dx < 0)
            return ae->vertex_top->prev->prev;
        else
            return ae->vertex_top->next->next;
    }

    inline bool is_maxima(const vertex& v)
    {
        return ((v.flags & vertex_flags::local_max) != vertex_flags::none);
    }

    // return is_maxima(*e->vertex_top)
    inline bool is_maxima(const edge* e)
    {
        return is_maxima(*e->vertex_top);
    }

    // hot_edge mean have out_poly
    inline bool is_hot_edge(const edge* e)
    {
        return (e->out_poly);
    }

    inline edge* get_prev_hot_edge(const edge* e)
    {
        edge* prev = e->prev_in_ael;
        while (prev && (is_open(prev) || !is_hot_edge(prev)))
            prev = prev->prev_in_ael;
        return prev;
    }

    inline bool outpoly_is_ascending(const edge* hotEdge)
    {
        return (hotEdge == hotEdge->out_poly->front_edge);
    }

    inline void set_sides(out_polygon* out_poly, edge* start_edge, edge* end_edge)
    {
        out_poly->front_edge = start_edge;
        out_poly->back_edge = end_edge;
    }

    // 测试和 e->prev_in_ael 是否共线，且均是hot_edge，这意味着可以合并，多数出现在
    // 两个输出轮廓相接
    // ToDo 不适合曲线内容
    inline bool test_join_with_prev1(const edge* e)
    {
        //this is marginally quicker than TestJoinWithPrev2
        //but can only be used when e.PrevInAEL.currX is accurate
        auto prev = e->prev_in_ael;
        return is_hot_edge(e) && !is_open(e) &&
            prev && prev->curr_x == e->curr_x &&
            is_hot_edge(prev) && !is_open(prev) &&
            (cross_product(prev->top, e->bot, e->top) == 0);
    }

    inline bool test_join_with_next1(const edge* e)
    {
        //this is marginally quicker than TestJoinWithNext2
        //but can only be used when e.NextInAEL.currX is accurate
        return is_hot_edge(e) && !is_open(e) &&
            e->next_in_ael && (e->next_in_ael->curr_x == e->curr_x) &&
            is_hot_edge(e->next_in_ael) && !is_open(e->next_in_ael) &&
            (cross_product(e->next_in_ael->top, e->bot, e->top) == 0);
    }

    // e is front edge of its out_poly
    inline bool is_front(const edge* e)
    {
        return (e == e->out_poly->front_edge);
    }

    inline bool is_open_end(const edge* e) {
        return false;
    }

    inline bool IsSamePolyType(const edge& e1, const edge& e2)
    {
        return e1.local_min->polytype == e2.local_min->polytype;
    }

    inline bool IsSamePolyType(const edge* e1, const edge* e2)
    {
        return IsSamePolyType(*e1, *e2);
    }

    // 试图获取连接这条边的 Y 轴向上第一个不水平且不是 maxima 的点
    vertex* get_currY_maxima_vertex(const edge* e)
    {
        vertex* result = e->vertex_top;
        if (e->wind_dx > 0)
            while (result->next->pt.y == result->pt.y) result = result->next;
        else
            while (result->prev->pt.y == result->pt.y) result = result->prev;
        if (!is_maxima(*result)) result = nullptr; // not a maxima   
        return result;
    }

    edge* get_horz_maxima_pair(const edge* horz, const vertex* vert_max)
    {
        //we can't be sure whether the MaximaPair is on the left or right, so ...
        auto result = horz->prev_in_ael;
        while (result && result->curr_x >= vert_max->pt.x)
        {
            if (result->vertex_top == vert_max) return result;  // Found!
            result = result->prev_in_ael;
        }
        result = horz->next_in_ael;
        while (result && top_x(result, horz->top.y) <= vert_max->pt.x)
        {
            if (result->vertex_top == vert_max) return result;  // Found!
            result = result->next_in_ael;
        }
        return nullptr;
    }

    inline void TrimHorz(edge* horzEdge, bool preserveCollinear)
    {
        bool wasTrimmed = false;
        Point pt = next_vertex(horzEdge)->pt;
        while (pt.y == horzEdge->top.y)
        {
            //always trim 180 deg. spikes (in closed paths)
            //but otherwise break if preserveCollinear = true
            if (preserveCollinear &&
                ((pt.x < horzEdge->top.x) != (horzEdge->bot.x < horzEdge->top.x)))
                break;

            horzEdge->vertex_top = next_vertex(horzEdge);
            horzEdge->top = pt;
            wasTrimmed = true;
            if (is_maxima(horzEdge)) break;
            pt = next_vertex(horzEdge)->pt;
        }

        if (wasTrimmed) set_direct(horzEdge); // +/-infinity
    }

    inline void UncoupleOutRec(edge* ae)
    {
        out_polygon* outrec = ae->out_poly;
        if (!outrec) return;
        outrec->front_edge->out_poly = nullptr;
        outrec->back_edge->out_poly = nullptr;
        outrec->front_edge = nullptr;
        outrec->back_edge = nullptr;
    }

    // 在 ael 中往后查找一个和 e 拥有相同 vertex_top（即同一个轮廓）的 active edge
    edge* get_maxima_pair(const edge* e)
    {
        edge* e2 = e->next_in_ael;
        while (e2)
        {
            // 由于 ael 是排序过往后遍历的，所以这里也只需要往后遍历
            if (e2->vertex_top == e->vertex_top) return e2;  // Found!
            e2 = e2->next_in_ael;
        }
        return nullptr;
    }

    void SwapOutrecs(edge& e1, edge& e2)
    {
        out_polygon* or1 = e1.out_poly;
        out_polygon* or2 = e2.out_poly;
        if (or1 == or2)
        {
            edge* e = or1->front_edge;
            or1->front_edge = or1->back_edge;
            or1->back_edge = e;
            return;
        }
        if (or1)
        {
            if (&e1 == or1->front_edge)
                or1->front_edge = &e2;
            else
                or1->back_edge = &e2;
        }
        if (or2)
        {
            if (&e2 == or2->front_edge)
                or2->front_edge = &e1;
            else
                or2->back_edge = &e1;
        }
        e1.out_poly = or2;
        e2.out_poly = or1;
    }

    void SwapOutrecs(edge* e1, edge* e2)
    {
        SwapOutrecs(*e1, *e2);
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
            insert_scanline((*it)->vert->pt.y);
        }

        cur_locmin_it = local_minima_list.begin();


        num y;
        if (!pop_scanline(y)) return;

        while (true)
        {
            insert_local_minima_to_ael(y);
            edge* e;
            while (pop_horz(&e)) do_horizontal(e);
            //if (horz_joiners_) ConvertHorzTrialsToJoins();
            bot_y_ = y;  // bot_y_ == bottom of scanbeam
            if (!pop_scanline(y)) break;  // y new top of scanbeam
            /* 由于intersect已经提前处理了，因此只需要在添加 local minima 和 
            DoTopOfScanbeam() 时处理端点的相交就 可以了*/
            //DoIntersections(y);
            DoTopOfScanbeam(y);
            while (pop_horz(&e)) do_horizontal(e);
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
                left_bound->wind_dx = 1,
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
                right_bound->wind_dx = -1,
                right_bound->vertex_top = lmin->vert->next;  // ie ascending
            right_bound->top = right_bound->vertex_top->pt;
            right_bound->out_poly = nullptr;
            right_bound->local_min = lmin;
            set_direct(right_bound);

            // 水平线的放置位置规则是由将水平线当成略微倾斜的线来处理的方法延申出来的
            // ToDo 这里应该要改改，因为 Clipper 本身是自上到下的。
            if (is_horizontal(left_bound))
            {
                if (horz_is_heading_right(*left_bound)) swap_edge(&left_bound, &right_bound);
            }
            else if (is_horizontal(right_bound))
            {
                if (horz_is_heading_left(*right_bound)) swap_edge(&left_bound, &right_bound);
            }
            // ToDo 这里判断不足以应对曲线内容
            else if (left_bound->dx > right_bound->dx)
                swap_edge(&left_bound, &right_bound);

            // 开始处理 left_bound
            bool contributing;
            left_bound->is_left_bound = true;
            insert_left_edge(left_bound);

            // 计算只用到左边的 windcount 所以右边没插入ael时也可以
            set_windcount_closed(left_bound);
            contributing = is_contributing_closed(left_bound);

            // 开始处理 right_bound
            right_bound->is_left_bound = false;
            right_bound->wind_cnt = left_bound->wind_cnt;
            right_bound->wind_cnt2 = left_bound->wind_cnt2;
            insert_right_edge(left_bound, right_bound);


            if (contributing)
            {
                add_local_min_poly(left_bound, right_bound, left_bound->bot, true);
                if (!is_horizontal(*left_bound) && test_join_with_prev1(left_bound))
                {
                    auto op = add_out_pt(left_bound->prev_in_ael, left_bound->bot);
                    add_join(op, left_bound->out_poly->pts);
                }
            }

            /* right_bound是直接插入到left_bound的后面的，在bot还有其他边的时候有可
             * 能顺序不对，这种情况则要逐一做intersect处理和交换顺序，直到 ael 的顺
             * 序是对的
            */
            while (right_bound->next_in_ael &&
                is_valid_ael_order(right_bound->next_in_ael, right_bound))
            {
                intersect_edges(right_bound, right_bound->next_in_ael, right_bound->bot);
                SwapPositionsInAEL(right_bound, right_bound->next_in_ael);
            }

            if (!is_horizontal(*right_bound) &&
                test_join_with_next1(right_bound))
            {
                auto op = add_out_pt(right_bound->next_in_ael, right_bound->bot);
                add_join(right_bound->out_poly->pts, op);
            }

            if (is_horizontal(*right_bound))
                push_horz(right_bound);
            else
                insert_scanline(right_bound->top.y);

            if (is_horizontal(*left_bound))
                push_horz(left_bound);
            else
                insert_scanline(left_bound->top.y);
        }
    }

    bool clipper::pop_local_minima(num y, local_minima** out)
    {
        if (cur_locmin_it == local_minima_list.end() || (*cur_locmin_it)->vert->pt.y != y) return false;
        *out = (*cur_locmin_it);
        ++cur_locmin_it;
        return true;
    }

    void clipper::insert_left_edge(edge* e)
    {
        edge* e2;
        if (!ael_first)
        {
            e->prev_in_ael = nullptr;
            e->next_in_ael = nullptr;
            ael_first = e;
        }
        // 测试能否添加到 ael_first 右边（next 方向）
        else if (!is_valid_ael_order(ael_first, e))
        {
            // 只能添加到 ael_first 左边（prev 方向） 
            e->prev_in_ael = nullptr;
            e->next_in_ael = ael_first;
            ael_first->prev_in_ael = e;
            ael_first = e;
        }
        else
        {
            e2 = ael_first;
            // 往 next 方向逐一测试可以插入的位置
            while (e2->next_in_ael && is_valid_ael_order(e2->next_in_ael, e))
                e2 = e2->next_in_ael;
            e->next_in_ael = e2->next_in_ael;
            if (e2->next_in_ael) e2->next_in_ael->prev_in_ael = e;
            e->prev_in_ael = e2;
            e2->next_in_ael = e;
        }
    }

    // 关键部分，这里与后面的 succ 选取函数构成输出构造的核心
    // 实际上这里的 newcomer 有隐藏的条件，一定是 local minima 的左右边或者是由 intersection 整出来的新 local minima 
    bool clipper::is_valid_ael_order(const edge* resident, const edge* newcomer)
    {
        // ToDo 这里 cross_product 不能覆盖曲线

        // 直接以 curr_x 进行排序
        if (newcomer->curr_x != resident->curr_x)
            return newcomer->curr_x > resident->curr_x;

        // 比较麻烦的是当 curr_x 相同的时候
        
        // 这时候是相交于 newcomer->bot
        double d = cross_product(resident->top, newcomer->bot, newcomer->top);
        if (d != 0) return d > 0;

        // 处理更特殊的共线情况
        //edges must be collinear to get here
        //for starting open paths, place them according to
        //the direction they're about to turn
        if (!is_maxima(resident) && (resident->top.y > newcomer->top.y))
        {
            // resident 不是 maxima 且 newcomer 更小时，就看 resident 接下来的走向是不是 cw
            return cross_product(newcomer->bot,
                resident->top, next_vertex(resident)->pt) <= 0;
        }
        else if (!is_maxima(newcomer) && (newcomer->top.y > resident->top.y))
        {
            // newcomer 不是 maxima 且 resident 更小时，就看 next_vertex 接下来的走向是不是 ccw
            return cross_product(newcomer->bot,
                newcomer->top, next_vertex(newcomer)->pt) >= 0;
        }

        // 剩下是更更特殊的情况

        int64_t y = newcomer->bot.y;
        bool newcomer_is_left = newcomer->is_left_bound;

        // resident 是扫描线下边边时，left_bound 的 newcomer 优先
        if (resident->bot.y != y || resident->local_min->vert->pt.y != y)
            return newcomer->is_left_bound;
        //resident must also have just been inserted
        // unlike 组合，以 left_bound 的优先, 例如 resident 和 newcomer 完全重合
        else if (resident->is_left_bound != newcomer_is_left)
            return newcomer_is_left;
        // like 组合就看 resident 和之前是不是一条直线
        else if (cross_product(prev_prev_vertex(resident)->pt,
            resident->bot, resident->top) == 0) return true;
        else
            //compare turning direction of the alternate bound
            return (cross_product(prev_prev_vertex(resident)->pt,
                newcomer->bot, prev_prev_vertex(newcomer)->pt) > 0) == newcomer_is_left;
    }

    // ToDo 这部分直接从 Clipper 复制而来，尝试试了几个案例都是对的 但是完全不懂这个
    // 规则是怎么来的, 文章中提到了 contributing 这个概念，但是完全没提是怎么计算的
    bool clipper::is_contributing_closed(edge* e)
    {
        switch (fillrule_)
        {
        case fill_rule::even_odd:
            break;
        case fill_rule::non_zero:
            if (abs(e->wind_cnt) != 1) return false;
            break;
        case fill_rule::positive:
            if (e->wind_cnt != 1) return false;
            break;
        case fill_rule::negative:
            if (e->wind_cnt != -1) return false;
            break;
        }

        switch (cliptype_)
        {
        case clip_type::none:
            return false;
        case clip_type::intersection:
            switch (fillrule_)
            {
            case fill_rule::positive:
                return (e->wind_cnt2 > 0);
            case fill_rule::negative:
                return (e->wind_cnt2 < 0);
            default:
                return (e->wind_cnt2 != 0);
            }
            break;

        case clip_type::Union:
            switch (fillrule_)
            {
            case fill_rule::positive:
                return (e->wind_cnt2 <= 0);
            case fill_rule::negative:
                return (e->wind_cnt2 >= 0);
            default:
                return (e->wind_cnt2 == 0);
            }
            break;

        case clip_type::difference:
            bool result;
            switch (fillrule_)
            {
            case fill_rule::positive:
                result = (e->wind_cnt2 <= 0);
                break;
            case fill_rule::negative:
                result = (e->wind_cnt2 >= 0);
                break;
            default:
                result = (e->wind_cnt2 == 0);
            }
            if (get_poly_type(e) == PathType::Subject)
                return result;
            else
                return !result;
            break;

        case clip_type::Xor: return true;  break;
        }
        return false;  // we should never get here
    }

    void clipper::add_join(out_pt* op1, out_pt* op2)
    {
        if ((op1->outrec == op2->outrec) && ((op1 == op2) ||
            //unless op1.next or op1.prev crosses the start-end divide
            //don't waste time trying to join adjacent vertices
            ((op1->next == op2) && (op1 != op1->outrec->pts)) ||
            ((op2->next == op1) && (op2 != op1->outrec->pts)))) return;

        joiner* j = new joiner(op1, op2, nullptr);
        j->idx = static_cast<int>(joiner_list_.size());
        joiner_list_.push_back(j);
    }

    void clipper::insert_scanline(num y)
    {
        scanline_list.push(y);
    }

    out_pt* clipper::intersect_edges(edge* e1, edge* e2, const Point& pt)
    {
        //MANAGE OPEN PATH INTERSECTIONS SEPARATELY ...

        // 暂时跳过非闭合线段
#if 0
        if (has_open_paths_ && (IsOpen(e1) || IsOpen(e2)))
        {
            if (IsOpen(e1) && IsOpen(e2)) return nullptr;

            Active* edge_o, * edge_c;
            if (IsOpen(e1))
            {
                edge_o = &e1;
                edge_c = &e2;
            }
            else
            {
                edge_o = &e2;
                edge_c = &e1;
            }

            if (abs(edge_c->wind_cnt) != 1) return nullptr;
            switch (cliptype_)
            {
            case clip_type::Union:
                if (!is_hot_edge(*edge_c)) return nullptr;
                break;
            default:
                if (edge_c->local_min->polytype == PathType::Subject)
                    return nullptr;
            }

            switch (fillrule_)
            {
            case fill_rule::Positive: if (edge_c->wind_cnt != 1) return nullptr; break;
            case fill_rule::Negative: if (edge_c->wind_cnt != -1) return nullptr; break;
            default: if (std::abs(edge_c->wind_cnt) != 1) return nullptr; break;
            }

            //toggle contribution ...
            if (is_hot_edge(*edge_o))
            {
                OutPt* resultOp = add_out_pt(*edge_o, pt);
#ifdef USINGZ
                if (zCallback_) SetZ(e1, e2, resultOp->pt);
#endif
                if (is_front(*edge_o)) edge_o->outrec->front_edge = nullptr;
                else edge_o->outrec->back_edge = nullptr;
                edge_o->outrec = nullptr;
                return resultOp;
            }

            //horizontal edges can pass under open paths at a LocMins
            else if (pt == edge_o->local_min->vertex->pt &&
                !IsOpenEnd(*edge_o->local_min->vertex))
            {
                //find the other side of the LocMin and
                //if it's 'hot' join up with it ...
                Active* e3 = FindEdgeWithMatchingLocMin(edge_o);
                if (e3 && is_hot_edge(*e3))
                {
                    edge_o->outrec = e3->outrec;
                    if (edge_o->wind_dx > 0)
                        SetSides(*e3->outrec, *edge_o, *e3);
                    else
                        SetSides(*e3->outrec, *e3, *edge_o);
                    return e3->outrec->pts;
                }
                else
                    return StartOpenPath(*edge_o, pt);
            }
            else
                return StartOpenPath(*edge_o, pt);
        }
#endif

        //MANAGING CLOSED PATHS FROM HERE ON

        //UPDATE WINDING COUNTS...

        int old_e1_windcnt, old_e2_windcnt;
        if (e1->local_min->polytype == e2->local_min->polytype)
        {
            if (fillrule_ == fill_rule::even_odd)
            {
                old_e1_windcnt = e1->wind_cnt;
                e1->wind_cnt = e2->wind_cnt;
                e2->wind_cnt = old_e1_windcnt;
            }
            else
            {
                if (e1->wind_cnt + e2->wind_dx == 0)
                    e1->wind_cnt = -e1->wind_cnt;
                else
                    e1->wind_cnt += e2->wind_dx;
                if (e2->wind_cnt - e1->wind_dx == 0)
                    e2->wind_cnt = -e2->wind_cnt;
                else
                    e2->wind_cnt -= e1->wind_dx;
            }
        }
        else
        {
            if (fillrule_ != fill_rule::even_odd)
            {
                e1->wind_cnt2 += e2->wind_dx;
                e2->wind_cnt2 -= e1->wind_dx;
            }
            else
            {
                e1->wind_cnt2 = (e1->wind_cnt2 == 0 ? 1 : 0);
                e2->wind_cnt2 = (e2->wind_cnt2 == 0 ? 1 : 0);
            }
        }

        switch (fillrule_)
        {
        case fill_rule::even_odd:
        case fill_rule::non_zero:
            old_e1_windcnt = abs(e1->wind_cnt);
            old_e2_windcnt = abs(e2->wind_cnt);
            break;
        default:
            if (fillrule_ == fillpos)
            {
                old_e1_windcnt = e1->wind_cnt;
                old_e2_windcnt = e2->wind_cnt;
            }
            else
            {
                old_e1_windcnt = -e1->wind_cnt;
                old_e2_windcnt = -e2->wind_cnt;
            }
            break;
        }

        const bool e1_windcnt_in_01 = old_e1_windcnt == 0 || old_e1_windcnt == 1;
        const bool e2_windcnt_in_01 = old_e2_windcnt == 0 || old_e2_windcnt == 1;

        if ((!is_hot_edge(e1) && !e1_windcnt_in_01) || (!is_hot_edge(e2) && !e2_windcnt_in_01))
        {
            return nullptr;
        }

        //NOW PROCESS THE INTERSECTION ...
        out_pt* resultOp = nullptr;
        //if both edges are 'hot' ...
        if (is_hot_edge(e1) && is_hot_edge(e2))
        {
            if ((old_e1_windcnt != 0 && old_e1_windcnt != 1) || (old_e2_windcnt != 0 && old_e2_windcnt != 1) ||
                (e1->local_min->polytype != e2->local_min->polytype && cliptype_ != clip_type::Xor))
            {
                resultOp = add_local_max_poly(e1, e2, pt);
#ifdef USINGZ
                if (zCallback_ && resultOp) SetZ(e1, e2, resultOp->pt);
#endif
            }
            else if (is_front(e1) || (e1->out_poly == e2->out_poly))
            {
                //this 'else if' condition isn't strictly needed but
                //it's sensible to split polygons that ony touch at
                //a common vertex (not at common edges).

                resultOp = add_local_max_poly(e1, e2, pt);
                out_pt* op2 = add_local_min_poly(e1, e2, pt);
#ifdef USINGZ
                if (zCallback_ && resultOp) SetZ(e1, e2, resultOp->pt);
                if (zCallback_) SetZ(e1, e2, op2->pt);
#endif
                if (resultOp && resultOp->pt == op2->pt &&
                    !is_horizontal(e1) && !is_horizontal(e2) &&
                    (cross_product(e1->bot, resultOp->pt, e2->bot) == 0))
                    add_join(resultOp, op2);
            }
            else
            {
                resultOp = add_out_pt(e1, pt);
#ifdef USINGZ
                OutPt* op2 = add_out_pt(e2, pt);
                if (zCallback_)
                {
                    SetZ(e1, e2, resultOp->pt);
                    SetZ(e1, e2, op2->pt);
                }
#else
                add_out_pt(e2, pt);
#endif
                SwapOutrecs(e1, e2);
            }
        }
        else if (is_hot_edge(e1))
        {
            resultOp = add_out_pt(e1, pt);
#ifdef USINGZ
            if (zCallback_) SetZ(e1, e2, resultOp->pt);
#endif
            SwapOutrecs(e1, e2);
        }
        else if (is_hot_edge(e2))
        {
            resultOp = add_out_pt(e2, pt);
#ifdef USINGZ
            if (zCallback_) SetZ(e1, e2, resultOp->pt);
#endif
            SwapOutrecs(e1, e2);
        }
        else
        {
            int64_t e1Wc2, e2Wc2;
            switch (fillrule_)
            {
            case fill_rule::even_odd:
            case fill_rule::non_zero:
                e1Wc2 = abs(e1->wind_cnt2);
                e2Wc2 = abs(e2->wind_cnt2);
                break;
            default:
                if (fillrule_ == fillpos)
                {
                    e1Wc2 = e1->wind_cnt2;
                    e2Wc2 = e2->wind_cnt2;
                }
                else
                {
                    e1Wc2 = -e1->wind_cnt2;
                    e2Wc2 = -e2->wind_cnt2;
                }
                break;
            }

            if (!IsSamePolyType(e1, e2))
            {
                resultOp = add_local_min_poly(e1, e2, pt, false);
#ifdef USINGZ
                if (zCallback_) SetZ(e1, e2, resultOp->pt);
#endif
            }
            else if (old_e1_windcnt == 1 && old_e2_windcnt == 1)
            {
                resultOp = nullptr;
                switch (cliptype_)
                {
                case clip_type::Union:
                    if (e1Wc2 <= 0 && e2Wc2 <= 0)
                        resultOp = add_local_min_poly(e1, e2, pt, false);
                    break;
                case clip_type::difference:
                    if (((get_poly_type(e1) == PathType::Clip) && (e1Wc2 > 0) && (e2Wc2 > 0)) ||
                        ((get_poly_type(e1) == PathType::Subject) && (e1Wc2 <= 0) && (e2Wc2 <= 0)))
                    {
                        resultOp = add_local_min_poly(e1, e2, pt, false);
                    }
                    break;
                case clip_type::Xor:
                    resultOp = add_local_min_poly(e1, e2, pt, false);
                    break;
                default:
                    if (e1Wc2 > 0 && e2Wc2 > 0)
                        resultOp = add_local_min_poly(e1, e2, pt, false);
                    break;
                }
#ifdef USINGZ
                if (resultOp && zCallback_) SetZ(e1, e2, resultOp->pt);
#endif
            }
        }
        return resultOp;
    }

    void clipper::push_horz(edge* e)
    {
        e->next_in_sel = (sel_first ? sel_first : nullptr);
        sel_first = e;
    }

    bool clipper::pop_horz(edge** e)
    {
        *e = sel_first;
        if (!(*e)) return false;
        sel_first = sel_first->next_in_sel;
        return true;
    }

    void clipper::do_horizontal(edge* horz)
    /*******************************************************************************
    * Notes: Horizontal edges (HEs) at scanline intersections (ie at the top or    *
    * bottom of a scanbeam) are processed as if layered.The order in which HEs     *
    * are processed doesn't matter. HEs intersect with the bottom vertices of      *
    * other HEs[#] and with non-horizontal edges [*]. Once these intersections     *
    * are completed, intermediate HEs are 'promoted' to the next edge in their     *
    * bounds, and they in turn may be intersected[%] by other HEs.                 *
    *                                                                              *
    * eg: 3 horizontals at a scanline:    /   |                     /           /  *
    *              |                     /    |     (HE3)o ========%========== o   *
    *              o ======= o(HE2)     /     |         /         /                *
    *          o ============#=========*======*========#=========o (HE1)           *
    *         /              |        /       |       /                            *
    *******************************************************************************/
    {
#if 0
        Point pt;
        bool horzIsOpen = is_open(horz);
        auto y = horz->bot.y;
        vertex* vertex_max = nullptr;
        edge* max_pair = nullptr;

        if (!horzIsOpen)
        {
            vertex_max = get_currY_maxima_vertex(horz);
            if (vertex_max)
            {
                max_pair = get_horz_maxima_pair(horz, vertex_max);
                //remove 180 deg.spikes and also simplify
                //consecutive horizontals when PreserveCollinear = true
                if (vertex_max != horz->vertex_top)
                    TrimHorz(horz, PreserveCollinear);
            }
        }

        num horz_left, horz_right;
        bool is_left_to_right =
            reset_horz_direction(horz, max_pair, horz_left, horz_right);

        if (is_hot_edge(horz))
            add_out_pt(horz, Point(horz->curr_x, y));

        out_pt* op;
        while (true) // loop through consec. horizontal edges
        {
            if (horzIsOpen && is_maxima(horz) && !is_open_end(horz))
            {
                vertex_max = get_currY_maxima_vertex(horz);
                if (vertex_max)
                    max_pair = get_horz_maxima_pair(horz, vertex_max);
            }

            edge* e;
            if (is_left_to_right) e = horz->next_in_ael;
            else e = horz->prev_in_ael;

            while (e)
            {

                if (e == max_pair)
                {
                    if (is_hot_edge(horz))
                    {
                        while (horz->vertex_top != e->vertex_top)
                        {
                            add_out_pt(horz, horz->top);
                            update_edge_into_ael(horz);
                        }
                        op = add_local_max_poly(horz, e, horz->top);
                        if (op && !is_open(horz) && op->pt == horz->top)
                            AddTrialHorzJoin(op);
                    }
                    DeleteFromAEL(*e);
                    DeleteFromAEL(horz);
                    return;
                }

                //if horzEdge is a maxima, keep going until we reach
                //its maxima pair, otherwise check for break conditions
                if (vertex_max != horz->vertex_top || is_open_end(horz))
                {
                    //otherwise stop when 'ae' is beyond the end of the horizontal line
                    if ((is_left_to_right && e->curr_x > horz_right) ||
                        (!is_left_to_right && e->curr_x < horz_left)) break;

                    if (e->curr_x == horz->top.x && !is_horizontal(e))
                    {
                        pt = next_vertex(horz)->pt;
                        if (is_left_to_right)
                        {
                            //with open paths we'll only break once past horz's end
                            if (is_open(*e) && !IsSamePolyType(*e, horz) && !is_hot_edge(e))
                            {
                                if (TopX(*e, pt.y) > pt.x) break;
                            }
                            //otherwise we'll only break when horz's outslope is greater than e's
                            else if (TopX(*e, pt.y) >= pt.x) break;
                        }
                        else
                        {
                            if (is_open(*e) && !IsSamePolyType(*e, horz) && !is_hot_edge(e))
                            {
                                if (TopX(*e, pt.y) < pt.x) break;
                            }
                            else if (TopX(*e, pt.y) <= pt.x) break;
                        }
                    }
                }

                pt = Point(e->curr_x, horz->bot.y);

                if (is_left_to_right)
                {
                    op = IntersectEdges(horz, *e, pt);
                    SwapPositionsInAEL(horz, *e);
                    // todo: check if op->pt == pt test is still needed
                    // expect op != pt only after add_local_max_poly when horz->outrec == nullptr
                    if (is_hot_edge(horz) && op && !is_open(horz) && op->pt == pt)
                        AddTrialHorzJoin(op);

                    if (!is_horizontal(e) && TestJoinWithPrev1(*e))
                    {
                        op = add_out_pt(*e->prev_in_ael, pt);
                        out_pt* op2 = add_out_pt(*e, pt);
                        add_join(op, op2);
                    }

                    horz->curr_x = e->curr_x;
                    e = horz->next_in_ael;
                }
                else
                {
                    op = IntersectEdges(*e, horz, pt);
                    SwapPositionsInAEL(*e, horz);

                    if (is_hot_edge(horz) && op &&
                        !is_open(horz) && op->pt == pt)
                        AddTrialHorzJoin(op);

                    if (!is_horizontal(e) && TestJoinWithNext1(*e))
                    {
                        op = add_out_pt(*e, pt);
                        out_pt* op2 = add_out_pt(*e->next_in_ael, pt);
                        add_join(op, op2);
                    }

                    horz->curr_x = e->curr_x;
                    e = horz->prev_in_ael;
                }
            }

            //check if we've finished with (consecutive) horizontals ...
            if (horzIsOpen && is_open_end(horz)) // ie open at top
            {
                if (is_hot_edge(horz))
                {
                    add_out_pt(horz, horz->top);
                    if (is_front(horz))
                        horz->outrec->front_edge = nullptr;
                    else
                        horz->outrec->back_edge = nullptr;
                    horz->outrec = nullptr;
                }
                DeleteFromAEL(horz);
                return;
            }
            else if (next_vertex(horz)->pt.y != horz->top.y)
                break;

            //still more horizontals in bound to process ...
            if (is_hot_edge(horz))
                add_out_pt(horz, horz->top);
            update_edge_into_ael(horz);

            if (PreserveCollinear && !horzIsOpen && HorzIsSpike(horz))
                TrimHorz(horz, true);

            is_left_to_right =
                ResetHorzDirection(horz, max_pair, horz_left, horz_right);
        }

        if (is_hot_edge(horz))
        {
            op = add_out_pt(horz, horz->top);
            if (!is_open(horz))
                AddTrialHorzJoin(op);
        }
        else
            op = nullptr;

        if ((horzIsOpen && !is_open_end(horz)) ||
            (!horzIsOpen && vertex_max != horz->vertex_top))
        {
            update_edge_into_ael(horz); // this is the end of an intermediate horiz.
            if (is_open(horz)) return;

            if (is_left_to_right && TestJoinWithNext1(horz))
            {
                out_pt* op2 = add_out_pt(*horz->next_in_ael, horz->bot);
                add_join(op, op2);
            }
            else if (!is_left_to_right && TestJoinWithPrev1(horz))
            {
                out_pt* op2 = add_out_pt(*horz->prev_in_ael, horz->bot);
                add_join(op2, op);
            }
        }
        else if (is_hot_edge(horz))
            add_local_max_poly(horz, *max_pair, horz->top);
        else
        {
            DeleteFromAEL(*max_pair);
            DeleteFromAEL(horz);
        }
#endif
    }

    bool clipper::reset_horz_direction(const edge* horz, const edge* max_pair, num& horz_left, num& horz_right)
    {
        if (horz->bot.x == horz->top.x)
        {
            //the horizontal edge is going nowhere ...
            horz_left = horz->curr_x;
            horz_right = horz->curr_x;
            auto e = horz->next_in_ael;
            while (e && e != max_pair) e = e->next_in_ael;
            return e != nullptr;
        }
        else if (horz->curr_x < horz->top.x)
        {
            horz_left = horz->curr_x;
            horz_right = horz->top.x;
            return true;
        }
        else
        {
            horz_left = horz->top.x;
            horz_right = horz->curr_x;
            return false;  // right to left
        }
    }

    void clipper::update_edge_into_ael(edge* e)
    {
        e->bot = e->top;
        e->vertex_top = next_vertex(e);
        e->top = e->vertex_top->pt;
        e->curr_x = e->bot.x;
        set_direct(e);
        if (is_horizontal(*e)) return;
        insert_scanline(e->top.y);
        if (test_join_with_prev1(e))
        {
            out_pt* op1 = add_out_pt(e->prev_in_ael, e->bot);
            out_pt* op2 = add_out_pt(e, e->bot);
            add_join(op1, op2);
        }
    }

    void clipper::DoIntersections(const num y)
    {

    }

    void clipper::CleanCollinear(out_polygon* output)
    {
    }

    void clipper::JoinOutrecPaths(edge* dest, edge* src)
    {
        //join e2 out_poly path onto e1 out_poly path and then delete e2 out_poly path
        //pointers. (NB Only very rarely do the joining ends share the same coords.)
        out_pt* p1_st = dest->out_poly->pts;
        out_pt* p2_st = src->out_poly->pts;
        out_pt* p1_end = p1_st->next;
        out_pt* p2_end = p2_st->next;
        if (is_front(dest))
        {
            p2_end->prev = p1_st;
            p1_st->next = p2_end;
            p2_st->next = p1_end;
            p1_end->prev = p2_st;
            dest->out_poly->pts = p2_st;
            dest->out_poly->front_edge = src->out_poly->front_edge;
            if (dest->out_poly->front_edge)
                dest->out_poly->front_edge->out_poly = dest->out_poly;
        }
        else
        {
            p1_end->prev = p2_st;
            p2_st->next = p1_end;
            p1_st->next = p2_end;
            p2_end->prev = p1_st;
            dest->out_poly->back_edge = src->out_poly->back_edge;
            if (dest->out_poly->back_edge)
                dest->out_poly->back_edge->out_poly = dest->out_poly;
        }

        //an owner must have a lower idx otherwise
        //it can't be a valid owner
        if (src->out_poly->owner && src->out_poly->owner->idx < dest->out_poly->idx)
        {
            if (!dest->out_poly->owner || src->out_poly->owner->idx < dest->out_poly->owner->idx)
                dest->out_poly->owner = src->out_poly->owner;
        }

        //after joining, the src->OutRec must contains no vertices ...
        src->out_poly->front_edge = nullptr;
        src->out_poly->back_edge = nullptr;
        src->out_poly->pts = nullptr;
        src->out_poly->owner = dest->out_poly;

        if (is_open_end(dest))
        {
            src->out_poly->pts = dest->out_poly->pts;
            dest->out_poly->pts = nullptr;
        }

        //and e1 and e2 are maxima and are about to be dropped from the Actives list.
        dest->out_poly = nullptr;
        src->out_poly = nullptr;
    }

    void clipper::DoTopOfScanbeam(num y)
    {
        sel_first = nullptr;  // sel_ is reused to flag horizontals (see PushHorz below)
        edge* e = ael_first;
        while (e)
        {
            //nb: 'e' will never be horizontal here
            if (e->top.y == y)
            {
                e->curr_x = e->top.x;
                if (is_maxima(e))
                {
                    e = DoMaxima(e);  // TOP OF BOUND (MAXIMA)
                    continue;
                }
                else
                {
                    //INTERMEDIATE VERTEX ...
                    if (is_hot_edge(e)) add_out_pt(e, e->top);
                    update_edge_into_ael(e);
                    if (is_horizontal(e))
                        push_horz(e);  // horizontals are processed later
                }
            }
            else // i.e. not the top of the edge
                e->curr_x = top_x(e, y);

            e = e->next_in_ael;
        }
    }

    // e must is_maxima
    edge* clipper::DoMaxima(edge* e)
    {
        edge* next_e, * prev_e, * max_pair;
        prev_e = e->prev_in_ael;
        next_e = e->next_in_ael;
        if (is_open_end(e))
        {
            if (is_hot_edge(e)) add_out_pt(e, e->top);
            if (!is_horizontal(e))
            {
                if (is_hot_edge(e))
                {
                    if (is_front(e))
                        e->out_poly->front_edge = nullptr;
                    else
                        e->out_poly->back_edge = nullptr;
                    e->out_poly = nullptr;
                }
                DeleteFromAEL(e);
            }
            return next_e;
        }
        else
        {
            max_pair = get_maxima_pair(e);
            // maxima 必定是左右成对的，没找到说明和它连接的是平行的线
            // 因为水平的线会优先处理掉，因此返回 next_e 继续处理
            if (!max_pair) return next_e;  // eMaxPair is horizontal
        }

        //only non-horizontal maxima here.
        //process any edges between maxima pair ...
        // 在 maxima pair 之间的边会被跳过到下一轮处理
        while (next_e != max_pair)
        {
            intersect_edges(e, next_e, e->top);
            SwapPositionsInAEL(e, next_e);
            next_e = e->next_in_ael;
        }

        if (is_open(e))
        {
            if (is_hot_edge(e))
                add_local_max_poly(e, max_pair, e->top);
            DeleteFromAEL(max_pair);
            DeleteFromAEL(e);
            return (prev_e ? prev_e->next_in_ael : ael_first);
        }

        //here E.next_in_ael == ENext == EMaxPair ...
        if (is_hot_edge(e))
            add_local_max_poly(e, max_pair, e->top);

        // 当走到一个 maxima top vertex 时，说明这个 active edge 差不多该结束了
        DeleteFromAEL(e);
        DeleteFromAEL(max_pair);
        return (prev_e ? prev_e->next_in_ael : ael_first);
    }

    void clipper::DeleteFromAEL(edge* e)
    {
        edge* prev = e->prev_in_ael;
        edge* next = e->next_in_ael;
        if (!prev && !next && (e != ael_first)) return;  // already deleted
        if (prev)
            prev->next_in_ael = next;
        else
            ael_first = next;
        if (next) next->prev_in_ael = prev;
        delete e;
    }

    //preconditon: e1 must be immediately to the left of e2
    void clipper::SwapPositionsInAEL(edge* e1, edge* e2)
    {
        edge* next = e2->next_in_ael;
        if (next) next->prev_in_ael = e1;
        edge* prev = e1->prev_in_ael;
        if (prev) prev->next_in_ael = e2;
        e2->prev_in_ael = prev;
        e2->next_in_ael = e1;
        e1->prev_in_ael = e2;
        e1->next_in_ael = next;
        if (!e2->prev_in_ael) ael_first = e2;
    }

    // e1,e2 必须是成对的 maxima edge，或者是 intersect 处理中视为成对的 active edge
    out_pt* clipper::add_local_max_poly(edge* e1, edge* e2, const Point& pt)
    {
        if (is_front(e1) == is_front(e2))
        {
            if (is_open_end(e1))
                //SwapFrontBackSides(*e1.outrec);
                ;
            else if (is_open_end(e2))
                //SwapFrontBackSides(*e2.outrec);
                ;
            else
            {
                // 正常情况下不该会有 maxima vertex 左右两边都是 front edge
                succeeded_ = false;
                return nullptr;
            }
        }

        auto result = add_out_pt(e1, pt);
        
        // 左右同一个输出，则合并
        if (e1->out_poly == e2->out_poly)
        {
            auto& outrec = *e1->out_poly;
            outrec.pts = result;

            UncoupleOutRec(e1);
            if (!is_open(e1)) CleanCollinear(&outrec);
            result = outrec.pts;
#if 0 // 暂时跳过生成 polytree
            if (using_polytree_ && outrec.owner && !outrec.owner->front_edge)
                outrec.owner = GetRealOutRec(outrec.owner->owner);
#endif
        }
        //and to preserve the winding orientation of outrec ...
        else if (is_open(e1))
        {
            if (e1->wind_dx < 0)
                JoinOutrecPaths(e1, e2);
            else
                JoinOutrecPaths(e2, e1);
        }
        // 左右是不同输出（多边形有相交），根据序号后面的合并到前面
        else if (e1->out_poly->idx < e2->out_poly->idx)
            JoinOutrecPaths(e1, e2);
        else
            JoinOutrecPaths(e2, e1);

        return result;
    }

    out_pt* clipper::add_out_pt(const edge* e, const Point& pt, out_seg* seg_data)
    {
        out_pt* new_op = nullptr;

        //Outrec.OutPts: a circular doubly-linked-list of POutPt where ...
        //op_front[.Prev]* ~~~> op_back & op_back == op_front.Next
        out_polygon* outpoly = e->out_poly;
        bool to_front = is_front(e);
        out_pt* op_front = outpoly->pts;
        out_pt* op_back = op_front->next;

        // 跳过相同的点
        if (to_front)
        {
            if (pt == op_front->pt)
                return op_front;
        }
        else if (pt == op_back->pt)
            return op_back;

        /*        new_op
        *           |
        *           V
        * op_front --> op_back     */
        new_op = new out_pt(pt, outpoly);
        new_op->next_data = seg_data;
        op_back->prev = new_op;
        new_op->prev = op_front;
        new_op->next = op_back;
        op_front->next = new_op;
        if (to_front) outpoly->pts = new_op;
        return new_op;
    }

    out_pt* clipper::add_local_min_poly(edge* e1, edge* e2, const Point& pt, bool is_new)
    {
        out_polygon* outrec = new out_polygon();
        outrec->idx = (unsigned)outrec_list_.size();
        outrec_list_.push_back(outrec);
        outrec->pts = nullptr;
        //outrec->polypath = nullptr;
        e1->out_poly = outrec;
        e2->out_poly = outrec;

        //Setting the owner and inner/outer states (above) is an essential
        //precursor to setting edge 'sides' (ie left and right sides of output
        //polygons) and hence the orientation of output paths ...

        if (is_open(e1))
        {
            outrec->owner = nullptr;
            outrec->is_open = true;
            if (e1->wind_dx > 0)
                set_sides(outrec, e1, e2);
            else
                set_sides(outrec, e2, e1);
        }
        else
        {
            edge* prevHotEdge = get_prev_hot_edge(e1);
            //e.windDx is the winding direction of the **input** paths
            //and unrelated to the winding direction of output polygons.
            //Output orientation is determined by e.outrec.frontE which is
            //the ascending edge (see add_local_min_poly).
            if (prevHotEdge)
            {
                outrec->owner = prevHotEdge->out_poly;
                if (outpoly_is_ascending(prevHotEdge) == is_new)
                    set_sides(outrec, e2, e1);
                else
                    set_sides(outrec, e1, e2);
            }
            else
            {
                outrec->owner = nullptr;
                if (is_new)
                    set_sides(outrec, e1, e2);
                else
                    set_sides(outrec, e2, e1);
            }
        }

        out_pt* op = new out_pt(pt, outrec);
        outrec->pts = op;
        return op;
    }

    void clipper::insert_right_edge(edge* left, edge* right)
    {
        right->next_in_ael = left->next_in_ael;
        if (left->next_in_ael) left->next_in_ael->prev_in_ael = right;
        right->prev_in_ael = left;
        left->next_in_ael = right;
    }

    // 关键部分之一，只能传入 ael 中的 edge
    // edge 的 windcount 指的是 edge 左右两边的区域的环绕数，两个相邻区域的环绕数最多差1
    void clipper::set_windcount_closed(edge* e)
    {
        edge* e2 = e->prev_in_ael;
        //find the nearest closed path edge of the same PolyType in AEL (heading left)
        PathType pt = get_poly_type(e);
        while (e2 && (get_poly_type(e2) != pt || is_open(e2))) e2 = e2->prev_in_ael;

        if (!e2)
        {
            // 最左为空则取决于自身方向
            e->wind_cnt = e->wind_dx;
            e2 = ael_first;
        }
        else if (fillrule_ == fill_rule::even_odd)
        {
            // 奇偶原则使得相邻区域里外相反，间隔区域例外相同
            e->wind_cnt = e->wind_dx;
            e->wind_cnt2 = e2->wind_cnt2;
            e2 = e2->next_in_ael;
        }
        else
        {
            //NonZero, positive, or negative filling here ...
            //if e's WindCnt is in the SAME direction as its WindDx, then polygon
            //filling will be on the right of 'e'.
            //NB neither e2.WindCnt nor e2.WindDx should ever be 0.
            if (e2->wind_cnt * e2->wind_dx < 0)
            {
                // 左边的边朝向和环绕数反向的话

                //opposite directions so 'e' is outside 'e2' ...
                if (abs(e2->wind_cnt) > 1)
                {
                    //outside prev poly but still inside another.
                    if (e2->wind_dx * e->wind_dx < 0)
                        //reversing direction so use the same WC
                        e->wind_cnt = e2->wind_cnt;
                    else
                        //otherwise keep 'reducing' the WC by 1 (ie towards 0) ...
                        e->wind_cnt = e2->wind_cnt + e->wind_dx;
                }
                else
                    //now outside all polys of same polytype so set own WC ...
                    e->wind_cnt = (is_open(e) ? 1 : e->wind_dx);
            }
            else
            {
                // 左边的边朝向和环绕数同向的话

                //'e' must be inside 'e2'
                if (e2->wind_dx * e->wind_dx < 0)
                    // 朝向相反意味着他们的 wind_cnt 都指向同一个区域
                    //reversing direction so use the same WC
                    e->wind_cnt = e2->wind_cnt;
                else
                    //otherwise keep 'increasing' the WC by 1 (ie away from 0) ...
                    e->wind_cnt = e2->wind_cnt + e->wind_dx;
            }
            e->wind_cnt2 = e2->wind_cnt2;
            e2 = e2->next_in_ael;  // ie get ready to calc WindCnt2
        }

        //update wind_cnt2 ...
        // 往左循环，逐一增加相反类型的wind_dx
        if (fillrule_ == fill_rule::even_odd)
            while (e2 != e)
            {
                if (get_poly_type(e2) != pt && !is_open(e2))
                    e->wind_cnt2 = (e->wind_cnt2 == 0 ? 1 : 0);
                e2 = e2->next_in_ael;
            }
        else
            while (e2 != e)
            {
                if (get_poly_type(e2) != pt && !is_open(e2))
                    e->wind_cnt2 += e2->wind_dx;
                e2 = e2->next_in_ael;
            }
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
