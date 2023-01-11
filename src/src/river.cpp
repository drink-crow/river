#include "river.h"
#include <boost/spirit/include/qi.hpp>
#include <boost/fusion/container.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/bind/bind.hpp>

BOOST_FUSION_ADAPT_STRUCT(
  river::point,
  (double, x)
  (double, y)
)

namespace river
{
  const point& seg_moveto::get_target() const {
    return target;
  }

  seg_type seg_moveto::get_type() const
  {
    return seg_type::moveto;
  }

  const point& seg_lineto::get_target() const {
    return target;
  }

  seg_type seg_lineto::get_type() const
  {
    return seg_type::lineto;
  }

  const point& seg_arcto::get_target() const {
    return target;
  }

  seg_type seg_arcto::get_type() const
  {
    return seg_type::arcto;
  }

  const point& seg_cubicto::get_target() const {
    return target;
  }

  seg_type seg_cubicto::get_type() const
  {
    return seg_type::cubicto;
  }

  double seg_cubicto::curr_x(const point& from, double y) const
  {
    auto cubic = get_cubic(from);
    auto res = cubic.point_at_y(y);
    if (res.count == 1) {
      return cubic.point_at(res.t[0]).x;
    }
    else {
      return (target.x - from.x) * (y - from.y) / (target.y - from.y) + from.x;
    }

  }

  void empty_path_move_func(path_moveto_func_para) {}
  void empty_path_line_func(path_lineto_func_para) {}
  void empty_path_arc_func(path_arcto_func_para) {}
  void empty_path_cubic_func(path_cubicto_func_para) {}

  void path::traverse(const path_traverse_funcs& in_funcs, void* user) const
  {
    path_traverse_funcs act = in_funcs;
    if (!act.move_to) act.move_to = empty_path_move_func;
    if (!act.line_to) act.line_to = empty_path_line_func;
    if (!act.arc_to) act.arc_to = empty_path_arc_func;
    if (!act.cubic_to) act.cubic_to = empty_path_cubic_func;

    point last_p(0, 0);
    point cur_p = last_p;
    for (auto& e : data) {
      cur_p = e->get_target();

      switch (e->get_type())
      {
      case seg_type::moveto:
      {
        act.move_to(last_p, cur_p, user);
        break;
      }
      case seg_type::lineto:
      {
        act.line_to(last_p, cur_p, user);
        break;
      }
      case seg_type::arcto:
      {
        auto arcto_seg = (const seg_arcto*)(e);
        // ToDo start_sweepRad 没有正确计算
        act.arc_to(last_p, arcto_seg->center, arcto_seg->center, cur_p, user);
        break;
      }
      case seg_type::cubicto:
      {
        auto cubicto_seg = (const seg_cubicto*)(e);
        act.cubic_to(last_p, cubicto_seg->ctrl1, cubicto_seg->ctrl2, cur_p, user);
        break;
      }
      default:
        break;
      }

      last_p = cur_p;
    }
  }

  paths make_path(const char* s)
  {
    using namespace boost::spirit;
    using boost::spirit::qi::char_;
    using boost::spirit::qi::double_;
    using boost::placeholders::_1;
    using it = std::string::iterator;

    namespace ascii = boost::spirit::ascii;
    namespace fusion = boost::fusion;

    using cubic_pack = boost::fusion::vector<char, point, point, point>;
    using arc_pack = boost::fusion::vector<char, point, point>;

    struct writer
    {
      paths* _path;
      path& back() const { return _path->back(); }

      void moveto(point const& p) const { _path->push_back(path()); back().moveto(p); }
      void lineto(const point& p) const { back().lineto(p); }
      void arcto(arc_pack const& pack) const {
        back().arcto(fusion::at_c<1>(pack), fusion::at_c<2>(pack));
      }
      void cubicto(cubic_pack const& pack) const {
        back().cubicto(fusion::at_c<1>(pack), fusion::at_c<2>(pack), fusion::at_c<3>(pack));
      }
    };

    paths res;
    writer w;
    w._path = &res;

    qi::rule<it, point(), ascii::space_type> point_p;
    qi::rule<it, cubic_pack(), ascii::space_type> cubic_p;
    qi::rule<it, arc_pack(), ascii::space_type> arc_p;
    point_p %= double_ >> double_;
    auto move_p = char_('m') >> point_p[boost::bind(&writer::moveto, &w, _1)];
    auto line_p = char_('l') >> point_p[boost::bind(&writer::lineto, &w, _1)];
    arc_p %= (char_('a') >> point_p >> point_p)[boost::bind(&writer::arcto, &w, _1)];
    cubic_p %= (char_('c') >> point_p >> point_p >> point_p)[boost::bind(&writer::cubicto, &w, _1)];
    auto path = *(move_p | line_p | arc_p | cubic_p);

    std::string str(s);
    point p;
    if (!qi::phrase_parse(str.begin(), str.end(), path, ascii::space)) {
      res.clear();
    }

    return res;
  }
}
