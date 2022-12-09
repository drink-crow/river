#include "rmath.h"

namespace rmath
{
    vec2 operator*(double sc, const vec2& v) { return v * sc; }
    double rmath::dist2_point_line(const vec2& p, const vec2& l0, const vec2& l1) {
        vec2 AC, D;
        AC = p - l0;
        vec2 AB = l1 - l0;
        double f = 0, d = AB.dist2(), res;
        f = AB.dot(AC);
        if (f < 0)
            res = AC.dist2();
        else if (f > d)
            res = (p - l1).dist2();
        else
        {
            f = f / d;
            D = l0 + AB * f;
            res = (p - D).dist2();
        }

        return res;
    }

    // 计算点投影到（有限长）线段的点是否位于线段内
    bool rmath::point_project_in_segment(const vec2& p, const vec2& p0, const vec2& p1)
    {
        if (p1 == p0) {
            return p == p0;
        }

        vec2 v = p1 - p0;
        vec2 u = p - p0;

        double sc = v.dot(u) / v.dot(v);
        return sc <= 1;
    }
}
