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
            struct tag<::math::vec2> {
                typedef point_tag type;
            };

            template<>
            struct coordinate_type<::math::vec2>
            {
                typedef double type;
            };

            template<>
            struct access<::math::vec2, 0>
            {
                static inline double get(const ::math::vec2& p)
                {
                    return p.x;
                }
                static inline void set(::math::vec2& p, const double& value)
                {
                    p.x = value;
                }
            };

            template<>
            struct access<::math::vec2, 1>
            {
                static inline double get(const ::math::vec2& p)
                {
                    return p.y;
                }
                static inline void set(::math::vec2& p, const double& value)
                {
                    p.y = value;
                }
            };

            template<>
            struct tag<::math::rect>
            {
                typedef box_tag type;
            };
            template<>
            struct point_type<::math::rect>
            {
                typedef ::math::vec2 type;
            };

            template<size_t D>
            struct indexed_access<::math::rect, min_corner, D>
            {
                typedef typename point_type<::math::rect>::type pt;
                typedef typename coordinate_type<pt>::type ct;

                static inline ct get(::math::rect const& b)
                {
                    return geometry::get<D>(b.min);
                }

                static inline void set(::math::rect& b, ct const& value)
                {
                    geometry::set<D>(b.min, value);
                }
            };

            template<size_t D>
            struct indexed_access<::math::rect, max_corner, D>
            {
                typedef typename point_type<::math::rect>::type pt;
                typedef typename coordinate_type<pt>::type ct;

                static inline ct get(::math::rect const& b)
                {
                    return geometry::get<D>(b.max);
                }

                static inline void set(::math::rect& b, ct const& value)
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
    

    template<typename seg, typename box>
    class scan_line
    {
    public:
        
    private:
        std::vector<seg> segments;
    };

    template<>
    struct all_function<dcel::half_edge*>
    {
        
    };
}
