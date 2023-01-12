#pragma once
#include "river.h"

namespace river {
  enum class seg_type : int
  {
    moveto = 0,
    lineto = 1,
    arcto = 2,
    cubicto = 3
  };

  struct segment
  {
    virtual const point& get_target() const = 0;
    virtual seg_type get_type() const = 0;
    virtual void copy(void* in_memory) const = 0;
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
    virtual void copy(void* in_memory) const override {
      ((seg_moveto*)in_memory)->seg_moveto::seg_moveto(target);
    }
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
    virtual void copy(void* in_memory) const override {
      ((seg_lineto*)in_memory)->seg_lineto::seg_lineto(target);
    }
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
    virtual rect get_boundary(const point& from) const {
      return rect::from({ target, from }); };

    virtual void copy(void* in_memory) const override {
      ((seg_arcto*)in_memory)->seg_arcto::seg_arcto(target, center, longarc);
    }
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
    virtual void copy(void* in_memory) const override {
      ((seg_cubicto*)in_memory)->seg_cubicto::seg_cubicto(ctrl1, ctrl2, target);
    }
    virtual segment* deep_copy() const override {
      return new seg_cubicto{ ctrl1,ctrl2,target };
    }
    virtual void reverse(const point& from) override {
      target = from;
      std::swap(ctrl1, ctrl2);
    }
    virtual double curr_x(const point& from, double y) const override;
  };
}
