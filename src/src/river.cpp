#include "river.h"
#include <boost/spirit/include/qi.hpp>
#include <boost/fusion/container.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/bind/bind.hpp>

BOOST_FUSION_ADAPT_STRUCT(
    river::Point,
    (double, x)
    (double, y)
)

namespace river
{
    const Point&  Seg_moveto::get_target() const {
        return target;
    }

    SegType Seg_moveto::get_type() const
    {
        return SegType::MoveTo;
    }

    const Point& Seg_lineto::get_target() const {
        return target;
    }

    SegType Seg_lineto::get_type() const
    {
        return SegType::LineTo;
    }

    const Point& Seg_arcto::get_target() const {
        return target;
    }

    SegType Seg_arcto::get_type() const
    {
        return SegType::ArcTo;
    }

    const Point& Seg_cubicto::get_target() const {
        return target;
    }

    SegType Seg_cubicto::get_type() const
    {
        return SegType::CubicTo;
    }

    Path make_path(const char* s)
    {
        using namespace boost::spirit;
        using boost::spirit::qi::char_;
        using boost::spirit::qi::double_;
        using boost::placeholders::_1;
        using it = std::string::iterator;

        namespace ascii = boost::spirit::ascii;
        namespace fusion = boost::fusion;

        using cubic_pack = boost::fusion::vector<char, Point, Point, Point>;
        using arc_pack = boost::fusion::vector<char, Point, Point>;

        struct writer
        {

            void moveto(Point const& i) const { _path->moveto(i); }
            void lineto(const Point& tp) const { _path->lineto(tp); }
            void arcto(arc_pack const& pack) const {
                _path->arcto(fusion::at_c<1>(pack), fusion::at_c<2>(pack));
            }
            void cubicto(cubic_pack const& pack) const {
                _path->cubicto(fusion::at_c<1>(pack), fusion::at_c<2>(pack), fusion::at_c<3>(pack));
            }

            Path* _path;
        };

        Path res;
        writer w;
        w._path = &res;

        qi::rule<it, Point(), ascii::space_type> point_p;
        qi::rule<it, cubic_pack(), ascii::space_type> cubic_p;
        qi::rule<it, arc_pack(), ascii::space_type> arc_p;
        point_p %= double_ >> double_;
        auto move_p = char_('m') >> point_p[boost::bind(&writer::moveto, &w, _1)];
        auto line_p = char_('l') >> point_p[boost::bind(&writer::lineto, &w, _1)];
        arc_p %= (char_('a') >> point_p >> point_p)[boost::bind(&writer::arcto, &w, _1)];
        cubic_p %= (char_('c') >> point_p >> point_p >> point_p)[boost::bind(&writer::cubicto, &w, _1)];
        auto path = *(move_p | line_p | arc_p | cubic_p);

        std::string str(s);
        Point p;
        if (!qi::phrase_parse(str.begin(), str.end(), path, ascii::space)) {
            res.data.clear();
        }

        return res;
    }
}
