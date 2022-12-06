#pragma once

#include "river.h"

#include <set>

namespace dcel {
    using namespace river;
    typedef double num;

    struct face;

    struct point
    {
        num x;
        num y;
    };

    struct point_less_operator
    {
        typedef point* key;
        bool operator()(const key& r, const key& l) const {
            if (r->x < l->x) return true;
            if (r->x == l->x) return r->y < l->y;

            return false;
        }
    };

    struct half_edge
    {
        point* start;
        point* end;

        half_edge* twin; // 孪生半边，数据相反
        face* face; // 该半边左边的面
        half_edge* next; // 同一个面下的下一个半边
        half_edge* prev; // 同一个面下的前一个半边

        set_type type = set_type::A;

        virtual seg_type get_seg_type() const = 0;
    };

    struct line_half_edge : public half_edge
    {
        virtual seg_type get_seg_type() const { return seg_type::LINETO; }
    };

    struct arc_half_edge : public half_edge
    {
        virtual seg_type get_seg_type() const { return seg_type::ARCTO; }
    };

    class dcel
    {
    public:
        void set_current_set_type(set_type);
        set_type get_current_set_type() const;

    private:
        void add_line(num x1, num y1, num x2, num y2);
        void add_arc();
        void add_cubic();

        set_type current_set_type = set_type::A;

        std::set<point*, point_less_operator> point_set;

    };

}
