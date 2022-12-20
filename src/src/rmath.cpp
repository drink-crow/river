#include "rmath.h"

namespace rmath
{
    bool is_zero(double v) {
        constexpr double err = 1e-8;
        return -err < v && v < err;
    }

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

    int bezier_cubic::split_y(double* t1, double* t2) const
    {
        //p = A (1-t)^3 +3 B t (1-t)^2 + 3 C t^2 (1-t) + D t^3
        //dp/dt =  3 (B - A) (1-t)^2 + 6 (C - B) (1-t) t + 3 (D - C) t^2
        //      =  [3 (D - C) - 6 (C - B) + 3 (B - A)] t^2
        //         + [ -6 (B - A) - 6 (C - B)] t
        //         + 3 (B - A)
        //      =  (3 D - 9 C + 9 B - 3 A) t^2 + (6 A - 12 B + 6 C) t + 3 (B - A)

        double y1, y2;
        const vec2 A = p3 * 3 - p2 * 9 + p1 * 9 - p0 * 3;
        const vec2 B = p0 * 6 - p1 * 12 + p2 * 6;
        const vec2 C = (p1 - p0) * 3;

        double a, b, c, k;
        double t1, t2;

        a = A.y;
        b = B.y;
        c = C.y;
        k = b * b - 4 * a * c;

        if (is_zero(a)) {
            if (is_zero(b)) {
                return 0;
            }
            else {
                auto t = -c / b;
                if (0 < t && t < 1) {
                    *t1 = t;
                    return 1;
                }
            }
        }
        else if (k > 0) {
            k = std::sqrt(k);
            auto r1 = (-b + k) / (2 * a);
            auto r2 = (-b - k) / (2 * a);
            
            std::vector<double> res;
            if (0 < r1 && r1 < 1) {
                res.push_back(r1);
            }
            if (0 < r2 && r2 < 1) {
                res.push_back(r2);
            }

            switch (res.size())
            {
            case 1: *t1 = res[0]; return 1;
            case 2: *t1 = res[0]; *t2 = res[1]; return 2;
            default: return 0;
            }
        }

        return 0;
    }

    void bezier_cubic::split(double* t, bezier_cubic* out, size_t size) const
    {
        bezier_cubic cur = *this;
        double last_t = 1;
        for (size_t i = 0; i < size; ++i) {
            double cur_t = (t[i] - (1 - last_t)) / last_t;
            cur.split(cur_t, out + i, out + i + 1);
            cur = out[i + 1];
            last_t = 1 - t[i];
        }
    }

    void bezier_cubic::split(double t, bezier_cubic* out1, bezier_cubic* out2) const
    {
        double t1 = 1 - t;

        const vec2 p01(p1 * t + p0 * t1);
        const vec2 p12(p2 * t + p1 * t1);
        const vec2 p23(p3 * t + p2 * t1);
        const vec2 p01_12(p12 * t + p01 * t1);
        const vec2 p12_23(p23 * t + p12 * t1);
        const vec2 p01_12__12_23(p12_23 * t + p01_12 * t1);

        out1->p0 = p0; out1->p1 = p01; out1->p2 = p01_12; out1->p3 = p01_12__12_23;
        out2->p0 = p01_12__12_23; out2->p1 = p12_23; out2->p2 = p23; out2->p3 = p3;
    }
}
