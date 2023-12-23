#include "rmath.h"

namespace rmath
{
  inline bool is_zero(double v) {
    constexpr double err = 1e-8;
    return -err < v&& v < err;
  }

  double clamp_0_2pi(double rad) {
    rad = remainder(rad, pi * 2);
    return signbit(rad) ? rad + pi * 2 : rad;
  }

  bool rect::intersect(const rect& r) const
  {
    return !(max.x < r.min.x || min.x > r.max.x
      || max.y < r.min.y || min.y > r.max.y);
  }

  point_at_y_result bezier_cubic::point_at_y(double y) const
  {
    point_at_y_result res;
    res.count = 0;

    /*
    有直线 y + C = 0 和 bezier曲线 P = f(t), 则有
    fy(t) + C = 0
    为一元三次方程组，解方程即可
    */
    const vec2 t3 = p3 - 3 * p2 + 3 * p1 - p0;
    const vec2 t2 = 3 * p2 - 6 * p1 + 3 * p0;
    const vec2 t1 = 3 * p1 - 3 * p0;
    const vec2 t0 = p0;

    double C = -y;

    const double a3 = t3.y;
    const double a2 = t2.y;
    const double a1 = t1.y;
    const double a0 = t0.y + C;

    auto cubic_res = resolv_cubic_equa(a3, a2, a1, a0);
    for (auto i = 0; i < cubic_res.count; ++i) {
      auto t = cubic_res.data[i];
      if (0 <= t && t <= 1) {
        res.t[res.count] = t;
        res.count += 1;
      }
    }

    return res;
  }

