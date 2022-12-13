#include "scan_line.h"

namespace scan_line
{
   void scan_line::add_segment(const key& in)
    {
        auto new_seg = new seg{in, funcs::get_rect(in) };
        
        segments.push_back(new_seg);

        auto startp = get_point(bg::get<bg::min_corner, 1>(new_seg->box));
        startp->in.push_back(new_seg);
        auto endp = get_point(bg::get<bg::max_corner, 1>(new_seg->box));
        endp->out.push_back(new_seg);
    }

    void scan_line::process()
    {
        std::set<seg*> current;

        for (auto p : scan_points)
        {
            for (auto in : p->in)
            {
                // 每一新添加的对象，和列表中的计算一次交点
                for (auto c : current) {
                    if (intersect(c->box, in->box)) {
                        funcs::intersect(c->key, in->key);
                    }
                }

                current.insert(in);
            }

            for (auto out : p->out) {
                current.erase(out);
            }
        }
    }

    bool scan_line::intersect(const box& r, const box& l) const
    {
        return bg::intersects(r,l);
    }

    scan_line::scan_point* scan_line::scan_line::get_point(const ct& y)
    {
        scan_point* res = nullptr;

        auto search = scan_points.find(y);
        if (search == scan_points.end()) {
            res = new scan_point;
            res->y = y;
            scan_points.insert(res);
        }
        else {
            res = *search;
        }

        return res;
    }
}
