#include "segment.h"

namespace river {
  const point& seg_moveto::get_target() const {
    return target;
  }

  seg_type seg_moveto::get_type() const
  {
    return seg_type::moveto;
  }

  const point& seg_lineto::get_target() const {
    return target;
  }

  seg_type seg_lineto::get_type() const
  {
    return seg_type::lineto;
  }

  const point& seg_arcto::get_target() const {
    return target;
  }

  seg_type seg_arcto::get_type() const
  {
    return seg_type::arcto;
  }

  const point& seg_cubicto::get_target() const {
    return target;
  }

  seg_type seg_cubicto::get_type() const
  {
    return seg_type::cubicto;
  }

  double seg_cubicto::curr_x(const point& from, double y) const
  {
    auto cubic = get_cubic(from);
    auto res = cubic.point_at_y(y);
    if (res.count == 1) {
      return cubic.point_at(res.t[0]).x;
    }
    else {
      return (target.x - from.x) * (y - from.y) / (target.y - from.y) + from.x;
    }

  }


}