  vec2 operator*(double sc, const vec2& v) { return v * sc; }
  double dist2_point_line(const vec2& p, const vec2& l0, const vec2& l1) {
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
  bool point_project_in_segment(const vec2& p, const vec2& p0, const vec2& p1)
  {
    if (p1 == p0) {
      return p == p0;
    }

    vec2 v = p1 - p0;
    vec2 u = p - p0;

    double sc = v.dot(u) / v.dot(v);
    return sc <= 1;
  }

  bool point_between_two_point(const vec2& p, const vec2& p0, const vec2& p1)
  {
    auto r = rect::from({ p0,p1 });

    if (p0.x == p1.x) return (r.min.y <= p.y) && (p.y <= r.max.y);
    return (r.min.x <= p.x) && (p.x <= r.max.x);
  }

  line_intersect_result intersect(const vec2& p0, const vec2& p1, const vec2& p2, const vec2& p3)
  {
    line_intersect_result res;
    res.count = 0;

    const vec2 a = p1 - p0;
    const vec2 b = p2 - p3;
    const vec2 c = p0 - p2;

    const double denominator = a.y * b.x - a.x * b.y;
    if (denominator == 0 || !std::isfinite(denominator))
    {
      // 此时平行
      if (dist2_point_line(p0, p2, p3) == 0)
      {
        // 此时共线
        if (p0 != p2 && p0 != p3 && point_between_two_point(p0, p2, p3)) {
          // p0 在 r 内
          auto& data = res.data[res.count];
          data.p = p0;
          data.t0 = 0;
          if (b.y == 0) data.t1 = (p0 - p2).x / -b.x;
          else data.t1 = (p0 - p2).y / -b.y;
          res.count++;
        }
        if (p1 != p2 && p1 != p3 && point_between_two_point(p1, p2, p3)) {
          // p1 在 r 内
          auto& data = res.data[res.count];
          data.p = p1;
          data.t0 = 1;
          if (b.y == 0) data.t1 = (p1 - p2).x / -b.x;
          else data.t1 = (p1 - p2).y / -b.y;
          res.count++;
        }

        if (p2 != p0 && p2 != p1 && point_between_two_point(p2, p0, p1)) {
          // p2 在 l 内
          auto& data = res.data[res.count];
          data.p = p2;
          data.t1 = 0;
          if (a.y == 0) data.t0 = (p2 - p0).x / a.x;
          else data.t0 = (p2 - p0).y / a.y;
          res.count++;
        }
        if (p3 != p0 && p3 != p1 && point_between_two_point(p3, p0, p1)) {
          // p3 在 l 内
          auto& data = res.data[res.count];
          data.p = p3;
          data.t1 = 1;
          if (a.y == 0) data.t0 = (p3 - p0).x / a.x;
          else data.t0 = (p3 - p0).y / a.y;
          res.count++;
        }
      }

      return res;
    }

    const double reciprocal = 1 / denominator;
    const double na = (b.y * c.x - b.x * c.y) * reciprocal;
    const double nb = (a.x * c.y - a.y * c.x) * reciprocal;

    const bool in_l = 0 <= na && na <= 1;
    const bool in_r = 0 <= nb && nb <= 1;

    if (in_l && in_r) {
      res.count = 1;
      res.data[0].p = (p0 + a * na);
      res.data[0].t0 = na;
      res.data[0].t1 = nb;
    }

    return res;
  }

  line_cubic_intersect_result intersect(const line& segment, const bezier_cubic& curve)
  {
    line_cubic_intersect_result res;

    auto& p0 = curve.p0;
    auto& p1 = curve.p1;
    auto& p2 = curve.p2;
    auto& p3 = curve.p3;

    /*
    有直线 Ax + By + C = 0 和 bezier曲线 P = f(t), 则有
    Afx(t) + Bfy(t) + C = 0
    为一元三次方程组，解方程即可
    */
    const vec2 t3 = p3 - 3 * p2 + 3 * p1 - p0;
    const vec2 t2 = 3 * p2 - 6 * p1 + 3 * p0;
    const vec2 t1 = 3 * p1 - 3 * p0;
    const vec2 t0 = p0;

    double A, B, C;
    segment.general_from(&A, &B, &C);

    const double a3 = A * t3.x + B * t3.y;
    const double a2 = A * t2.x + B * t2.y;
    const double a1 = A * t1.x + B * t1.y;
    const double a0 = A * t0.x + B * t0.y + C;

    auto l = segment.p1 - segment.p0;
    auto cubic_res = resolv_cubic_equa(a3, a2, a1, a0);
    for (auto i = 0; i < cubic_res.count; ++i) {
      auto t = cubic_res.data[i];
      if (0 <= t && t <= 1) {
        // ToDo 由于计算精度的问题，这里应该很难算出同一个点，特别实在交点靠近任意一方的端点时
        auto p = curve.point_at(t);
        if (point_between_two_point(p, segment.p0, segment.p1)) {
          auto& data = res.data[res.count];
          data.p = p;
          if (l.y == 0) { data.t0 = (p - segment.p0).x / l.x; }
          else { data.t0 = (p - segment.p0).y / l.y; }
          data.t1 = t;
          ++res.count;
        }
      }
    }

    return res;
  }

  void line::general_from(double* A, double* B, double* C) const
  {
    if (p0 == p1) {
      *A = 0;
      *B = 1;
      *C = -p0.y;
      return;
    }

    if (p0.x == p1.x) {
      *A = 1;
      *B = 0;
      *C = -p0.x;
      return;
    }

    if (p0.y == p1.y) {
      *A = 0;
      *B = 1;
      *C = -p0.y;
      return;
    }

    *A = p1.y - p0.y;
    *B = p0.x - p1.x;
    *C = (p1.x * p0.y) - (p0.x * p1.y);
  }

  arc::arc(vec2 const& in_start, vec2 const& in_center, double in_sweep_rad)
  {
    p0 = in_start;
    _center = in_center;
    _sweep_rad = in_sweep_rad;

    vec2 tmp = start() - center();
    _radius = tmp.dist();
    _start_rad = clamp_0_2pi(atan2(tmp.y, tmp.x));

    double end_rad = _start_rad + sweep_rad();
    p1 = vec2(cos(end_rad), sin(end_rad));
    p1 = center() + p1 * _radius;
    _end_rad = clamp_0_2pi(atan2(p1.y, p1.x));
  }

  const vec2& arc::start() const{
    return p0;
  }

  const vec2& arc::end() const{
    return p1;
  }

  const vec2& arc::center() const{
    return _center;
  }
  
  double arc::sweep_rad() const {
    return _sweep_rad;
  }

  double arc::start_rad() const {
    return _start_rad;
  }

  double arc::end_rad() const {
    return _end_rad;
  }

  double arc::radius() const {
    return _radius;
  }

  arc arc::reverse() const {
    arc r = *this;
    std::swap(r.p0, r.p1);
    std::swap(r._start_rad, r._end_rad);

    return r;
  }

  arc_split_y::arc_split_y(arc const& in_arc):target(in_arc)
  {
    /*******************************************************
    * 计算分段以及相关数据
    ********************************************************/

    double s_rad = target.start_rad();
    double e_rad = s_rad + target.sweep_rad();
    if (s_rad > e_rad) {
      std::swap(s_rad, e_rad);
    }

    // 因为 x 轴正方向为0，所以要调整相位
    s_rad -= pi / 2;
    e_rad -= pi / 2;

    s_rem = remainder(s_rad, pi);
    s_quot = s_rad / pi;

    e_rem = remainder(e_rad, pi);
    e_quot = e_rad / pi;

    // 相差不超过半圈
    if (s_quot == e_quot) {
      split_num = 0;
    }

    // 相差刚好半圈
    else if ((s_quot + 1) == e_quot && e_rad - s_rad == pi) {
      split_num = 0;
    }
    else 
    {
      split_num = e_quot - s_quot + (e_rem > 0 ? 1 : 0);
    }

    // 保底 1 段，也就是不拆分
    split_num++;

    /********************************************************
    * 开始拆分
    *********************************************************/
    std::vector<arc> arc_vector;

    if (split_num == 1) {
      arc_vector.push_back(target);
      return;
    }

    // 按照 sweep_rad 为正开始拆分
    vec2 s_p = target.start();
    vec2 e_p = target.end();
    if (target.sweep_rad() < 0) {
      std::swap(s_p, e_p);
    }

    arc_vector.reserve(split_num);
    // 头
    arc_vector.push_back(arc(s_p, target.center(), pi - s_rem));
    // 中间
    bool odd = div(s_quot + 1, 2).rem;
    for (size_t i = 2; i < split_num; ++i) {
      vec2 p_tmp = target.center() + vec2(0, odd ? target.radius() : -target.radius());
      arc_vector.push_back(arc(p_tmp, target.center(), pi));
      odd = !odd;
    }
    // 尾
    arc_vector.push_back(arc(e_p, target.center(), pi - e_rem).reverse());

    // 按需反转
    if (target.sweep_rad() < 0) {
      std::reverse(arc_vector.begin(), arc_vector.end());
      for (auto& a : arc_vector) { a = a.reverse(); }
    }
  }

  size_t arc_split_y::split_y_num() const
  {
    return split_num;
  }
  // out 预申请的空间必须满足 split_y_num() * sizeof(arc)
  void arc_split_y::split_y(arc* out) const
  {







  }

  arc const& arc_split_y::curr_arc() const { return target; }

  int bezier_cubic::split_y(double* t1, double* t2) const
  {
    //p = A (1-t)^3 +3 B t (1-t)^2 + 3 C t^2 (1-t) + D t^3
    //dp/dt =  3 (B - A) (1-t)^2 + 6 (C - B) (1-t) t + 3 (D - C) t^2
    //      =  [3 (D - C) - 6 (C - B) + 3 (B - A)] t^2
    //         + [ -6 (B - A) - 6 (C - B)] t
    //         + 3 (B - A)
    //      =  (3 D - 9 C + 9 B - 3 A) t^2 + (6 A - 12 B + 6 C) t + 3 (B - A)

    const vec2 A = p3 * 3 - p2 * 9 + p1 * 9 - p0 * 3;
    const vec2 B = p0 * 6 - p1 * 12 + p2 * 6;
    const vec2 C = (p1 - p0) * 3;

    double a, b, c, k;

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

  vec2 bezier_cubic::point_at(double t) const
  {
    double t2 = t * t;
    double t3 = t2 * t;
    double k = 1 - t;
    double k2 = k * k;
    double k3 = k2 * k;

    return p0 * k3 + p1 * 3 * k2 * t + p2 * 3 * k * t2 + p3 * t3;
  }

  rect bezier_cubic::get_boundary() const
  {
    double a, b, c, k;
    double t1, t2;

    //x = A (1-t)^3 +3 B t (1-t)^2 + 3 C t^2 (1-t) + D t^3
    //dx/dt =  3 (B - A) (1-t)^2 + 6 (C - B) (1-t) t + 3 (D - C) t^2
    //      =  [3 (D - C) - 6 (C - B) + 3 (B - A)] t^2
    //         + [ -6 (B - A) - 6 (C - B)] t
    //         + 3 (B - A)
    //      =  (3 D - 9 C + 9 B - 3 A) t^2 + (6 A - 12 B + 6 C) t + 3 (B - A)

    //不能直接求拐点，因为拐点不代表xy方向上的极值

    double x1, x2;
    const vec2 A = p3 * 3 - p2 * 9 + p1 * 9 - p0 * 3;
    const vec2 B = p0 * 6 - p1 * 12 + p2 * 6;
    const vec2 C = (p1 - p0) * 3;

    a = A.x;
    b = B.x;
    c = C.x;
    k = b * b - 4 * a * c;

    if (is_zero(a))
    {
      if (is_zero(b))
      {
        x1 = p0.x;
        x2 = p3.x;
      }
      else
      {
        t1 = -c / b;
        if (t1 > 0 && t1 < 1)
        {
          x1 = point_at(t1).x;
          x2 = x1;
        }
        else
        {
          x1 = p0.x;
          x2 = p3.x;
        }
      }
    }
    else if (k < 0)
    {
      x1 = p0.x;
      x2 = p3.x;
    }
    else
    {
      k = std::sqrt(k);
      t1 = (-b + k) / (2 * a);
      t2 = (-b - k) / (2 * a);

      if (t1 > 0 && t1 < 1)
      {
        x1 = point_at(t1).x;
      }
      else
        x1 = p0.x;

      if (t2 > 0 && t2 < 1)
      {
        x2 = point_at(t2).x;
      }
      else
        x2 = p3.x;

    }

    double y1, y2;
    a = A.y;
    b = B.y;
    c = C.y;
    k = b * b - 4 * a * c;

    if (is_zero(a))
    {
      if (is_zero(b))
      {
        y1 = p0.y;
        y2 = p3.y;
      }
      else
      {
        t1 = -c / b;
        if (t1 > 0 && t1 < 1)
        {
          y1 = point_at(t1).y;
          y2 = y1;
        }
        else
        {
          y1 = p0.y;
          y2 = p3.y;
        }
      }
    }
    else if (k < 0)
    {
      y1 = p0.y;
      y2 = p3.y;
    }
    else
    {
      k = std::sqrt(k);
      t1 = (-b + k) / (2 * a);
      t2 = (-b - k) / (2 * a);

      if (t1 > 0 && t1 < 1)
      {
        y1 = point_at(t1).y;
      }
      else
        y1 = p0.y;

      if (t2 > 0 && t2 < 1)
      {
        y2 = point_at(t2).y;
      }
      else
        y2 = p3.y;
    }

    vec2 tp1((std::min)(x1, x2), (std::min)(y1, y2));
    vec2 tp2((std::max)(x1, x2), (std::max)(y1, y2));

    return rect::from({ p0, p3, tp1, tp2 });
  }

  void bezier_cubic::split(const double* t, bezier_cubic* out, size_t size) const
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

  void bezier_cubic::circle(const vec2& center, double r, bezier_cubic* out)
  {
    constexpr auto magic = 0.55228475;
    auto d = r * magic;
    out[0].p0 = center + vec2(r, 0);
    out[0].p1 = out[0].p0 + vec2(0, d);
    out[0].p3 = center + vec2(0, r);
    out[0].p2 = out[0].p3 + vec2(d, 0);

    out[1].p0 = center + vec2(0, r);
    out[1].p1 = out[1].p0 - vec2(d, 0);
    out[1].p3 = center - vec2(r, 0);
    out[1].p2 = out[1].p3 + vec2(0, d);

    out[2].p0 = center - vec2(r, 0);
    out[2].p1 = out[2].p0 - vec2(0, d);
    out[2].p3 = center - vec2(0, r);
    out[2].p2 = out[2].p3 - vec2(d, 0);

    out[3].p0 = center - vec2(0, r);
    out[3].p1 = out[3].p0 + vec2(d, 0);
    out[3].p3 = center + vec2(r, 0);
    out[3].p2 = out[3].p3 - vec2(0, d);
  }


  quad_equa_result resolv_quad_equa(double a, double b, double c)
  {
    quad_equa_result res;
    if (is_zero(a))
    {
      //一元方程
      if (is_zero(b))
        return res;

      res.count = 1;
      res.data[0] = -c / b;
      return res;
    }

    double k = b * b - 4 * a * c;
    if (is_zero(k))
    {
      res.count = 1;
      res.data[0] = -b / (2 * a);
    }
    else if (k > 0)
    {
      double sqrtK = std::sqrt(k);
      res.count = 2;
      res.data[0] = (-b + sqrtK) / (2 * a);
      res.data[1] = (-b - sqrtK) / (2 * a);
    }

    return res;
  }

  cubic_equa_result resolv_cubic_equa(double a3, double a2, double a1, double a0)
  {
    cubic_equa_result res;

    // ToDo 这里还需要判断a3与a2的指数是否相差过大
    if (is_zero(a3))
    {
      //二次方程
      auto quad_res = resolv_quad_equa(a2, a1, a0);
      res.count = quad_res.count;
      for (int i = 0; i < quad_res.count; ++i) {
        res.data[i] = quad_res.data[i];
      }
      return res;
    }

    //有 a3·x^3 + a2·x^2 + a1·x + a0 = 0

    //设 a = a2 / a3 和 b = a1 / a3 和 c = a0 / a3;
    //有 x^3 + a·x^2 + b·x + c = 0
    double a, b, c;
    a = a2 / a3;
    b = a1 / a3;
    c = a0 / a3;

    //设 x = y - a/3 有
    //      (y - a/3)^3 + a·(y - a/3)^2 + b·(y - a/3) + c = 0
    //化简得
    //      y^3 + (b - a·a/3)y + (2·a^3/27 - a·b/3 +c) = 0
    //设 p = b - aa/3 和 q = 2a^3/27 - ab/3 + c，得
    //      y^3 + p·y + q = 0
    double aPow2 = a * a;
    double p = b - aPow2 / 3;
    double q = 2 * aPow2 * a / 27 - a * b / 3 + c;

    //令 y = v + w，得
    //      (v + w)^3 + p·(v + w) + q = 0
    //化简，有
    //      (3·v·w + p)(v + w) + (v^3 + w^3 + q) = 0

    //选择 3·v·w + p = 0 作为附加条件
    //有
    //      v^3 + w^3 = -q
    //      v·w = -p/3    ......(1)
    //因此有
    //      v^3 + w^3 = -q
    //      v^3·w^3 = -(p^3)/(3^3)    ......(2)
    //(1)的解是(2)的解，但是(2)的解不一定是(1)的解，因此选择满足方程的的解:
    //      v·w = -p/3
    //根据韦达定理，(2)的解是下述二次方程的根
    //韦达定理中一般二次方程中有x1 + x2 = -b/a, x1·x2=c/a, 设v^3, w^3分别为韦达定理中的x1, x2，可得以下方程
    //      m^2 + q·m - (p/3)^3 = 0
    //该二次方程的解为
    //      m1,2 = -q/2 ± sqrt( (q/2)^2 + (p/3)^3 )
    //因此，有
    //      v = cbrt( -q/2 + sqrt( (q/2)^2 + (p/3)^3 ) )
    //      w = cbrt( -q/2 - sqrt( (q/2)^2 + (p/3)^3 ) )
    //所以对于y^3 + p·y + q = 0的解可以写成如下形式
    //      y = v + w = cbrt( -q/2 + sqrt( (q/2)^2 + (p/3)^3 ) ) + cbrt( -q/2 - sqrt( (q/2)^2 + (p/3)^3 ) )

    //对于三次方程y^3 + p·y + q = 0，记(q/2)^2 + (p/3)^3为判别式，以下记判别式为k
    //如果k > 0, 有一个实数解和两个虚数解
    //如果k = 0, 有三个实数解，但至少有两个解是相同的
    //如果k < 0, 有三个不同的实数解

    //令
    //      ϵ = -1/2 - sqrt(3)/2i  (复数)
    //作为1的第三个根，于是有
    //      ϵ^2 = -1/2 + sqrt(3)/2i
    //      ϵ^3 = 1
    //令
    //      v1 = cbrt( -q/2 + sqrt( (q/2)^2 + (p/3)^3 ) )
    //作为任何值的第三个根和
    //      w1 = -p/(3·v1)
    //因此三次方程y^3 + p·y + q = 0的解可以写成以下形式
    //      y1 = v1 + w1
    //      y2 = v1·ϵ + w1·ϵ^2
    //      y3 = v1·ϵ^2 + w1·ϵ


    double k = std::pow(q / 2, 2) + std::pow(p / 3, 3);
    if (is_zero(k))
    {
      //此时v1, w1均为实数
      //观察有ϵ和ϵ^2为共轭复数
      //此时有两个实数解，已有一个实数解y1 = v1 + w1
      //剩下的解为
      //      y2 = v1·ϵ + w1·ϵ^2
      //      y3 = v1·ϵ^2 + w1·ϵ
      //在虚数部分有定有
      //      0 = v1 - w1
      //      v1 = w1
      //则解为
      //      y1 = v1 + w1
      //      y2 = -1/2·(v1 + w1) = -v1
      //且v1 = w1

      double v1 = std::cbrt(-q / 2);
      if (is_zero(v1)) {
        res.count = 1;
        res.data[0] = 0. - a / 3;
      }
      else
      {
        res.count = 2;
        res.data[0] = v1 * 2 - a / 3;
        res.data[1] = -v1 - a / 3;
      }
    }
    else if (k > 0.)
    {
      //此时v1, w1均为实数
      //只有一个实数解则只能是y1 = v1 + w1
      double sqrtk = std::sqrt(k);
      double v1 = std::cbrt(-q / 2 + sqrtk);
      double w1 = -p / (3 * v1);

      res.count = 1;
      res.data[0] = v1 + w1 - a / 3;
    }
    else
    {
      //当k<0时，有三个实数解
      //根据对虚数的开方公式可得
      //      y1 = z1 + w3 = 2·cbrt(r)·cos(φ/3)
      //      y2 = z2 + w2 = 2·cbrt(r)·cos((φ + 2π)/3)
      //      y3 = z3 + w1 = 2·cbrt(r)·cos((φ + 4π)/3)
      //其中
      //      z = -q/2 + sqrt(k)
      //      φ = arctan[tan(φ)], 有tan(φ) = -2·sqrt(-k)/q, 如果tan(φ)<0, 结果要改变正负符号
      double sqrt_minus_k = std::sqrt(-k);
      double tanPhi = -2 * sqrt_minus_k / q;
      double phi = std::atan(tanPhi);
      double minus = (tanPhi < 0) ? -1 : 1;

      double r = std::sqrt(q * q / 4 - k);
      double AAA = 2 * std::cbrt(r);
      constexpr double pi = 3.1415926535;

      res.count = 3;
      res.data[0] = minus * AAA * std::cos(phi / 3) - a / 3;
      res.data[1] = minus * AAA * std::cos((phi + 2 * pi) / 3) - a / 3;
      res.data[2] = minus * AAA * std::cos((phi + 4 * pi) / 3) - a / 3;
    }

    return res;
  }

  inline rect rough_rect(const bezier_cubic& b) {
    return rect::from({ b.p0, b.p1, b.p2, b.p3 });
  }

  struct data
  {
    bezier_cubic bezier;
    rect rect;
    double t0 = 0;
    double t1 = 1;
    int recurse = 0;

    inline void subdive(data& out1, data& out2) const
    {
      bezier.split(0.5, &out1.bezier, &out2.bezier);
      out1.rect = rough_rect(out1.bezier);
      out1.t0 = t0;
      out1.t1 = (t0 + t1) / 2;
      out1.recurse = recurse + 1;
      out2.rect = rough_rect(out2.bezier);
      out2.t0 = out1.t1;
      out2.t1 = t1;
      out2.recurse = recurse + 1;
    }
  };

  // 使用bound计算近似轮廓，不计算自交内容
  cubic_intersect_result intersect(const bezier_cubic& b1, const bezier_cubic& b2, int precise)
  {
    cubic_intersect_result res;
    res.count = 0;


    typedef std::pair<data, data> data_pair;
    std::vector<data_pair> pairs;
    std::vector<data_pair> intersect_pair;

    data p1{ b1,rough_rect(b1),0,1,0 };
    data p2{ b2,rough_rect(b2),0,1,0 };
    if (!p1.rect.intersect(p2.rect)) return res;

    pairs.push_back(data_pair(p1, p2));
    while (!pairs.empty()) {
      auto back = pairs.back();
      pairs.pop_back();

      data p11, p12, p21, p22;
      back.first.subdive(p11, p12);
      back.second.subdive(p21, p22);

      constexpr auto check_func = [](const data& p0, const data& p1, int precise,
        decltype(pairs)& next_out, decltype(intersect_pair)& intersect_out) {
          if (p0.rect.intersect(p1.rect)) {
            if (p0.recurse > precise)
              intersect_out.push_back(data_pair(p0, p1));
            else
              next_out.push_back(data_pair(p0, p1));
          }
      };

      check_func(p11, p21, precise, pairs, intersect_pair);
      check_func(p11, p22, precise, pairs, intersect_pair);
      check_func(p12, p21, precise, pairs, intersect_pair);
      check_func(p12, p22, precise, pairs, intersect_pair);
    }

    std::sort(intersect_pair.begin(), intersect_pair.end(),
      [](const data_pair& l, const data_pair& r) {
        return l.first.t0 < r.first.t0;
      });
    // 移除太过相似的结果
    intersect_pair.erase(std::unique(intersect_pair.begin(), intersect_pair.end(),
      [](const data_pair& l, const data_pair& r) ->bool {
        return (std::abs)(l.first.t0 - r.first.t0) < 1e-8;
      }), intersect_pair.end());

    // ToDo 不能检查部分重合的情况
    for (int i = 0; i < intersect_pair.size(); ++i) {
      auto& p = intersect_pair[i];
      res.data[i].t0 = (p.first.t0 + p.first.t1) / 2;
      res.data[i].t1 = (p.second.t0 + p.second.t1) / 2;
      res.data[i].p = b1.point_at(res.data[i].t0);
      res.count += 1;
      if (res.count == 9) break;
    }

    return res;
  }
}
