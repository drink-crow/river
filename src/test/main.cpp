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

#if 0
    //river.add_line(0,0,100,0);
    //river.add_line(100,0,100,100);
    //river.add_line(100,100,0,100);
    //river.add_line(0,100,0,0);

    //river.add_line(0, 20, 100, 20);
    //river.add_line(100, 20, 100, 80);
    //river.add_line(100, 80, 0, 80);
    //river.add_line(0, 80, 0, 20);

    //river.add_line(0, 0, 50, 100);
    //river.add_line(50, 100, 100, 0);
    //river.add_line(100, 0, 0, 0);

    std::vector<double> A{
        0,0,
        100,100,
        50,150,
        0,100,
        100,0,
        0,0,
    };

    std::vector<double> B{
        0,150,
        30,100,
        20,50,
        70,-30,
        70,120,
        0,150
    };

    constexpr auto add_path = [](const std::vector<double>& path, river::processor* r) {
        for (size_t i = 3; i < path.size(); i += 2)
        {
            r->add_line(path[i - 3], path[i - 2], path[i - 1], path[i - 0]);
        }
    };

    add_path(A, &river);
    add_path(B, &river);
    river.process();
#endif

    //auto p = river::make_path("m 0 0 l 100 100 c 0 70 0 250 100 200 l 0 0");
    auto subject = river::make_path("m 0 10 l 100 0 l 110 20 l 110 80 l 100 100 l 0 90 l 0 10");
    auto clip = river::make_path("m 50 30 l 110 20 l 150 30 l 150 70 l 110 80 l 50 70 l 50 30");
    river.add_path(subject, PathType::Subject);
    river.add_path(clip, PathType::Clip);
    river.process();
#if 0
    {
        using namespace Clipper2Lib;
        Paths64 subject, clip, solution;
        subject.push_back(MakePath("0, 0, 100, 0, 110, 20, 110, 80, 100, 100, 0, 100"));
        clip.push_back(MakePath("50, 30, 110, 20, 150, 30, 150, 70, 110, 80, 50, 70"));

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

        solution = Intersect(subject, clip, FillRule::Positive);

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
