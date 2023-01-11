#include "river.h"
#include "segment.h"

namespace river {
  path::path(path&& l) noexcept {
    data.swap(l.data);
  }

  path::~path() { for (auto segment : data) delete segment; }

  void path::moveto(const point& tp) { data.push_back(new seg_moveto(tp)); }
  void path::moveto(double x, double y) { data.push_back(new seg_moveto({ x,y })); }
  void path::lineto(const point& tp) { data.push_back(new seg_lineto(tp)); }
  void path::lineto(double x, double y) { data.push_back(new seg_lineto({ x,y })); }
  // 三点确定一个圆弧
  void path::arcto(const point& tp, const point& middle) { data.push_back(new seg_arcto(middle, tp)); }
  //void arcto(point const& center, double sweepRad);
  void path::cubicto(const point& ctrl1, const point& ctrl2, const point& end) { data.push_back(new seg_cubicto(ctrl1, ctrl2, end)); }
  // 二阶自动升三阶
  //void cubicto(const point& end, const point& ctrl);
}