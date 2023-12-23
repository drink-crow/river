#pragma once

#include <vector>
#include <cmath>
#include <limits>
#include <algorithm>
#include <initializer_list>
namespace rmath
{

  constexpr double pi = 3.141592653589793;

  // 处理弧度 rad 为 [0,2pi)
  double clamp_0_2pi(double rad);

  struct vec2
  {
    typedef double num;

    num x = 0;
    num y = 0;

    vec2() = default;
    vec2(num _x, num _y) :x(_x), y(_y) {}
    vec2(num _v) :x(_v), y(_v) {}

    vec2 operator+(const vec2& r) const { return vec2(x + r.x, y + r.y); }
    vec2 operator+(const num& n) const { return vec2(x + n, y + n); }
    vec2 operator-() const { return vec2(-x, -y); }
    vec2 operator-(const vec2& r) const { return vec2(x - r.x, y - r.y); }
    vec2 operator-(const num& n) const { return vec2(x - n, y - n); }
    vec2 operator*(num sc) const { return vec2(x * sc, y * sc); }
    vec2 operator/(num sc) const { return vec2(x / sc, y / sc); }

    bool operator==(const vec2& r) const { return x == r.x && y == r.y; }
    bool operator!=(const vec2& r) const { return !(*this == r); }

    // 点乘
    num dot(const vec2& p) const { return x * p.x + y * p.y; }
    num cross(const vec2& p) const { return x * p.y - p.x * y; }
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
    typedef point_type::num num;

    point_type min;
    point_type max;

    rect() = default;
    rect(const point_type& p) :min(p), max(p) {}
    rect(const point_type& _min, const point_type& _max) : min(_min), max(_max) {}
    rect(const point_type& p, num offset): min(p-offset), max(p+offset)
    {}
    bool intersect(const rect& r) const;
    num width() const { return max.x - min.x; }
    num height() const { return max.y - min.y; }
    point_type center() const { return (max + min) / 2; }

    rect inflated(const num& n) const { return rect(min - n, min + n); }

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

  struct mat3 {
    typedef double num;
    num value[3][3];
  };

  // 计算点到（无限长）直线的距离的平方
  double dist2_point_line(const vec2& p, const vec2& l0, const vec2& l1);

  // 计算点投影到（有限长）线段的点是否位于线段内
  bool point_project_in_segment(const vec2& p, const vec2& p0, const vec2& p1);

  struct line
  {
    vec2 p0, p1;

    // 将直线整理成一般形式 Ax + By + C = 0
    void general_from(double* out_A, double* out_B, double* out_C) const;
  };

  struct point_at_y_result {
    int count = 0; // at most 3
    double t[3];
  };

  // 用到的所有rad均为弧度制，正为逆时针，负为顺时针, x轴正方向为0
  struct arc
  {
    arc(vec2 const& start, vec2 const& center, double sweep_rad);
    // 
    //arc(vec2 const& start, vec2 const& mid, vec2 const& end);
    //arc(vec2 const& start, vec2 const& center, vec2 const& end, bool is_long_arc);

    const vec2& start() const;
    const vec2& end() const;
    const vec2& center() const;
    double sweep_rad() const;
    double start_rad() const;
    double end_rad() const;
    double radius() const;

    // 反转朝向
    arc reverse() const;

  private:
    vec2 p0,p1,_center;
    double _sweep_rad, _start_rad, _end_rad, _radius;
  };

  struct arc_split_y {
    arc_split_y(arc const& in_arc);

    // 拆分成y轴单调的小弧，返回拆分后的数量，返回值 >= 1, 为 1 也就是不拆分
    size_t split_y_num() const;
    // out 的数量必须满足 split_y_num()
    void split_y(arc* out) const;

    arc const& curr_arc() const;

  private:
    arc target;

    double s_rem, e_rem;
    int s_quot, e_quot;
    size_t split_num;
  };


  struct bezier_cubic
  {
    vec2 p0, p1, p2, p3;

    [[nodiscard]] vec2 point_at(double t) const;
    [[nodiscard]] point_at_y_result point_at_y(double y) const;
    [[nodiscard]] rect get_boundary() const;
    // 分离出的每一部分在 y 轴上单调, 返回打断点的个数，最多为 2
    [[nodiscard]] int split_y(double* t1, double* t2) const;
    void split(const double* t, bezier_cubic* out, size_t size) const;
    void split(double t, bezier_cubic* out1, bezier_cubic* out2) const;

    // out size must NOT LESS than 4
    static void circle(const vec2& center, double r, bezier_cubic* out);
  };

  struct quad_equa_result
  {
    int count = 0; // 最多2个实数解
    double data[2];
  };
  quad_equa_result resolv_quad_equa(double a2, double a1, double a0);

  struct cubic_equa_result
  {
    int count = 0; // 实数解数量，最多三个
    double data[3];
  };
  // 解一般一元三次方程a3·x^3 + a2·x^2 + a1·x + a0 = 0的实数根
  cubic_equa_result resolv_cubic_equa(double a3, double a2, double a1, double a0);

  struct intersect_info
  {
    vec2 p{ 0 };
    double t0 = 0;
    double t1 = 0;
  };
  struct line_intersect_result {
    int count = 0;  // 最多有2个交点(共线时)
    intersect_info data[2];
  };

  // 数据中 t0 是 line{p0p1} 的，t1 是line{p2p3}的
  [[nodiscard]] line_intersect_result intersect(const vec2& p0, const vec2& p1, const vec2& p2, const vec2& p3);

  struct line_cubic_intersect_result {
    int count = 0; // 最多有3个交点
    intersect_info data[3];
  };
  // 仅计算线段和三次贝塞尔相交的情况，对于贝塞尔本身的自交不做计算
  // 数据中 t0 是 segment 的，t1 是curve的
  [[nodiscard]] line_cubic_intersect_result intersect(const line& segment, const bezier_cubic& curve);

  struct cubic_intersect_result {
    int count = 0; // 最多9个交点
    intersect_info data[9];
  };
  // precise 用与控制结果 t 的有效位数
  [[nodiscard]] cubic_intersect_result intersect(const bezier_cubic& b1, const bezier_cubic& b2, int precise = 25);

}