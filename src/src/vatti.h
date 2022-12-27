#pragma once
#include "river.h"
#include <queue>
#include "boost/pool/pool.hpp"
#include "boost/pool/object_pool.hpp"
namespace vatti
{
    using namespace river;

    typedef double num;

    enum class vertex_flags : uint32_t {
        none = 0, 
        open_start = 0b1 << 0,
        open_end   = 0b1 << 1, 
        local_max  = 0b1 << 2, 
        local_min  = 0b1 << 3
    };

    constexpr enum vertex_flags operator &(enum vertex_flags a, enum vertex_flags b)
    {
        return (enum vertex_flags)(uint32_t(a) & uint32_t(b));
    }

    constexpr enum vertex_flags operator |(enum vertex_flags a, enum vertex_flags b)
    {
        return (enum vertex_flags)(uint32_t(a) | uint32_t(b));
    }

    struct edge;
    struct out_pt;
    struct out_polygon;
    struct joiner;

    struct vertex
    {
        Point pt;

        // if segment == nullptr, it is line point;
        Seg* next_seg;
        vertex* next;
        vertex* prev;
        vertex_flags flags = vertex_flags::none;

        ~vertex() {
            if (next_seg) delete next_seg;
        }
    };

    struct local_minima
    {
        vertex* vert;
        PathType polytype;
        bool is_open;

        local_minima(vertex* v, PathType pt, bool open) :
            vert(v), polytype(pt), is_open(open){}
    };

    struct break_info
    {
        vertex* vert;
        Point break_point;
        num t;

        bool operator==(const break_info& r) const {
            return vert == r.vert
                && break_point == r.break_point
                && t == r.t;
        }
    };

    struct out_pt {
        Point pt;
        out_pt* next = nullptr;
        out_pt* prev = nullptr;
        out_polygon* outrec;
        joiner* joiner = nullptr;

        out_pt(const Point& pt_, out_polygon* outrec_) : pt(pt_), outrec(outrec_) {
            next = this;
            prev = this;
        }
    };

    struct out_polygon
    {
        size_t idx = 0;
        out_polygon* owner = nullptr;

        edge* front_edge = nullptr;
        edge* back_edge = nullptr;
        out_pt* pts = nullptr;

        bool is_open = false;
    };

    struct edge
    {
        Point bot;
        Point top;
        num curr_x = 0;
        // ToDo 单 double 不足以表示曲线内容走向和斜率
        double dx = 0.;
        int wind_dx = 1; //1 or -1 depending on winding direction
        int wind_cnt = 0;
        int wind_cnt2 = 0;		//winding count of the opposite polytype
        out_polygon* out_poly = nullptr;

        edge* prev_in_ael = nullptr;
        edge* next_in_ael = nullptr;
        //SEL: 'sorted edge list' (Vatti's ST - sorted table)
        //     linked list used when sorting edges into their new positions at the
        //     top of scanbeams, but also (re)used to process horizontals.
        edge* prev_in_sel = nullptr;
        edge* next_in_sel = nullptr;
        edge* jump = nullptr;
        vertex* vertex_top = nullptr;
        local_minima* local_min = nullptr;

        bool is_left_bound = false;
    };

    struct joiner
    {
        int      idx;
        out_pt* op1;
        out_pt* op2;
        joiner* next1;
        joiner* next2;
        joiner* nextH;

        explicit joiner(out_pt* op1_, out_pt* op2_, joiner* nexth) :
            op1(op1_), op2(op2_), nextH(nexth)
        {
            idx = -1;
            next1 = op1->joiner;
            op1->joiner = this;

            if (op2)
            {
                next2 = op2->joiner;
                op2->joiner = this;
            }
            else
                next2 = nullptr;
        }
    };

    class clipper
    {
    public:
        void add_path(const Paths& paths, PathType polytype, bool is_open);

        void process();

        void intersect(vertex* const & r, vertex* const& l);
        void process_intersect();
    private:
        vertex* new_vertex();
        void set_segment(vertex* prev, vertex* mem, Seg* move_seg);
        void add_local_min(vertex* vert, PathType, bool is_open);
        void insert_scanline(num y);
        bool pop_scanline(num& y);
        void insert_local_minima_to_ael(num y);
        out_pt* intersect_edges(edge* e1, edge* e2, const Point& pt);
        bool pop_local_minima(num y, local_minima**);
        void insert_left_edge(edge* e);
        bool is_valid_ael_order(const edge* resident, const edge* newcomer);
        void set_windcount_closed(edge* e);
        bool is_contributing_closed(edge* e);
        void insert_right_edge(edge* left, edge* right);
        out_pt* add_local_min_poly(edge* e1, edge* e2, const Point& pt, bool is_new = false);
        out_pt* add_local_max_poly(edge* e1, edge* e2, const Point& pt);
        out_pt* add_out_pt(const edge* e, const Point& pt);
        void add_join(out_pt* op1, out_pt* op2);
        void push_horz(edge* e);
        bool pop_horz(edge** e);
        void do_horizontal(edge* e);
        void DoIntersections(const num y);
        bool reset_horz_direction(const edge* horz, const edge* max_pair,
            num& horz_left, num& horz_right);
        void update_edge_into_ael(edge* e);
        void CleanCollinear(out_polygon* output);
        void JoinOutrecPaths(edge* dest, edge* src);
        void DoTopOfScanbeam(num y);
        edge* DoMaxima(edge* e);
        void SwapPositionsInAEL(edge* e1, edge* e2);
        void DeleteFromAEL(edge* e);

        clip_type cliptype_ = clip_type::intersection;
        fill_rule fillrule_ = fill_rule::positive;
        fill_rule fillpos = fill_rule::positive;

        // 使用 object_pool 提高内存申请的效率和放置内存泄露
        boost::object_pool<vertex> vertex_pool;
        std::vector<vertex*> paths_start;
        std::vector<local_minima*> local_minima_list;
        std::vector<local_minima*>::iterator cur_locmin_it;
        // 不知道为啥 Clipper 要特意使用 priority_queue，先模仿下
        std::priority_queue<num,std::vector<num>,std::greater<num>> scanline_list;
        edge* ael_first = nullptr;
        edge* sel_first = nullptr;
        std::vector<out_polygon*> outrec_list_;
        std::vector<joiner*> joiner_list_;
        bool PreserveCollinear = false;
        bool succeeded_ = true;
        num bot_y_;

        std::vector<break_info> break_info_list;
    };
}

