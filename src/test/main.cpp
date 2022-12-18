#include "debug_util.h"
#include "river.h"
#include <qline.h>
#include <qpoint.h>


int main(int argc, char** argv)
{
    debug_util::connect();

    river::processor river;
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

    debug_util::disconnect();
    return 0;
}
