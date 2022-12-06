#include "dcel.h"

namespace dcel {

    void dcel::set_current_set_type(set_type new_type)
    {
        current_set_type = new_type;
    }

    set_type dcel::get_current_set_type() const
    {
        return current_set_type;
    }

    void dcel::add_line(num x1, num y1, num x2, num y2)
    {
        point tmp_p;

        point* start_p = nullptr;
        point* end_p = nullptr;

        tmp_p.x = x1; tmp_p.y = y1;
        auto it = point_set.find(&tmp_p);
        if (it == point_set.end()) {
            start_p = new point;
            point_set.insert(start_p);
        }
        else {
            start_p = *it;
        }

        tmp_p.x = x2; tmp_p.y = y2;
        auto it = point_set.find(&tmp_p);
        if (it == point_set.end()) {
            end_p = new point;
            point_set.insert(end_p);
        }
        else {
            end_p = *it;
        }


    }

}
