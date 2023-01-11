#pragma once

#include <type_traits>
#include <vector>
#include "rmath.h"

namespace river {

  enum class seg_type : int
  {
    moveto = 0,
    lineto = 1,
    arcto = 2,
    cubicto = 3
  };

  enum class path_type
  {
    subject, clip
  };

  enum class fill_rule { even_odd, non_zero, positive, negative };
  enum class clip_type { none, intersection, union_, difference, xor_ };

  using point = rmath::vec2;
  using rmath::rect;
  struct segment
  {
    virtual const point& get_target() const = 0;
    virtual seg_type get_type() const = 0;
    virtual segment* deep_copy() const = 0;
    virtual rect get_boundary(const point& from) const = 0;
    virtual void reverse(const point& from) = 0;
    virtual double curr_x(const point& from, double y) const = 0;
  };

  struct seg_moveto : public segment
  {
    point target;

    seg_moveto(const point& p) :target(p) {}
    virtual const point& get_target() const override;
    virtual seg_type get_type() const override;
    virtual rect get_boundary(const point& from) const { return rect(target); };
    virtual segment* deep_copy() const override {
      return new seg_moveto{ target };
    }
    virtual void reverse(const point& from) override {
      target = from;
    }
    virtual double curr_x(const point& from, double y) const override { return target.x; }
  };

  struct seg_lineto : public segment
  {
    point target;

    seg_lineto(const point& p) :target(p) {}
    const point& get_target() const override;
    seg_type get_type() const override;
    virtual rect get_boundary(const point& from) const { return rect::from({ target, from }); };
    virtual segment* deep_copy() const override {
      return new seg_lineto{ target };
    }
    virtual void reverse(const point& from) override {
      target = from;
    }
    virtual double curr_x(const point& from, double y) const override {
      return (target.x - from.x) * (y - from.y) / (target.y - from.y) + from.x;
    }
  };

  struct seg_arcto : public segment
  {
    point target;
    point center;
    bool longarc;

    seg_arcto(const point& _target, const point& c, bool _longarc) :
      target(_target), center(c), longarc(_longarc) { }
    // ToDo 没有正确计算
    seg_arcto(const point& mid, const point& end) : target(end) { }
    const point& get_target() const override;
    seg_type get_type() const override;
    // ToDo 需要正确计算包围框
    virtual rect get_boundary(const point& from) const { return rect::from({ target, from }); };

    virtual segment* deep_copy() const override {
      return new seg_arcto{ target,center,longarc };
    }
    virtual void reverse(const point& from) override {
      target = from;
    }
    virtual double curr_x(const point& from, double y) const override {
      // ToDo 需要修正计算
      return (target.x - from.x) * (y - from.y) / (target.y - from.y) + from.x;
    }
  };

  struct seg_cubicto : public segment
  {
    point ctrl1;
    point ctrl2;
    point target;

    seg_cubicto(const point& ctrl1, const point& ctrl2, const point& p) :
      ctrl1(ctrl1), ctrl2(ctrl2), target(p) {}
    rmath::bezier_cubic get_cubic(const point& from) const {
      return rmath::bezier_cubic{ from, ctrl1, ctrl2, target };
    }
    const point& get_target() const override;
    seg_type get_type() const override;
    virtual rect get_boundary(const point& from) const {
      return get_cubic(from).get_boundary();
    };
    virtual segment* deep_copy() const override {
      return new seg_cubicto{ ctrl1,ctrl2,target };
    }
    virtual void reverse(const point& from) override {
      target = from;
      std::swap(ctrl1, ctrl2);
    }
    virtual double curr_x(const point& from, double y) const override;
  };

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

  struct path
  {
    std::vector<segment*> data;

    path() = default;
    path(path&& l) noexcept {
      data.swap(l.data);
    }

    ~path() { for (auto segment : data) delete segment; }
    void moveto(const point& tp) { data.push_back(new seg_moveto(tp)); }
    void moveto(double x, double y) { data.push_back(new seg_moveto({ x,y })); }
    void lineto(const point& tp) { data.push_back(new seg_lineto(tp)); }
    void lineto(double x, double y) { data.push_back(new seg_lineto({ x,y })); }
    // 三点确定一个圆弧
    void arcto(const point& tp, const point& middle) { data.push_back(new seg_arcto(middle, tp)); }
    //void arcto(point const& center, double sweepRad);
    void cubicto(const point& ctrl1, const point& ctrl2, const point& end) { data.push_back(new seg_cubicto(ctrl1, ctrl2, end)); }
    // 二阶自动升三阶
    //void cubicto(const point& end, const point& ctrl);

    // 遍历所有元素，函数指针为空是安全的，会忽略对应内容
    void traverse(const path_traverse_funcs& funcs, void* user) const;
  };

  using paths = std::vector<path>;

  class processor_private;
  class processor
  {
  public:
    typedef double num;

    processor();
    ~processor();

    void add_line(num x1, num y1, num x2, num y2);
    void add_path(const paths& in, path_type pt);

    void process(clip_type operation, fill_rule fill_rule, paths& output);

  private:
    processor_private* pptr;
  };

  paths make_path(const char* s);

}
