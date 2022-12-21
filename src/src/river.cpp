#include "river.h"
#include <boost/spirit/include/qi.hpp>
#include <boost/phoenix/phoenix.hpp>

#include <boost/fusion/container.hpp>
#include <boost/fusion/sequence.hpp>

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

    Point get_point(double x, double y) {
        return Point(x, y);
    }

    

    struct moveto_sa
    {
        template <typename Container, typename Item>
        struct result
        {
            typedef void type;
        };

        template <typename Path, typename Point>
        void operator()(Path& c, Point const& p) const
        {
            c.moveto(p);
        }

    };

    void hello(boost::fusion::vector<double, double> const& p) {
        std::cout << boost::fusion::at_c<0>(p)  << ","  << boost::fusion::at_c<1>(p) << std::endl;
    }

    namespace qi = boost::spirit::qi;
    typedef boost::fusion::vector<double, double> _p;

    Point get_p(_p const& p) {
        return Point(boost::fusion::at_c<0>(p), boost::fusion::at_c<1>(p));
    }

    struct move_to_sa
    {
        void operator()(_p const& p, qi::unused_type, qi::unused_type) const
        {
            _path->moveto(get_p(p));
        }

        move_to_sa(Path* p) : _path(p) { }
        Path* _path;
    };

    Path make_path(const char* s)
    {
        namespace ascii = boost::spirit::ascii;

        using namespace boost;
        using namespace boost::spirit;

        using std::string;
        using qi::char_;
        using qi::double_;

        Path res;


        qi::rule<string::iterator, _p(), ascii::space_type >
            point_ = qi::double_[phoenix::at_c<0>(_val) = _1]
            >> qi::double_[phoenix::at_c<1>(_val) = _1];

        //pt_rule point_p;
        auto move_p = (char_('m') >> point_)[move_to_sa(&res)];
        //auto line_p = char_('l') >> point_;
        //auto arc_p = char_('a') >> point_ >> point_;
        //auto cubic_p = char_('c') >> point_ >> point_ >> point_;

        //auto path = *(move_p | line_p | arc_p | cubic_p);

        std::string str(s);
        if (!phrase_parse(str.begin(), str.end(), move_p, ascii::space)) {
            //res.data.clear();
        }

        res.data.clear();
        return res;
    }
}
