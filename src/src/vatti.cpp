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

    // 切记要提前处理了等价水平的曲线
    inline bool is_horizontal(const edge& e)
    {
        return (e.top.y == e.bot.y);
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

    inline bool is_open(const edge* e) {
        return false;
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

    // ToDo 对于曲线内容行不通
    inline void set_direct(edge* e)
    {
        e->dx = get_direct(e->bot, e->top);
    }

    inline vertex* next_vertex(const edge* e)
    {
        if (e->wind_dx > 0)
            return e->vertex_top->next;
        else
            return e->vertex_top->prev;
    }

    inline vertex* prev_prev_vertex(const edge* ae)
    {
        if (ae->wind_dx > 0)
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

            // 水平线的放置位置规则是由将水平线当成略微倾斜的线来处理的方法延申出来的
            // ToDo 这里应该要改改，因为 Clipper 本身是自上到下的。
            if (is_horizontal(*left_bound))
            {
                if (horz_is_heading_right(*left_bound)) swap_edge(&left_bound, &right_bound);
            }
            else if (is_horizontal(*right_bound))
            {
                if (horz_is_heading_left(*right_bound)) swap_edge(&left_bound, &right_bound);
            }
            // ToDo 这里判断不足以应对曲线内容
            else if (left_bound->dx < right_bound->dx)
                swap_edge(&left_bound, &right_bound);

            // 开始处理 left_bound
            bool contributing;
            left_bound->is_left_bound = true;
            insert_left_edge(left_bound);

            set_windcount_closed(left_bound);
            contributing = is_contributing_closed(left_bound);

            // 开始处理 right_bound
            right_bound->is_left_bound = false;
            right_bound->wind_cnt = left_bound->wind_cnt;
            right_bound->wind_cnt2 = left_bound->wind_cnt2;
            insert_right_edge(left_bound, right_bound);


            if (contributing)
            {
                AddLocalMinPoly(*left_bound, *right_bound, left_bound->bot, true);
                if (!IsHorizontal(*left_bound) && TestJoinWithPrev1(*left_bound))
                {
                    OutPt* op = AddOutPt(*left_bound->prev_in_ael, left_bound->bot);
                    AddJoin(op, left_bound->outrec->pts);
                }
            }

            while (right_bound->next_in_ael &&
                IsValidAelOrder(*right_bound->next_in_ael, *right_bound))
            {
                IntersectEdges(*right_bound, *right_bound->next_in_ael, right_bound->bot);
                SwapPositionsInAEL(*right_bound, *right_bound->next_in_ael);
            }

            if (!IsHorizontal(*right_bound) &&
                TestJoinWithNext1(*right_bound))
            {
                OutPt* op = AddOutPt(*right_bound->next_in_ael, right_bound->bot);
                AddJoin(right_bound->outrec->pts, op);
            }

            if (IsHorizontal(*right_bound))
                PushHorz(*right_bound);
            else
                InsertScanline(right_bound->top.y);
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
        
        // ToDo 首先尝试以cw排序，这里可能要改ccw排序
        // 这时候是相交于 newcomer->bot
        double d = cross_product(resident->top, newcomer->bot, newcomer->top);
        if (d != 0) return d < 0;

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
