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
  auto subject = river::make_path("m 0 10 l 100 0 l 110 20 l 110 80 l 100 100 l 0 90 l 0 10"
    "m 120 100 c 70 70 150 70 70 110 l 120 100"
    "m 160 120 c 70 70 100 40 130 70 l 160 120");
  auto clip = river::make_path("m 50 30 l 110 20 l 150 30 l 150 70 l 110 80 l 50 70 l 50 30"
    "m 30 40 l 50 40 l 50 60 l 30 60 l 30 40"
    "m 120 120 c 150 30 20 -10 40 20 l 120 120");
  river.add_path(subject, PathType::Subject);
  river.add_path(clip, PathType::Clip);
  Paths solution;
  river.process(clip_type::intersection, fill_rule::positive, solution);

  debug_util::disconnect();
  return 0;
}
