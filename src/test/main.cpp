#include "debug_util.h"
#include "river.h"
#include <qline.h>
#include <qpoint.h>


int main(int argc, char** argv)
{
    debug_util::connect();

    river::processor river;
    river.add_line(0,0,100,0);
    river.add_line(100,0,100,100);
    river.add_line(100,100,0,100);
    river.add_line(0,100,0,0);

    river.add_line(0, 20, 100, 20);
    river.add_line(100, 20, 100, 80);
    river.add_line(100, 80, 0, 80);
    river.add_line(0, 80, 0, 20);

    river.add_line(0, 0, 50, 100);
    river.add_line(50, 100, 100, 0);
    river.add_line(100, 0, 0, 0);

    river.process();

    debug_util::disconnect();
    return 0;
}
