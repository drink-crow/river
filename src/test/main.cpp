#include "debug_util.h"
#include "river.h"
#include <qline.h>
#include <qpoint.h>

#include "clipper2/clipper.h"

int main(int argc, char** argv)
{
    debug_util::connect();

    using namespace river;
    processor river;

#if 1
    //auto p = river::make_path("m 0 0 l 100 100 c 0 70 0 250 100 200 l 0 0");
    auto subject = river::make_path("m 0 10 l 100 0 l 110 20 l 110 80 l 100 100 l 0 90 l 0 10"
        "m 120 100 c 70 70 180 0 120 0 l 120 100");
    auto clip = river::make_path("m 50 30 l 110 20 l 150 30 l 150 70 l 110 80 l 50 70 l 50 30"
        "m 30 40 l 50 40 l 50 60 l 30 60 l 30 40");
    river.add_path(subject, PathType::Subject);
    river.add_path(clip, PathType::Clip);
    Paths solution;
    river.process(clip_type::intersection, fill_rule::positive, solution);
#endif

#if 0
    {
        using namespace Clipper2Lib;
        Paths64 subject, clip, solution;
        //subject.push_back(MakePath("0, 0, 100, 0, 110, 20, 110, 80, 100, 100, 0, 100"));
        //clip.push_back(MakePath("50, 30, 110, 20, 150, 30, 150, 70, 110, 80, 50, 70"));

        subject.push_back(MakePath("0, 50, 50, 0, 100, 50, 50, 100"));
        clip.push_back(MakePath("100, 50, 90, 40, 90, 30, 100, 20, 120, 90, 60, 150, 30, 120"));
        //clip.push_back(MakePath("10, 60, 40, 90, 10, 120, -20, 90"));

        for (auto& path : subject)
        {
            auto last = path.back();
            for (auto& p : path) {
                debug_util::show_line(QLineF(last.x, last.y, p.x, p.y));
                last = p;
            }
        }
        QPen blue_pen(Qt::blue);
        for (auto& path : clip)
        {
            auto last = path.back();
            for (auto& p : path) {
                debug_util::show_line(QLineF(last.x, last.y, p.x, p.y), blue_pen);
                last = p;
            }
        }

        solution = Union(subject, clip, FillRule::Positive);

        QPen redpen(Qt::red);
        for(auto& path:solution)
        {
            auto last = path.back();
            for(auto& p : path){
                debug_util::show_line(QLineF(last.x,last.y,p.x,p.y ), redpen);
                last = p;
            }
        }
    }
#endif

    debug_util::disconnect();
    return 0;
}
