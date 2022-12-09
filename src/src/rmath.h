#pragma once

namespace rmath
{
    struct vec2
    {
        double x = 0;
        double y = 0;

        vec2() = default;
        vec2(double _x, double _y) :x(_x), y(_y) {}

        vec2 operator+(const vec2& r) const { return vec2(x + r.x, y + r.y); }
        vec2 operator-() const { return vec2(-x, -y); }
        vec2 operator-(const vec2& r) const { return vec2(x - r.x, y - r.y); }
        vec2 operator*(double sc) const { return vec2(x * sc, y * sc); }

        bool operator==(const vec2& r) const { return x == r.x && y == r.y; }
        bool operator!=(const vec2& r) const { return !(*this == r); }

        // 点乘
        double dot(const vec2& p) const { return x * p.x + y * p.y; }
        // 求模的平方
        double dist2() const { return x * x + y * y; }
    };

    vec2 operator*(double sc, const vec2& v);

    struct rect
    {
        vec2 min;
        vec2 max;
    };

    // 计算点到（无限长）直线的距离的平方
    double dist2_point_line(const vec2& p, const vec2& l0, const vec2& l1);

    // 计算点投影到（有限长）线段的点是否位于线段内
    bool point_project_in_segment(const vec2& p, const vec2& p0, const vec2& p1);
}