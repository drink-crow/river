#include "debug_util.h"
#include "river.h"

#include <vector>
#include <string>
#include <iostream>
#include <fstream>

#include <qline.h>
#include <qpoint.h>

#include "boost/format.hpp"

using boost::format;

constexpr auto max_num = std::numeric_limits<double>::max();

struct svg_data
{
  rmath::rect rect{ rmath::vec2(max_num, max_num), rmath::vec2(-max_num, -max_num) };
  std::vector<std::string> paths;
  std::vector<std::string> nodes;
  std::string color{"blue"};

  void update_rect(const river::point& p) {
    rect.min.x = (std::min)(rect.min.x, p.x);
    rect.min.y = (std::min)(rect.min.y, p.y);
    rect.max.x = (std::max)(rect.max.x, p.x);
    rect.max.y = (std::max)(rect.max.y, p.y);
  }

  std::string get_svg() const {
    constexpr double height = 400;
    double scale = height / rect.height();
    double width = scale * rect.width();
    double stroke_w = 1 / scale;

    std::string svg(R"(<svg version="1.1" )");
    svg.append((format(R"(viewBox="%f,%f,%f,%f" )")
     % (rect.min.x * scale - 5) % (rect.min.y * scale - 5) 
     % (width + 10) % (height + 10)).str());
    svg.append((format(R"(width="%f" height="%f" )")
      % (width+10) % (height+10)).str());

    svg.append(R"(xmlns="http://www.w3.org/2000/svg")");
    svg.append(">");
    svg.append((format(R"(<g transform = "scale(%f,%f) translate(0,%f) " stroke-width="%f"> )") 
      % scale % (-scale) % (-(rect.center().y * 2)) % stroke_w).str());

    for (auto& path_ : paths) {
      svg.append(R"(<path d=")");
      svg.append(path_);
      svg.append(R"(" stroke="none" fill="red" fill-opacity="0.3" /> )");
    }

    for (auto& node_ : nodes) {
      svg.append(R"(<path d=")");
      svg.append(node_);
      svg.append((format(R"(" stroke="%s" fill="none" /> )") % color).str());
    }

    svg.append("</g>");
    svg.append(R"(</svg>)");

    return svg;
  }
};



void svg_moveto(path_moveto_func_para)
{
  auto data = (svg_data*)(user);
  auto& paths = data->paths;
  auto& nodes = data->nodes;

  data->update_rect(to);

  paths.push_back({});
  paths.back().append((format("M%f %f ") % to.x % to.y).str());
  nodes.push_back({});
  nodes.back().append((format("M%f %f ") % to.x % to.y).str());
}

void svg_lineto(path_lineto_func_para)
{
  auto data = (svg_data*)(user);
  auto& paths = data->paths;
  auto& nodes = data->nodes;

  data->update_rect(to);
  paths.back().append((format("L%f %f ") % to.x % to.y).str());
  nodes.back().append((format("L%f %f ") % to.x % to.y).str());
}

void svg_cubicto(path_cubicto_func_para)
{
  auto data = (svg_data*)(user);
  auto& paths = data->paths;
  auto& nodes = data->nodes;

  data->update_rect(ctrl1);
  data->update_rect(ctrl2);
  data->update_rect(to);
  paths.back().append((format("C%f %f %f %f %f %f ") 
    % ctrl1.x % ctrl1.y % ctrl2.x % ctrl2.y % to.x % to.y).str());
  nodes.back().append((format("L%f %f ") % ctrl1.x % ctrl1.y).str());
  nodes.back().append((format("L%f %f ") % ctrl2.x % ctrl2.y).str());
  nodes.back().append((format("L%f %f ") % to.x % to.y).str());
}

int main(int argc, char** argv)
{
  debug_util::connect();

  using namespace river;
  processor river;
  svg_data svg;
  //auto p = river::make_path("m 0 0 l 100 100 c 0 70 0 250 100 200 l 0 0");
  auto subject = river::make_path(
    "m 30 96 c 32 89 24 78 10 60 c 16 60 40 87 39 91 c 39 94 31 98 30 96"
    "m 25 75 c 31 57 21 51 29 44 c 32 41 34 74 30 78"
    "m 47 96 c 51 63 47 58 53 54 c 62 49 83 53 83 57 c 81 72 80 75 79 69 "
            "c 77 61 72 54 56 59 c 51 61 53 81 56 92"
    "m 70 91 c 69 85 57 75 36 63 c 41 61 54 68 77 85");
  auto clip = river::make_path(
    "m 16 32 c 40 36 65 38 91 38 l 84 43 c 59 42 33 40 06 36"
    "m 42 52 c 48 40 42 20 47 02 c 52 15 48 42 52 50");
  river.add_path(subject, path_type::subject);
  river.add_path(clip, path_type::clip);

  path_traverse_funcs write_svg;
  write_svg.move_to = svg_moveto;
  write_svg.line_to = svg_lineto;
  write_svg.cubic_to = svg_cubicto;

  river.process(clip_type::union_, fill_rule::positive, write_svg, &svg);

  std::fstream svg_file("river_test.svg", std::ios_base::out | std::ios_base::trunc);
  svg_file << svg.get_svg();
  svg_file.close();

  debug_util::disconnect();
  return 0;
}
