#include "river.h"

namespace river
{
    const Point&  Seg_moveto::get_target() const {
        return target;
    }

    SegType Seg_moveto::get_type() const
    {
        return SegType::MoveTo;
    }

    const Point& Seg_lineto::get_target() const {
        return target;
    }

    SegType Seg_lineto::get_type() const
    {
        return SegType::LineTo;
    }

    const Point& Seg_arcto::get_target() const {
        return target;
    }

    SegType Seg_arcto::get_type() const
    {
        return SegType::ArcTo;
    }

    const Point& Seg_cubicto::get_target() const {
        return target;
    }

    SegType Seg_cubicto::get_type() const
    {
        return SegType::CubicTo;
    }
}
