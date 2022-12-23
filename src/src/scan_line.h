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
    // ToDo 去除需要boost.geometry.traits的内容，可以直接获取数据在组织，这部分内容可以内置起来
    
    template<typename T> struct all_function {
        // you must do below typedef and func
        // typedef myAABBtype box;
        // typedef myNumType ct;
        // typedef myKeyType key;
        // 
        // static box get_rect(const key& in)
        // {
        //     return AABB_of(in);
        // }
        // static void intersect(const key& r, const key& l, void* user)
        // {
        //     // the code precess intersect of key r vers key l
        // }
    };

    template<class key>
    class scan_line
    {
    public:
        typedef typename ::scan_line::all_function<key> funcs;
        typedef typename funcs::box box;
        typedef typename funcs::ct ct;

        struct seg
        {
            key key;
            box box;
        };

        struct scan_point
        {
            ct x{ 0 };

            std::vector<seg*> in;
            std::vector<seg*> out;
        };

        struct scan_point_compare
        {
            using is_transparent = void;
            using value = scan_point*;

            bool operator()(const value& l, const value& r) const {
                return l->x < r->x;
            };

            bool operator()(const value& l, const ct& r) const {
                return l->x < r;
            };
            bool operator()(const ct& l, const value& r) const {
                return l < r->x;
            };
        };

        void add_segment(const key& in);
        void process(void* user);


    private:
        bool intersect(const box& r, const box& l) const;
        // 查找或新增
        scan_point* get_point(const ct& x);
        std::set<scan_point*, scan_point_compare> scan_points;
        std::vector<seg*> segments;
    };

    template<class key>
    void scan_line<key>::add_segment(const key& in)
    {
        namespace bg = boost::geometry;

        auto new_seg = new seg{ in, funcs::get_rect(in) };

        segments.push_back(new_seg);

        auto startp = get_point(bg::get<bg::min_corner, 0>(new_seg->box));
        startp->in.push_back(new_seg);
        auto endp = get_point(bg::get<bg::max_corner, 0>(new_seg->box));
        endp->out.push_back(new_seg);
    }

    template<class key>
    void scan_line<key>::process(void* user)
    {
        std::set<seg*> current;

        for (auto p : scan_points)
        {
            for (auto in : p->in)
            {
                // 每一新添加的对象，和列表中的计算一次交点
                for (auto c : current) {
                    if (intersect(c->box, in->box)) {
                        funcs::intersect(c->key, in->key, user);
                    }
                }

                current.insert(in);
            }

            for (auto out : p->out) {
                current.erase(out);
            }
        }
    }

    template<class key>
    bool scan_line<key>::intersect(const box& r, const box& l) const
    {
        namespace bg = boost::geometry;

        return bg::intersects(r, l);
    }

    template<class key>
    typename scan_line<key>::scan_point* scan_line<key>::scan_line::get_point(const ct& x)
    {
        scan_point* res = nullptr;

        auto search = scan_points.find(x);
        if (search == scan_points.end()) {
            res = new scan_point;
            res->x = x;
            scan_points.insert(res);
        }
        else {
            res = *search;
        }

        return res;
    }

}
