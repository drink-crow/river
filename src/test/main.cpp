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
    svg.append((format(R"(viewBox="-5,-5,%f,%f" )")
      % (width+10) % (height+10)).str());
    svg.append((format(R"(width="%f" height="%f" )")
      % (width) % (height)).str());

    svg.append(R"(xmlns="http://www.w3.org/2000/svg")");
    svg.append(">");
    svg.append((format(R"(<g transform = "scale(%f,%f) translate(0,%f) " 
        stroke-width="%f"> )") 
      % scale % (-scale) % (-rect.height()) % stroke_w).str());

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
    "m 18 15.2 l 5.1 15.2 c 4 15.2 3 15 2.3 14.6 c 1.7 14.2 0.9 13.4 0 12 l 1 11 c 1.5 12 2 12.5 2.5 12.7 c 3 13 3.5 13 4.3 13 l 18 13");
  auto clip = river::make_path(
    "m 5 13 l 5 11 c 5 6.8 4.5 3.5 4 1 l 4 0 l 7 0 l7 13"
    "m 12 13 l 12 4 c 12 2.5 12 1.5 12.5 1 c 13 0.3 13.6 0 14.5 0 c 15.6 0 16.7 0.5 18 1.5 l 17.3 2.4 c 16.6 2 16 1.7 15.7 1.7 c 15 1.6 14.5 2.3 14.5 3.7 l 14.5 13");
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
