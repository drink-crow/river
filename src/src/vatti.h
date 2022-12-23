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
        none = 0, open_start = 0x1, open_end = 0b1<<1, local_max = 0b1 << 2, local_min = 0b1 << 3
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

    struct out_polygon
    {
        edge* front_edge = nullptr;
        edge* back_edge = nullptr;
    };

    struct edge
    {
        Point bot;
        Point top;
        num curr_x = 0;
        double dx = 0.;
        int wind_dx = 1; //1 or -1 depending on winding direction
        int wind_cnt = 0;
        int wind_cnt2 = 0;		//winding count of the opposite polytype
        out_polygon* out_poly;

        edge* prev_in_ael = nullptr;
        edge* next_in_ael = nullptr;
        edge* jump = nullptr;
        vertex* vertex_top = nullptr;
        local_minima* local_min = nullptr;

        bool is_left_bound = false;
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
        bool pop_scanline(num& y);
        void insert_local_minima_to_ael(num y);
        bool pop_local_minima(num y, local_minima**);

        // 使用 object_pool 提高内存申请的效率和放置内存泄露
        boost::object_pool<vertex> vertex_pool;
        std::vector<vertex*> paths_start;
        std::vector<local_minima*> local_minima_list;
        dceltype(local_minima_list)::iterator cur_locmin_it;
        // 不知道为啥 Clipper 要特意使用 priority_queue，先模仿下
        std::priority_queue<num> scanline_list;
        edge* ael_first = nullptr;

        std::vector<break_info> break_info_list;
    };
}

