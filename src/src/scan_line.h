#pragma once
#include "math.h"
#include "dcel.h"
#include <vector>

namespace scan_line
{
  // ToDo 去除需要boost.geometry.traits的内容，可以直接获取数据在组织，这部分内容可以内置起来

  template<typename T> struct all_function {
    // you must do below typedef and func
    // typedef myAABBtype box;
    // typedef myNumType ct;
    // typedef mySegmentType segment;
    // 
    // static bool intersect(const box& a, const box& b) {
    //   return true if a intecrsect b
    // }
    // 
    // static ct get_min_x(const box& b) { ... }
    // static ct get_min_y(const box& b) { ... }
    // static ct get_max_x(const box& b) { ... }
    // static ct get_max_y(const box& b) { ... }
    // 
    // static box get_rect(const segment& in)
    // {
    //     return AABB_of(in);
    // }
    // static void intersect(const segment& r, const segment& l, void* user)
    // {
    //     // the code precess intersect of segment r vers segment l
    // }
  };

  template<class segment>
  class scan_line
  {
  public:
    typedef typename ::scan_line::all_function<segment> funcs;
    typedef typename funcs::box box;
    typedef typename funcs::ct ct;

    struct inner_seg
    {
      segment seg;
      // 当前的包围框
      box bbox;
    };

    struct scan_point
    {
      ct x{ 0 };

      std::vector<inner_seg*> in;
      std::vector<inner_seg*> out;
    };

    struct scan_point_compare
    {
      using is_transparent = void;
      using value = scan_point*;

      bool operator()(const value& l, const value& r) const {
        return l->x < r->x;
      };

      bool operator()(const value& l, const ct& r) const {
        return l->x < r;
      };
      bool operator()(const ct& l, const value& r) const {
        return l < r->x;
      };
    };

    void add_segment(const segment& in);
    void process(void* user);


  private:
    // 查找或新增
    scan_point* get_point(const ct& x);
    std::set<scan_point*, scan_point_compare> scan_points;
    std::vector<inner_seg*> segments;
  };

  template<class segment>
  void scan_line<segment>::add_segment(const segment& in)
  {
    auto new_seg = new inner_seg{ in, funcs::get_rect(in) };

    segments.push_back(new_seg);

    auto startp = get_point(funcs::get_min_x(new_seg->bbox));
    startp->in.push_back(new_seg);
    auto endp = get_point(funcs::get_max_x(new_seg->bbox));
    endp->out.push_back(new_seg);
  }

  template<class segment>
  void scan_line<segment>::process(void* user)
  {
    std::set<inner_seg*> current;

    for (auto p : scan_points)
    {
      for (auto in : p->in)
      {
        // 每一新添加的对象，和列表中的计算一次交点
        for (auto c : current) {
          if (funcs::intersect(c->bbox, in->bbox)) {
            funcs::intersect(c->seg, in->seg, user);
          }
        }

        current.insert(in);
      }

      for (auto out : p->out) {
        current.erase(out);
      }
    }
  }

  template<class segment>
  typename scan_line<segment>::scan_point* scan_line<segment>::scan_line::get_point(const ct& x)
  {
    scan_point* res = nullptr;

    auto search = scan_points.find(x);
    if (search == scan_points.end()) {
      res = new scan_point;
      res->x = x;
      scan_points.insert(res);
    }
    else {
      res = *search;
    }

    return res;
  }

}
