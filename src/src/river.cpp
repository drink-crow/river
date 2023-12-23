#include "river.h"
#include <boost/spirit/include/qi.hpp>
#include <boost/fusion/container.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/bind/bind.hpp>

#include "segment.h"

BOOST_FUSION_ADAPT_STRUCT(
  river::point,
  (double, x)
  (double, y)
)

namespace river
{
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
        //back().arcto(fusion::at_c<1>(pack), fusion::at_c<2>(pack));
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
    qi::rule<it, point(), ascii::space_type> move_p =
        char_('m') >> point_p[boost::bind(&writer::moveto, &w, _1)];
    qi::rule<it, point(), ascii::space_type> line_p =
        char_('l') >> point_p[boost::bind(&writer::lineto, &w, _1)];
    arc_p %= (char_('a') >> point_p >> point_p)[boost::bind(&writer::arcto, &w, _1)];
    cubic_p %= (char_('c') >> point_p >> point_p >> point_p)[boost::bind(&writer::cubicto, &w, _1)];
    // 自动推导的 rule 在非 vs 环境下有问题，即使编译器一样
    //auto path = *(move_p | line_p | arc_p | cubic_p);

    std::string str(s);
    if (!qi::phrase_parse(str.begin(), str.end(), *(move_p | line_p | arc_p | cubic_p), ascii::space)) {
      res.clear();
    }

    return res;
  }
}
