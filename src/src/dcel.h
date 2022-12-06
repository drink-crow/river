#pragma once

#include "river.h"

#include <set>

namespace dcel {

struct face;

struct point
{
    double x;
    double y;
};

struct point_less_operator
{
    typedef point* key;
    bool operator()(const key &r, const key &l) const{
        if( r->x < l->x) return true;
        if( r->x == l->x) return r->y < l->y;

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
};

class dcel
{
public:

private:
    void add_line();
    void add_arc();
    void add_cubic();

    std::set<point*, point_less_operator> point_set;
};

}
