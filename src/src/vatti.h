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

    struct out_seg {
        SegType type = SegType::MoveTo;
        void* data = nullptr;
    };

    struct out_pt {
        Point pt;
        out_pt* next = nullptr;
        out_pt* prev = nullptr;
        out_seg* next_data = nullptr;
        out_polygon* outrec;

        out_pt(const Point& pt_, out_polygon* outrec_) : pt(pt_), outrec(outrec_) {
            next = this;
            prev = this;
        }
    };

    struct cubic_data {
        Point ctrl1;
        Point ctrl2;
    };

    struct out_polygon
    {
        size_t idx = 0;
        out_polygon* owner = nullptr;

        edge* front_edge = nullptr;
        edge* back_edge = nullptr;

        edge* left_bound = nullptr;
        Point left_stop_pt;
        edge* right_bound = nullptr;
        Point right_stop_pt;


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
        int wind_dx_all = 1;
        int wind_cnt = 0;
        int wind_cnt2 = 0; //winding count of the opposite polytype

        out_polygon* out_poly = nullptr;

        edge* prev_in_ael = nullptr;
        edge* next_in_ael = nullptr;
        vertex* vertex_top = nullptr;
        local_minima* local_min = nullptr;

        bool is_left_bound = false;
        bool is_same_with_prev = false;
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
        void add_local_min(vertex* vert, PathType pt, bool is_open);
        void reset();
        void insert_scanline(num y);
        bool pop_scanline(num& y);
        bool pop_local_minima(num y, local_minima** out);
        void insert_local_minima_to_ael(num y);
        void recalc_windcnt();
        edge* calc_windcnt(edge* e);
        void insert_into_ael(edge* newcomer);
        void insert_into_ael(edge* left, edge* newcomer);
        bool is_valid_ael_order(const edge* resident, const edge* newcomer) const;
        void push_windcnt_change(num x);
        void set_windcount_closed(edge* e);
        void do_top_of_scanbeam(num y);

        clip_type cliptype_ = clip_type::intersection;
        fill_rule fillrule_ = fill_rule::positive;
        fill_rule fillpos = fill_rule::positive;

        // 使用 object_pool 提高内存申请的效率和放置内存泄露
        boost::object_pool<vertex> vertex_pool;
        std::vector<vertex*> paths_start;
        std::vector<break_info> break_info_list;
        std::vector<local_minima*> local_minima_list;
        std::vector<local_minima*>::iterator cur_locmin_it;
        std::priority_queue<num,std::vector<num>,std::greater<num>> scanline_list;
        edge* ael_first = nullptr;
        std::vector<num> windcnt_change_list;

        //boost::pool<out_seg> seg_pool;
        //boost::pool<double> seg_data_pool;

        bool succeeded_ = true;

    };
}

