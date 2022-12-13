#pragma once

#include "river.h"
#include "rmath.h"

#include <set>
#include <vector>

#include "boost/geometry.hpp"

#include "glm/glm.hpp"

namespace dcel {
    using namespace river;
    typedef double num;
    using namespace rmath;

    struct half_edge;
    struct face;
    class dcel;

    struct point
    {
        point(num x, num y) : p(x,y){};

        rmath::vec2 p;

        std::vector<half_edge*> edges;
    };

    bool vec2_compare_func(const ::rmath::vec2& r, const ::rmath::vec2& l);

    struct point_less_operator
    {
        using is_transparent = void;

        typedef point* key;
        bool operator()(const key& r, const key& l) const {
            return vec2_compare_func(r->p, l->p);
        }
        bool operator()(const key& l, const vec2& r) const {
            return vec2_compare_func(l->p, r);
        };
        bool operator()(const vec2& l, const key& r) const {
            return vec2_compare_func(l, r->p);
        };
    };

    struct vec2_compare
    {
        bool operator()(const ::rmath::vec2& r, const ::rmath::vec2& l) const {
            return vec2_compare_func(r, l);
        }
    };

    struct half_edge
    {
        point* start = nullptr;
        point* end = nullptr;

        half_edge* twin = nullptr; // 孪生半边，数据相反
        face* face = nullptr; // 该半边左边的面
        half_edge* next = nullptr; // 同一个面下的下一个半边
        half_edge* prev = nullptr; // 同一个面下的前一个半边

        set_type type = set_type::A;

        virtual ~half_edge() = default;

        virtual seg_type get_seg_type() const = 0;
        virtual ::rmath::rect get_boundary() const = 0;

        void add_break_point(const ::rmath::vec2& bp) {
            break_points.insert(bp);
        }
        auto& get_break_points() const {
            return break_points;
        }

        // 通过记录的断点，插入新的 edge 到 decl 中，本体的移除稍后又 decl 进行
        virtual std::vector<half_edge*> process_break_point(dcel* parent) { return {}; }

    private:
        std::set<::rmath::vec2, vec2_compare> break_points;
    };

    struct line_half_edge : public half_edge
    {
        virtual seg_type get_seg_type() const override { return seg_type::LINETO; }
        virtual ::rmath::rect get_boundary() const override {
            return ::rmath::rect();
        }
        virtual std::vector<half_edge*> process_break_point(dcel* parent) override;
    };

    struct arc_half_edge : public half_edge
    {
        virtual seg_type get_seg_type() const override { return seg_type::ARCTO; }
        virtual ::rmath::rect get_boundary() const override {
            return ::rmath::rect();
        }
    };

    bool point_between_two_point(const vec2& p, const vec2& p0, const vec2& p1);

    void intersect(line_half_edge* l, line_half_edge* r);

    class dcel
    {
    public:
        void set_current_set_type(set_type);
        set_type get_current_set_type() const;

        void add_line(num x1, num y1, num x2, num y2);
        void add_arc();
        void add_cubic();

        void process_break();

        line_half_edge* new_line_edge(point* start, point* end);

        point* find_or_add_point(num x, num y);

        set_type current_set_type = set_type::A;

        std::set<point*, point_less_operator> point_set;
        std::set<half_edge*> edges;
    };

}
