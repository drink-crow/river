#pragma once

#include <cmath>
#include <limits>
#include <algorithm>
#include <initializer_list>
namespace rmath
{

    constexpr double pi = 3.141592653589793;

    struct vec2
    {
        typedef double num;

        num x = 0;
        num y = 0;

        vec2() = default;
        vec2(num _x, num _y) :x(_x), y(_y) {}
        vec2(num _v) :x(_v),  y(_v) {}

        vec2 operator+(const vec2& r) const { return vec2(x + r.x, y + r.y); }
        vec2 operator-() const { return vec2(-x, -y); }
        vec2 operator-(const vec2& r) const { return vec2(x - r.x, y - r.y); }
        vec2 operator*(num sc) const { return vec2(x * sc, y * sc); }
        vec2 operator/(num sc) const { return vec2(x / sc, y / sc); }

        bool operator==(const vec2& r) const { return x == r.x && y == r.y; }
        bool operator!=(const vec2& r) const { return !(*this == r); }

        // 点乘
        num dot(const vec2& p) const { return x * p.x + y * p.y; }
        num cross(const vec2& p) const { return x*p.y - p.x*y; }
        // 求模的平方
        num dist2() const { return x * x + y * y; }
        num dist() const { return std::sqrt(dist2()); }

        // 弧度制
        vec2 rotated(num rad) const {
            double cos = (std::cos)(rad); double sin = (std::sin)(rad); return vec2(cos * x - sin * y, sin * x + cos * y);
        }
        // 弧度制
        vec2& rotate(num rad) { *this = rotated(rad); return *this; }

        vec2 normalization() const { num l = dist(); return *this / l; }
        // 归一化
        vec2& normalizate() { *this = normalization(); return *this; }
    };

    vec2 operator*(vec2::num sc, const vec2& v);

    struct rect
    {
        typedef vec2 point_type;

        point_type min;
        point_type max;

        rect() = default;
        rect(const point_type& p):min(p),max(p){}
        rect(const point_type& _min, const point_type& _max) : min(_min), max(_max) {}

        static rect from(std::initializer_list<point_type> _Ilist)
        {
            constexpr auto _max = std::numeric_limits<point_type::num>::max();
            rect out(point_type(_max),
            point_type(-_max));
            for (auto& e : _Ilist)
            {
                out.min.x = (std::min)(out.min.x, e.x);
                out.min.y = (std::min)(out.min.y, e.y);

                out.max.x = (std::max)(out.max.x, e.x);
                out.max.y = (std::max)(out.max.y, e.y);
            }
            return out;
        }
    };

    // 计算点到（无限长）直线的距离的平方
    double dist2_point_line(const vec2& p, const vec2& l0, const vec2& l1);

    // 计算点投影到（有限长）线段的点是否位于线段内
    bool point_project_in_segment(const vec2& p, const vec2& p0, const vec2& p1);
}