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

    inline PathType get_polytype(const edge* e) {
        return e->local_min->polytype;
    }

    inline bool is_open(const edge* e) { return false; }

    void clipper::add_local_min(vertex* vert, PathType pt, bool is_open)
    {
        //make sure the vertex is added only once ...
        if ((vertex_flags::local_min & vert->flags) != vertex_flags::none) return;

        vert->flags = (vert->flags | vertex_flags::local_min);
        local_minima_list.push_back(new local_minima(vert, pt, is_open));
    }

    /*         0
             pi/2
               |       
     -inf pi---O---0  +inf*/
    num get_direction(const Point& top, const Point& bot) {
        double dy = double(top.y - bot.y);
        if (dy != 0)
            return (top.x - bot.x) / dy;
        else if (top.x > bot.x)
            return std::numeric_limits<num>::max();
        else
            return -std::numeric_limits<num>::max();
    }

    void set_direction(edge* e) {
        e->dx = get_direction(e->top, e->bot);
    }

    void swap_edge(edge** l, edge** r) {
        edge* tmp = *l;
        *l = *r;
        *r = tmp;
    }

    void clipper::reset()
    {
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
        reset();

        num y;
        if (!pop_scanline(y)) return;

        while (true)
        {
            insert_local_minima_to_ael(y);
            //edge* e;
            //while (pop_horz(&e)) do_horizontal(e);
            recalc_windcnt();
            update_ouput_bound();
            if (!pop_scanline(y)) break;  // y new top of scanbeam
            do_top_of_scanbeam(y);
            close_output();
            //while (pop_horz(&e)) do_horizontal(e);
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
        local_minima* local_minima;
        edge* left_bound, * right_bound;

        while (pop_local_minima(y, &local_minima))
        {
            left_bound = new edge();
            left_bound->bot = local_minima->vert->pt;
            left_bound->curr_x = left_bound->bot.x;
            left_bound->wind_dx = -1;
            left_bound->wind_dx_all = left_bound->wind_dx;
            left_bound->vertex_top = local_minima->vert->prev;  // ie descending
            left_bound->top = left_bound->vertex_top->pt;
            left_bound->local_min = local_minima;
            set_direction(left_bound);

            right_bound = new edge();
            right_bound->bot = local_minima->vert->pt;
            right_bound->curr_x = right_bound->bot.x;
            right_bound->wind_dx = 1;
            right_bound->wind_dx_all = right_bound->wind_dx;
            right_bound->vertex_top = local_minima->vert->next;  // ie ascending
            right_bound->top = right_bound->vertex_top->pt;
            right_bound->local_min = local_minima;
            set_direction(right_bound);

            if (left_bound->dx > right_bound->dx) {
                swap_edge(&left_bound, &right_bound);
            }

            left_bound->is_left_bound = true;
            right_bound->is_left_bound = false;

            insert_into_ael(left_bound);
            insert_into_ael(left_bound, right_bound);

            push_windcnt_change(left_bound->bot.x);
        }
    }

    void clipper::recalc_windcnt()
    {
        std::sort(windcnt_change_list.begin(), windcnt_change_list.end());
        windcnt_change_list.erase(std::unique(windcnt_change_list.begin(), windcnt_change_list.end()), windcnt_change_list.end());

        auto curr_e = ael_first;
        for (auto x : windcnt_change_list)
        {
            while (curr_e) {
                if (curr_e->curr_x != x) {
                    curr_e = curr_e->next_in_ael;
                    continue;
                }

                 curr_e = calc_windcnt(curr_e);
            }
        }
    }

    edge* clipper::calc_windcnt(edge* curr_e)
    {
        edge* left = curr_e->prev_in_ael;
        PathType pt = get_polytype(curr_e);
        while (left && (get_polytype(left) != pt || is_open(left))) left = left->prev_in_ael;

        // 获取所有 curr_e 后面 is_same_with_prev 的 wind_dx 的集合
        int wind_dx = curr_e->wind_dx;
        {
            edge* next = curr_e->next_in_ael;
            while (next && next->is_same_with_prev && get_polytype(next) == pt) {
                wind_dx += next->wind_dx;
                next = next->next_in_ael;
            }
            curr_e->wind_dx_all = wind_dx;
        }

        if (!left) {
            curr_e->wind_cnt = wind_dx;
            left = ael_first;
        }
        else if (fillrule_ == fill_rule::even_odd) {
            curr_e->wind_cnt = curr_e->wind_dx_all;
            curr_e->wind_cnt2 = left->wind_cnt2;
            left = left->next_in_ael;
        }
        else {
            //NonZero, positive, or negative filling here ...
            //if e's WindCnt is in the SAME direction as its WindDx, then polygon
            //filling will be on the right of 'e'.
            //NB neither e2.WindCnt nor e2.WindDx should ever be 0.
            if (left->wind_cnt * left->wind_dx_all < 0)
            {
                //opposite directions so 'e' is outside 'left' ...
                if (abs(left->wind_cnt) > 1)
                {
                    //outside prev poly but still inside another.
                    if (left->wind_dx_all * curr_e->wind_dx_all < 0)
                        //reversing direction so use the same WC
                        curr_e->wind_cnt = left->wind_cnt;
                    else
                        //otherwise keep 'reducing' the WC by 1 (ie towards 0) ...
                        curr_e->wind_cnt = left->wind_cnt + curr_e->wind_dx_all;
                }
                else
                    //now outside all polys of same polytype so set own WC ...
                    curr_e->wind_cnt = (is_open(curr_e) ? 1 : curr_e->wind_dx_all);
            }
            else
            {
                //'e' must be inside 'left'
                if (left->wind_dx_all * curr_e->wind_dx_all < 0)
                    //reversing direction so use the same WC
                    curr_e->wind_cnt = left->wind_cnt;
                else
                    //otherwise keep 'increasing' the WC by 1 (ie away from 0) ...
                    curr_e->wind_cnt = left->wind_cnt + curr_e->wind_dx_all;
            }
            curr_e->wind_cnt2 = left->wind_cnt2;
            left = left->next_in_ael;  // ie get ready to calc WindCnt2
        }

        //update wind_cnt2 ...
        // ToDo 这里还要计算不同 path_type 重合的边
        if (fillrule_ == fill_rule::even_odd)
            while (left != curr_e)
            {
                if (get_polytype(left) != pt && !is_open(left))
                    curr_e->wind_cnt2 = (curr_e->wind_cnt2 == 0 ? 1 : 0);
                left = left->next_in_ael;
            }
        else
            while (left != curr_e)
            {
                if (get_polytype(left) != pt && !is_open(left))
                    curr_e->wind_cnt2 += left->wind_dx_all;
                left = left->next_in_ael;
            }

        {
            edge* next = curr_e->next_in_ael;
            while (next && next->is_same_with_prev && get_polytype(next) == pt) {
                {
                    next->wind_dx_all = curr_e->wind_dx_all;
                    next->wind_cnt = curr_e->wind_cnt;
                    next->wind_cnt2 = curr_e->wind_cnt2;
                }
            }
            curr_e->wind_dx_all = wind_dx;
        }

        curr_e = curr_e->next_in_ael;
        if (curr_e && curr_e->is_same_with_prev) curr_e = curr_e->next_in_ael;

        return curr_e;
    }

    void clipper::update_ouput_bound()
    {
        if (windcnt_change_list.empty()) return;
        windcnt_change_list.clear();

        std::vector<edge*> bound_edge;
        auto e = ael_first;
        while (e) {
            bool contributing = is_contributing(e);

            if (contributing) {
                bound_edge.push_back(e);
            }

            e = e->next_in_ael;
            while (e && e->is_same_with_prev) {
                e = e->next_in_ael;
            }         
        }

        // ToDo 需要注意已经在 obl 中的 bound_edge

        // 下部早已合并，现在来合并上部可以合并的


        // 新的输出轮廓和旧的相连


        //struct union_data {
        //    out_bound* bound = nullptr;
        //    edge* bound_edge = nullptr;
        //};

        // 此时必定没有可以相连接的 out_bound, 也没有可以相连接组成闭合区域的 bound_edge
        // 只剩下可以上下相连的 out_bound 和 edge，按顺序两两相连即可
        if(obl_first)
        {
            size_t upi = 0;
            auto down_l = obl_first;
            auto down_l = obl_first->next_in_obl;
            {
                auto up_l = bound_edge[upi];
                auto up_r = bound_edge[upi + 1];
                auto down_l = bound_edge[upi];
            }
        }

        // 解散旧的 obl，构造新的 obl
    }

    bool clipper::is_contributing(edge* e)
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

        case clip_type::union_:
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
            if (get_polytype(e) == PathType::Subject)
                return result;
            else
                return !result;
            break;

        case clip_type::xor_: return true;  break;
        }

        return false;
    }

    bool clipper::pop_local_minima(num y, local_minima** out)
    {
        if (cur_locmin_it == local_minima_list.end() || (*cur_locmin_it)->vert->pt.y != y) return false;
        *out = (*cur_locmin_it);
        ++cur_locmin_it;
        return true;
    }

    void clipper::push_windcnt_change(num x)
    {
        windcnt_change_list.push_back(x);
    }

    void clipper::insert_into_ael(edge* newcomer)
    {
        if (!ael_first) {
            // newcomer being ael_first
            ael_first = newcomer;
            newcomer->prev_in_ael = nullptr;
            newcomer->next_in_ael = nullptr;
        }
        else if (!is_valid_ael_order(ael_first, newcomer))
        {
            // swap ael_first、newcomer position in ael
            newcomer->prev_in_ael = nullptr;
            newcomer->next_in_ael = ael_first;
            ael_first->prev_in_ael = newcomer;
            ael_first->next_in_ael = nullptr;
        }
        else {
            // find last position can insert
            insert_into_ael(ael_first, newcomer);
        }
    }

    void clipper::insert_into_ael(edge* left, edge* newcomer)
    {
        if (left == nullptr) return insert_into_ael(newcomer);

        edge* resident = left;
        edge* next = resident->next_in_ael;
        while (next && is_valid_ael_order(next, newcomer))
        {
            resident = resident->next_in_ael;
            next = next->next_in_ael;
        }
        newcomer->next_in_ael = next;
        if (next) next->prev_in_ael = newcomer;
        newcomer->prev_in_ael = resident;
        resident->next_in_ael = newcomer;
    }

    bool clipper::is_valid_ael_order(const edge* resident, const edge* newcomer) const
    {
        if (newcomer->curr_x != resident->curr_x)
            return newcomer->curr_x > resident->curr_x;

        // 永远只有端点相交的情况，top 也永远位于 top 上方
        // 同一点排序，以点为原点，9点钟方向，顺时针排序
        // Subject 在 clip 前面
        // ToDo 还要考虑端点相交，但是是曲线的情况
        if (resident->dx != newcomer->dx)
            return resident->dx < newcomer->dx;
        return get_polytype(resident) < get_polytype(newcomer);
    }

    void clipper::insert_scanline(num y)
    {
        scanline_list.push(y);
    }

    void clipper::do_top_of_scanbeam(num y)
    {
        
    }

    void clipper::close_output()
    {
        constexpr auto try_merge_output_with_next = [](edge* curr) -> bool
        {
            auto next = curr->next_in_obl;
            if (next && next->curr_x == curr->curr_x) {
                // merge_output(curr, next);
                // remove_output_pair(curr, next);
                return true;
            }
            return false;
        };

        auto curr_out = obl_first;
        bool is_left = true;
        while (curr_out) {
            edge* next = curr_out->next_in_obl;
            edge* nextnext = next->next_in_obl;
            if (is_left && try_merge_output_with_next(curr_out)) {
                curr_out = nextnext;
                continue;
            }
            else if (next) {
                // try next first
                if (try_merge_output_with_next(next)) {
                    continue;
                }
            }
            is_left = !is_left;
            curr_out = curr_out->next_in_obl;
        }
    }

    // 关键部分之一，只能传入 ael 中的 edge
    // edge 的 windcount 指的是 edge 左右两边的区域的环绕数，两个相邻区域的环绕数最多差1
    void clipper::set_windcount_closed(edge* e)
    {
        edge* e2 = e->prev_in_ael;
        //find the nearest closed path edge of the same PolyType in AEL (heading left)
        PathType pt = get_polytype(e);
        while (e2 && (get_polytype(e2) != pt || is_open(e2))) e2 = e2->prev_in_ael;

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
                if (get_polytype(e2) != pt && !is_open(e2))
                    e->wind_cnt2 = (e->wind_cnt2 == 0 ? 1 : 0);
                e2 = e2->next_in_ael;
            }
        else
            while (e2 != e)
            {
                if (get_polytype(e2) != pt && !is_open(e2))
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
}
