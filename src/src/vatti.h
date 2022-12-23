#pragma once
#include "river.h"
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
        vertex* pt;
        PathType polytype;
        bool is_open;

        local_minima(vertex* v, PathType pt, bool open) :
            pt(v), polytype(pt), is_open(open){}
    };

    struct break_info
    {
        vertex* vert;
        Point break_point;
    };

    class clipper
    {
    public:
        void add_path(const Paths& paths, PathType polytype, bool is_open);

        void process();

        void intersect(vertex* const & r, vertex* const& l);
    private:
        vertex* new_vertex();
        void add_local_min(vertex* vert, PathType, bool is_open);

        // 使用 object_pool 提高内存申请的效率和放置内存泄露
        boost::object_pool<vertex> vertex_pool;
        std::vector<vertex*> paths_start;
        std::vector<local_minima*> local_minima_list;

        std::vector<break_info> break_info_list;
    };
}

