#include "river.h"
#include "dcel.h"

namespace river {

class processor_private{
public:
    dcel::dcel dcel;
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

void processor::add_line(num x1, num y1, num x2, num y2)
{
    pptr->dcel.add_line(x1, y1, x2, y2);
}

void processor::process()
{
    pptr->dcel.process_break();
}

}