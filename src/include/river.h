#pragma once

#include <type_traits>
#include <vector>
#include "rmath.h"

namespace river {

enum class SegType : int
{
    MoveTo,
    LineTo,
    ArcTo,
    CubicTo
};

enum class PathType
{
    Subject, Clip
};


using Point = rmath::vec2;

    struct Seg
    {
        virtual const Point& get_target() const = 0;
        virtual SegType get_type() const = 0;
        virtual Seg* deep_copy() const = 0;
    };

    struct Seg_moveto : public Seg
    {
        Point target;

        Seg_moveto(const Point& p) :target(p) {}
        virtual const Point& get_target() const override;
        virtual SegType get_type() const override;
        virtual Seg* deep_copy() const override {
            return new Seg_moveto{ target };
        }
    };

    struct Seg_lineto : public Seg
    {
        Point target;

        Seg_lineto(const Point& p) :target(p) {}
        const Point & get_target() const override;
        SegType get_type() const override;
        virtual Seg* deep_copy() const override {
            return new Seg_lineto{ target };
        }
    };

    struct Seg_arcto : public Seg
    {
        Point target;
        Point center;
        bool longarc;

        Seg_arcto(const Point& _target, const Point& c, bool _longarc) :
            target(_target), center(c), longarc(_longarc) { }
        // ToDo 没有正确计算
        Seg_arcto(const Point& mid, const Point& end) : target(end) { }
        const Point & get_target() const override;
        SegType get_type() const override;
        virtual Seg* deep_copy() const override {
            return new Seg_arcto{ target,center,longarc };
        }
    };

    struct Seg_cubicto : public Seg
    {
        Point ctrl_Point1;
        Point ctrl_Point2;
        Point target;

        Seg_cubicto(const Point& ctrl1, const Point& ctrl2, const Point& p) :
            ctrl_Point1(ctrl1), ctrl_Point2(ctrl2), target(p) {}
        const Point & get_target() const override;
        SegType get_type() const override;
        virtual Seg* deep_copy() const override {
            return new Seg_cubicto{ ctrl_Point1,ctrl_Point2,target };
        }
    };

#define path_moveto_func_para const Point& from, const Point& to, void* user
#define path_lineto_func_para const Point& from, const Point& to, void* user
#define path_arcto_func_para const Point& from, const Point& center, const Point& start_sweepRad, const Point& to, void* user
#define path_cubicto_func_para const Point& from, const Point& ctrl1, const Point& ctrl2, const Point& to, void* user
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

    struct Path
    {
        std::vector<Seg*> data;

        Path() = default;
        Path(Path&& l) noexcept {
            data.swap(l.data);
        } 

        ~Path() { for (auto Seg : data) delete Seg; }
        void moveto(const Point& tp) { data.push_back(new Seg_moveto(tp)); }
        void moveto(double x, double y) { data.push_back(new Seg_moveto({ x,y })); }
        void lineto(const Point& tp) { data.push_back(new Seg_lineto(tp)); }
        void lineto(double x, double y) { data.push_back(new Seg_lineto({x,y})); }
        // 三点确定一个圆弧
        void arcto(const Point& tp, const Point& middle) { data.push_back(new Seg_arcto(middle, tp)); }
        //void arcto(Point const& center, double sweepRad);
        void cubicto(const Point& ctrl1, const Point& ctrl2,const Point& end) { data.push_back(new Seg_cubicto(ctrl1, ctrl2, end)); }
        // 二阶自动升三阶
        //void cubicto(const Point& end, const Point& ctrl);

        // 遍历所有元素，函数指针为空是安全的，会忽略对应内容
        void traverse(const path_traverse_funcs& funcs, void* user) const;
    };

    using Paths = std::vector<Path>;


namespace traits {
    struct unknow_tag {};
    struct Point_tag {};
    struct line_tag {};
    struct arc_tag {};
    struct cubic_tag {};
    struct box_tag {};
    struct Point_type {};

    struct min_corner {};
    
    
    template<typename T> struct tag{};
    template<typename T> struct coordinate_type{};
    template<typename T, int i> struct access {};

    template<>
    struct tag<Point>
    {
        typedef Point_tag type ;
    };

    template<>
    struct coordinate_type<Point>
    {
        typedef double type ;
    };

    template<>
    struct access<Point, 0>
    {
        static inline double get(const Point& p)
        {
            return p.x;
        }
        static inline void set(Point& p, const double& value)
        {
            p.x = value;
        }
    };

    template<>
    struct access<Point, 1>
    {
        static inline double get(const Point& p)
        {
            return p.y;
        }
        static inline void set(Point& p, const double& value)
        {
            p.y = value;
        }
    };
}

class processor_private;
class processor
{
public:
    typedef double num;

    processor();
    ~processor();

    void add_line(num x1, num y1, num x2, num y2);
    void add_path(const Paths& in, PathType pt);

    void process();

private:
    processor_private* pptr;
};

Paths make_path(const char* s);

}
