#include "debug_util.h"
#include "river.h"
#include <qline.h>
#include <qpoint.h>

int main(int argc, char** argv)
{
  debug_util::connect();

  using namespace river;
  processor river;

  //auto p = river::make_path("m 0 0 l 100 100 c 0 70 0 250 100 200 l 0 0");
  auto subject = river::make_path(
    "m 18 15.2 l 5.1 15.2 c 4 15.2 3 15 2.3 14.6 c 1.7 14.2 0.9 13.4 0 12 l 1 11 c 1.5 12 2 12.5 2.5 12.7 c 3 13 3.5 13 4.3 13 l 18 13");
  auto clip = river::make_path(
    "m 5 13 l 5 11 c 5 6.8 4.5 3.5 4 1 l 4 0 l 7 0 l7 13"
    "m 12 13 l 12 4 c 12 2.5 12 1.5 12.5 1 c 13 0.3 13.6 0 14.5 0 c 15.6 0 16.7 0.5 18 1.5 l 17.3 2.4 c 16.6 2 16 1.7 15.7 1.7 c 15 1.6 14.5 2.3 14.5 3.7 l 14.5 13");
  river.add_path(subject, path_type::subject);
  river.add_path(clip, path_type::clip);
  paths solution;
  river.process(clip_type::union_, fill_rule::positive, solution);

  debug_util::disconnect();
  return 0;
}
