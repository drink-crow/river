#include "processor.h"
#include "dcel.h"
#include "vatti.h"
#include "util.h"

namespace river {

class processor_private{
public:
    dcel::dcel dcel;
    vatti::clipper clipper;;
};

processor::processor()
    : pptr(new processor_private)
{

}

processor::~processor()
{
    delete pptr;
    pptr = nullptr;
}

#if 0
void processor::add_line(num x1, num y1, num x2, num y2)
{
    pptr->dcel.add_line(x1, y1, x2, y2);
}



void processor::process()
{
    pptr->dcel.process();
}
#endif

#if 1

void processor::add_path(const Paths& in, PathType pt)
{
    draw_path(in);
    pptr->clipper.add_path(in, pt, false);
}

void processor::process()
{

}

#endif

}