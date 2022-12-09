#pragma once
#include "math.h"
#include "dcel.h"
#include <vector>

#include "boost/geometry.hpp"

namespace boost
{
    namespace geometry
    {
        namespace traits
        {
            

            template<>
            struct tag<::rmath::vec2> {
                typedef point_tag type;
            };

            template<>
            struct coordinate_type<::rmath::vec2>
            {
                typedef double type;
            };

            template<>
            struct coordinate_system<::rmath::vec2>
            {
                typedef cs::cartesian type;
            };

            template<>
            struct dimension<::rmath::vec2> : boost::mpl::int_<2> {};

            template<>
            struct access<::rmath::vec2, 0>
            {
                static inline double get(const ::rmath::vec2& p)
                {
                    return p.x;
                }
                static inline void set(::rmath::vec2& p, const double& value)
                {
                    p.x = value;
                }
            };

            template<>
            struct access<::rmath::vec2, 1>
            {
                static inline double get(const ::rmath::vec2& p)
                {
                    return p.y;
                }
                static inline void set(::rmath::vec2& p, const double& value)
                {
                    p.y = value;
                }
            };

            template<>
            struct tag<::rmath::rect>
            {
                typedef box_tag type;
            };
            template<>
            struct point_type<::rmath::rect>
            {
                typedef ::rmath::vec2 type;
            };

            template<size_t D>
            struct indexed_access<::rmath::rect, min_corner, D>
            {
                typedef typename point_type<::rmath::rect>::type pt;
                typedef typename coordinate_type<pt>::type ct;

                static inline ct get(::rmath::rect const& b)
                {
                    return geometry::get<D>(b.min);
                }

                static inline void set(::rmath::rect& b, ct const& value)
                {
                    geometry::set<D>(b.min, value);
                }
            };

            template<size_t D>
            struct indexed_access<::rmath::rect, max_corner, D>
            {
                typedef typename point_type<::rmath::rect>::type pt;
                typedef typename coordinate_type<pt>::type ct;

                static inline ct get(::rmath::rect const& b)
                {
                    return geometry::get<D>(b.max);
                }

                static inline void set(::rmath::rect& b, ct const& value)
                {
                    geometry::set<D>(b.max, value);
                }
            };
        }
    }
}

namespace scan_line
{
    template<typename T> struct all_function {
        typedef T seg;
    };

    namespace bg = boost::geometry;

    typedef dcel::half_edge* key;
    typedef ::rmath::rect box;
    typedef double ct;

    template<>
    struct all_function<dcel::half_edge*>
    {
        typedef ::rmath::rect box;

        static box get_rect(const key& in)
        {
            return in->get_boundary();
        }
        
        // 端点重合不用处理
        static void intersect(const key& r, const key& l)
        {
            constexpr int sort_value[] = { 0,1,10,100 };
            int sort = sort_value[(int)r->get_seg_type()] + sort_value[(int)l->get_seg_type()];

            // 这种分类发挺危险的，一旦 seg_type 有所改变，这里很容易失效
            switch (sort)
            {
            case 2:
                dcel::intersect((dcel::line_half_edge*)r, (dcel::line_half_edge*)l);
                break;
            case 11:
                break;
            case 20:
                break;
            case 101:
                break;
            case 110:
                break;
            case 200:
                break;
            default:
                break;
            }
        }
    };

    typedef all_function<dcel::half_edge*> funcs;

    class scan_line
    {
    public:
        struct seg
        {
            key key;
            box box;
        };

        struct scan_point
        {
            ct y{ 0 };

            std::vector<seg*> in;
            std::vector<seg*> out;
        };

        struct scan_point_compare
        {
            using is_transparent = void;
            using value = scan_point*;

            bool operator()(const value& l, const value& r) const {
                return l->y < r->y;
            };

            bool operator()(const value& l, const ct& r) const {
                return l->y < r;
            };
            bool operator()(const ct& l, const value& r) const {
                return l < r->y;
            };
        };

        void add_segment(const key& in);
        void process();


    private:
        bool intersect(const box& r, const box& l) const;
        // 查找或新增
        scan_point* get_point(const ct& y);
        std::set<scan_point*, scan_point_compare> scan_points;
        std::vector<seg*> segments;
    };
}
