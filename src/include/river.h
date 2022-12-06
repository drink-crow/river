#pragma once

#include <type_traits>
namespace river {

enum class seg_type
{
    MOVETO,
    LINETO,
    ARCTO,
    CUBICTO
};

namespace path {
    struct point {
        double x;
        double y;
    };

    struct seg
    {
        virtual const point& get_target() const = 0;
        virtual seg_type get_type() const = 0;
    };

    struct seg_moveto : public seg
    {
        point target;

        const point & get_target() const override;
        seg_type get_type() const override;
    };

    struct seg_lineto : public seg
    {
        point target;

        const point & get_target() const override;
        seg_type get_type() const override;
    };

    struct seg_arcto : public seg
    {
        point target;
        point center;
        bool longarc;

        const point & get_target() const override;
        seg_type get_type() const override;
    };

    struct seg_cubicto : public seg
    {
        point ctrl_point1;
        point ctrl_point2;
        point target;

        const point & get_target() const override;
        seg_type get_type() const override;
    };
}

namespace traits {
    struct unknow_tag{};
    struct point_tag{};
    struct line_tag{};
    struct arc_tag{};
    struct cubic_tag{};

    template<typename T> struct tag{};
    template<typename T> struct coordinate_type{};
    template<typename T> struct access_x {};
    template<typename T> struct access_y {};

    template<>
    struct tag<path::point>
    {
        typedef point_tag type ;
    };

    template<>
    struct coordinate_type<path::point>
    {
        typedef double type ;
    };

    template<>
    struct access_x<path::point>
    {
        static inline double get(const path::point& p)
        {
            return p.x;
        }
        static inline void set(path::point& p, const double& value)
        {
            p.x = value;
        }
    };

    template<>
    struct access_y<path::point>
    {
        static inline double get(const path::point& p)
        {
            return p.y;
        }
        static inline void set(path::point& p, const double& value)
        {
            p.y = value;
        }
    };
}

}
