#pragma once

#include <type_traits>
#include <vector>
#include "rmath.h"

namespace river {
  enum class path_type
  {
    subject, clip
  };

  enum class fill_rule { even_odd, non_zero, positive, negative };
  enum class clip_type { none, intersection, union_, difference, xor_ };

  using point = rmath::vec2;
  using num = point::num;
  using rmath::rect;

  struct segment;

#define path_moveto_func_para const point& from, const point& to, void* user
#define path_lineto_func_para const point& from, const point& to, void* user
#define path_arcto_func_para const point& from, const point& center, const point& start_sweepRad, const point& to, void* user
#define path_cubicto_func_para const point& from, const point& ctrl1, const point& ctrl2, const point& to, void* user
  typedef void (*path_moveto_func)(path_moveto_func_para);
  typedef void (*path_lineto_func)(path_lineto_func_para);
  typedef void (*path_arcto_func)(path_arcto_func_para);
  typedef void (*path_cubicto_func)(path_cubicto_func_para);
  struct path_traverse_funcs
  {
    path_moveto_func   move_to = nullptr;
    path_lineto_func   line_to = nullptr;
    path_arcto_func    arc_to = nullptr;
    path_cubicto_func  cubic_to = nullptr;
  };

  /* 该 path 结构只是用来做简单的数据交换，使用者不应该使用该结构做任何复杂的操作,
  *  不应擅自修改其中的数据, 程序中也不会对 path 中的数据进行额外的检查.
  *  
  * this 'path' stuct uesd for simple data exchange for this library. you 
  * should NOT modify the data in ths struct directly. Program would NOT do any
  * extra exam for this data.
  */
  struct path
  {
    std::vector<num> data;
    std::vector<char> element_types;

    path() = default;
    path(path&& l) noexcept;

    void moveto(const point& tp);
    void moveto(double x, double y);
    void lineto(const point& tp);
    void lineto(double x, double y);
    // 三点确定一个圆弧
    //void arcto(const point& tp, const point& middle);
    //void arcto(point const& center, double sweepRad);
    void cubicto(const point& ctrl1, const point& ctrl2, const point& end);
    // 二阶自动升三阶
    //void cubicto(const point& end, const point& ctrl);

    // 遍历所有元素，函数指针为空是安全的，会忽略对应内容
    void traverse(const path_traverse_funcs& funcs, void* user) const;
  };

  typedef void (*read_path_func)(const void* path, 
    path_traverse_funcs segment_funcs, void* user);

  using paths = std::vector<path>;

  class processor_private;
  class processor
  {
  public:
    typedef double num;

    processor();
    ~processor();

    void add_path(const paths& in, path_type pt);
    void add_path(read_path_func read_func, const void* path, path_type pt);
    void process(clip_type operation, fill_rule fill_rule, paths& output);
    void process(clip_type operation, fill_rule fill_rule,
      path_traverse_funcs write_func, void* output);

  private:
    processor_private* pptr;
  };

  paths make_path(const char* s);
}
