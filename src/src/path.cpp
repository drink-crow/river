#include "river.h"
#include "segment.h"

namespace river {
  path::path(path&& l) noexcept {
    data.swap(l.data);
    element_types.swap(l.element_types);
  }

  void path::moveto(const point& tp) { 
    moveto(tp.x, tp.y);
  }

  void path::moveto(double x, double y) {
    element_types.push_back((char)seg_type::moveto);
    data.push_back(x);
    data.push_back(y);
  }

  void path::lineto(const point& tp) {
    lineto(tp.x, tp.y);
  }

  void path::lineto(double x, double y) {
    element_types.push_back((char)seg_type::lineto);
    data.push_back(x);
    data.push_back(y);
  }

  // 三点确定一个圆弧
  //void path::arcto(const point& tp, const point& middle) {
  //  element_types.push_back((char)seg_type::arcto);
  //}

  //void arcto(point const& center, double sweepRad);
  void path::cubicto(const point& ctrl1, const point& ctrl2, const point& end) {
    element_types.push_back((char)seg_type::cubicto);
    data.push_back(ctrl1.x);
    data.push_back(ctrl1.y);
    data.push_back(ctrl2.x);
    data.push_back(ctrl2.y);
    data.push_back(end.x);
    data.push_back(end.y);
  }
  // 二阶自动升三阶
  //void cubicto(const point& end, const point& ctrl);

  void empty_path_move_func(path_moveto_func_para) {}
  void empty_path_line_func(path_lineto_func_para) {}
  void empty_path_arc_func(path_arcto_func_para) {}
  void empty_path_cubic_func(path_cubicto_func_para) {}

  void path::traverse(const path_traverse_funcs& in_funcs, void* user) const
  {
    path_traverse_funcs act = in_funcs;
    if (!act.move_to) act.move_to = empty_path_move_func;
    if (!act.line_to) act.line_to = empty_path_line_func;
    //if (!act.arc_to) act.arc_to = empty_path_arc_func;
    if (!act.cubic_to) act.cubic_to = empty_path_cubic_func;

    point last_p(0, 0);
    point cur_p = last_p;

    size_t data_i = 0;
    
    for (auto& e : element_types) {
      switch (e)
      {
      case (char)seg_type::moveto:
      {
        cur_p.x = data[data_i + 0];
        cur_p.y = data[data_i + 1];
        act.move_to(last_p, cur_p, user);
        data_i += 2;
        break;
      }
      case (char)seg_type::lineto:
      {
        cur_p.x = data[data_i + 0];
        cur_p.y = data[data_i + 1];
        act.line_to(last_p, cur_p, user);
        data_i += 2;
        break;
      }
      case (char)seg_type::arcto:
      {
        //// ToDo start_sweepRad 没有正确计算
        //act.arc_to(last_p, arcto_seg->center, arcto_seg->center, cur_p, user);
        break;
      }
      case (char)seg_type::cubicto:
      {
        point ctrl1, ctrl2;
        ctrl1.x = data[data_i + 0];
        ctrl1.y = data[data_i + 1];
        ctrl2.x = data[data_i + 2];
        ctrl2.y = data[data_i + 3];
        cur_p.x = data[data_i + 4];
        cur_p.y = data[data_i + 5];
        act.cubic_to(last_p, ctrl1, ctrl2, cur_p, user);
        data_i += 6;
        break;
      }
      default:
        break;
      }

      last_p = cur_p;
    }
  }

}