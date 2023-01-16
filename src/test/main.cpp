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
  enum class path_type {
    subject, clip, output
  };
  typedef river::point point;

  rmath::rect rect{ point(max_num, max_num), point(-max_num, -max_num) };
  std::vector<std::string> paths;
  std::vector<path_type> path_types;
  path_type curr_type = path_type::subject;

  std::vector<std::string> nodes;
  std::vector<point> nodes_pt;
  std::vector<std::string> hints;
  std::string color{"blue"};
  river::fill_rule fill_rule_ = river::fill_rule::positive;

  void update_rect(const point& p) {
    rect.min.x = (std::min)(rect.min.x, p.x);
    rect.min.y = (std::min)(rect.min.y, p.y);
    rect.max.x = (std::max)(rect.max.x, p.x);
    rect.max.y = (std::max)(rect.max.y, p.y);
  }

  void add_node(const point& p) {
    nodes_pt.push_back(p);
  }

  void hints_add_line(const point& l0, const point& l1) {
    hints.push_back((format(R"(<line x1="%f" y1="%f" x2="%f" y2="%f" stroke="blue"/>)")
      % l0.x % l0.y % l1.x % l1.y).str());
  }

  std::string get_svg() const {
    constexpr double height = 400;
    double scale = height / rect.height();
    double width = scale * rect.width();
    double stroke_w = 1 / scale;

    std::string svg(R"(<svg version="1.1" )");
    svg.append((format(R"(viewBox="%f,%f,%f,%f" )")
     % rect.min.x % rect.min.y % rect.width() % rect.height()).str());
    svg.append((format(R"(width="%f" height="%f" )")
      % (width+10) % (height+10)).str());

    svg.append(R"(xmlns="http://www.w3.org/2000/svg")");
    svg.append(">");
    svg.append((format(
      R"(<g transform = "scale(%f,%f) translate(0,%f) " stroke-width="%f"> )") 
      % 1 % -1 % (-(rect.center().y * 2)) % stroke_w).str());

    auto t_it = path_types.begin();
    for (auto& path_ : paths) {
      std::string prop;
      switch (*t_it)
      {
      case path_type::subject:
        prop = R"(" stroke="green" fill="green" fill-opacity="0.2" )";
        break;
      case path_type::clip:
        prop = R"(" stroke="blue" fill="blue" fill-opacity="0.2" )";
        break;
      case path_type::output:
        prop = R"(" stroke="black" fill="tomato" )";
        break;
      default:
        break;
      }

      svg.append(R"(<path d=")");
      svg.append(path_);
      svg.append(prop);
      svg.append( " /> ");

      ++t_it;
    }

    for (auto& p : nodes_pt) {
      auto r = rmath::rect(p, 2 / scale);
      svg.append( (format(R"(<rect x="%f" y="%f" width="%f" height="%f" )"
        R"(fill="blue" />)")
        % r.min.x % r.min.y % r.width() % r.height()).str());
    }

    for (auto& hint : hints) {
      svg.append(hint);
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

  data->path_types.push_back(data->curr_type);
  if (!paths.empty()) paths.back().append(" Z ");
  paths.push_back({});
  paths.back().append((format("M%f %f ") % to.x % to.y).str());
  if(data->curr_type == svg_data::path_type::output) {
    data->add_node(to);
  }
}

void svg_lineto(path_lineto_func_para)
{
  auto data = (svg_data*)(user);
  auto& paths = data->paths;
  auto& nodes = data->nodes;

  data->update_rect(to);
  paths.back().append((format("L%f %f ") % to.x % to.y).str());
  if (data->curr_type == svg_data::path_type::output) {
    data->add_node(to);
  }
}

void svg_cubicto(path_cubicto_func_para)
{
  auto data = (svg_data*)(user);
  auto& paths = data->paths;
  auto& nodes = data->nodes;
  auto& hints = data->hints;

  data->update_rect(ctrl1);
  data->update_rect(ctrl2);
  data->update_rect(to);
  paths.back().append((format("C%f %f %f %f %f %f ") 
    % ctrl1.x % ctrl1.y % ctrl2.x % ctrl2.y % to.x % to.y).str());
  if (data->curr_type == svg_data::path_type::output) {
    data->add_node(ctrl1);
    data->add_node(ctrl2);
    data->add_node(to);
    data->hints_add_line(from, ctrl1);
    data->hints_add_line(ctrl2, to);
  }
}

void read_input(read_path_func_para) {
  auto paths_ = (const river::paths*)(paths);
  for (auto& p : *paths_) {
    p.traverse(segment_funcs, user);
  }
}

std::string svg_process(
  river::read_path_func read_subject_func, const void* subject_path, 
  river::read_path_func read_clip_func, const void* clip_path,
  river::clip_type operation, river::fill_rule fill_rule)
{
  using namespace river;

  processor river;
  svg_data svg;
  path_traverse_funcs write_svg;
  write_svg.move_to = svg_moveto;
  write_svg.line_to = svg_lineto;
  write_svg.cubic_to = svg_cubicto;


  svg.curr_type = svg_data::path_type::subject;
  read_subject_func(subject_path, write_svg, &svg);
  river.add_path(read_subject_func, subject_path, path_type::subject);

  svg.curr_type = svg_data::path_type::clip;
  read_clip_func(clip_path, write_svg, &svg);
  river.add_path(read_clip_func, clip_path, path_type::clip);


  svg.curr_type = svg_data::path_type::output;
  river.process(operation, fill_rule, write_svg, &svg);
  return svg.get_svg();
}

int main(int argc, char** argv)
{
  using namespace river;
  using namespace rmath;

  debug_util::connect();


  auto subject = make_path(
    "m 30 96 c 32 89 24 78 10 60 c 16 60 40 87 39 91 c 39 94 31 98 30 96"
    "m 25 75 c 31 57 21 51 29 44 c 32 41 34 74 30 78"
    "m 47 96 c 51 63 47 58 53 54 c 62 49 83 53 83 57 c 81 72 80 75 79 69 "
            "c 77 61 72 54 56 59 c 51 61 53 81 56 92"
    "m 70 91 c 69 85 57 75 36 63 c 41 61 54 68 77 85"
    "m 16 32 c 40 36 65 38 91 38 l 84 43 c 59 42 33 40 06 36"
    "m 42 52 c 48 40 42 20 47 02 c 52 15 48 42 52 50"
  );
  bezier_cubic b[4];
  bezier_cubic::circle(vec2(48.5, 50), 35, b);

  auto clip = river::make_path(
    (format("m %f %f c %f %f %f %f %f %f c %f %f %f %f %f %f "
      "c %f %f %f %f %f %f c %f %f %f %f %f %f")
      % b[0].p0.x % b[0].p0.y
      % b[0].p1.x % b[0].p1.y % b[0].p2.x % b[0].p2.y % b[0].p3.x % b[0].p3.y
      % b[1].p1.x % b[1].p1.y % b[1].p2.x % b[1].p2.y % b[1].p3.x % b[1].p3.y
      % b[2].p1.x % b[2].p1.y % b[2].p2.x % b[2].p2.y % b[2].p3.x % b[2].p3.y
      % b[3].p1.x % b[3].p1.y % b[3].p2.x % b[3].p2.y % b[3].p3.x % b[3].p3.y
      ).str().c_str()
    );

  auto svg = svg_process(read_input, &subject, read_input, &clip,
    river::clip_type::intersection, river::fill_rule::positive);
  std::fstream svg_file("river_test.svg", std::ios_base::out | std::ios_base::trunc);
  svg_file << svg;
  svg_file.close();

  debug_util::disconnect();
  return 0;
}
