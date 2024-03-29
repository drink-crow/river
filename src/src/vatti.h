#pragma once
#include "river.h"
#include <queue>
#include "boost/pool/pool.hpp"
#include "boost/pool/object_pool.hpp"

#include "segment.h"

namespace vatti
{
  using namespace river;

  typedef double num;

  enum class vertex_flags : uint32_t {
    none = 0,
    open_start = 0b1 << 0,
    open_end = 0b1 << 1,
    local_max = 0b1 << 2,
    local_min = 0b1 << 3
  };

  constexpr enum vertex_flags operator &(enum vertex_flags a, enum vertex_flags b)
  {
    return (enum vertex_flags)(uint32_t(a) & uint32_t(b));
  }

  constexpr enum vertex_flags operator |(enum vertex_flags a, enum vertex_flags b)
  {
    return (enum vertex_flags)(uint32_t(a) | uint32_t(b));
  }

  struct edge;
  struct out_pt;
  struct out_polygon;
  struct out_bound;

  struct vertex
  {
    point pt;

    // if segment == nullptr, it is line point;
    segment* next_seg;
    vertex* next;
    vertex* prev;
    vertex_flags flags = vertex_flags::none;
  };

  struct local_minima
  {
    vertex* vert;
    path_type polytype;
    bool is_open;

    local_minima(vertex* v, path_type pt, bool open) :
      vert(v), polytype(pt), is_open(open) {}
  };

  struct break_info
  {
    vertex* vert;
    point break_point;
    num t;

    bool operator==(const break_info& r) const {
      return vert == r.vert
        && break_point == r.break_point
        && t == r.t;
    }
  };

  struct out_seg {
    seg_type type = seg_type::moveto;
    void* data = nullptr;
  };

  struct out_pt {
    point pt;
    out_pt* next = nullptr;
    out_pt* prev = nullptr;
    out_seg* next_data = nullptr;
    out_polygon* outrec;

    out_pt(const point& pt_, out_polygon* outrec_) : pt(pt_), outrec(outrec_) {
      next = this;
      prev = this;
    }
  };

  struct cubic_data {
    point ctrl1;
    point ctrl2;
  };

  struct out_polygon
  {
    size_t idx = 0;
    out_polygon* owner = nullptr;
    point origin;
    out_bound* up_bound = nullptr;
    out_bound* down_bound = nullptr;

    std::vector<segment*> up_path;
    std::vector<segment*> down_path;

    bool is_complete = false;
    bool is_open = false;
  };

  struct out_bound
  {
    out_bound* prev_in_obl = nullptr;
    out_bound* next_in_obl = nullptr;

    out_polygon* owner = nullptr;
    edge* curr_edge = nullptr;
    num stop_x;
    int wind_dx = 1;

    inline bool is_up() const {
      return wind_dx < 0;
    }
  };

  struct edge
  {
    point bot;
    point top;
    num curr_x = 0;
    // ToDo 单 double 不足以表示曲线内容走向和斜率
    double dx = 0.;
    int wind_dx = 1; // 1 = up, -1 = down
    int wind_cnt = 0; // 表示环绕方向左边区域的环绕数
    int wind_cnt2 = 0; //winding count of the opposite polytype

    edge* prev_in_ael = nullptr;
    edge* next_in_ael = nullptr;
    out_bound* bound = nullptr;

    vertex* vertex_top = nullptr;
    local_minima* local_min = nullptr;

    inline bool is_up() const { return wind_dx < 0; }
  };

  class clipper
  {
  public:
    void add_path(const paths& paths, path_type polytype, bool is_open);
    void add_path(read_path_func, const void* path, path_type pt, bool is_open);

    void process(clip_type operation, fill_rule fill, paths& output);
    void process(clip_type operation, fill_rule fill_rule,
      path_traverse_funcs write_func, void* output);

    void intersect(vertex* const& r, vertex* const& l);
    vertex* new_vertex();
    seg_lineto* new_lineto(const point& target);
    seg_cubicto* new_cubicto(const point& ctrl1, const point& ctrl2,
      const point& target);
    void insert_vertex_list(vertex* first, vertex* end, path_type polytype, bool is_open);
  private:
    local_minima* new_local_minima(vertex* v, path_type pt, bool open);
    edge* new_edge();
    out_bound* new_bound();
    out_polygon* new_out_polygon();
    segment* copy_seg(const edge* e, bool reverse = false);

    void process_intersect();
    void add_local_min(vertex* vert, path_type pt, bool is_open);
    void reset();
    void insert_scanline(num y);
    bool pop_scanline(num old_y, num& new_y);
    bool pop_local_minima(num y, local_minima** out);
    void insert_local_minima_to_ael(num y);
    void recalc_windcnt();
    void update_ouput_bound(num y);
    bool is_contributing(edge* e);
    void calc_windcnt(edge* e);
    void insert_windcnt_change(num x);
    void insert_into_ael(edge* newcomer);
    void insert_into_ael(edge* left, edge* newcomer);
    void take_from_ael(edge*);
    bool is_valid_ael_order(const edge* resident, const edge* newcomer) const;
    void set_windcount_closed(edge* e);
    void do_top_of_scanbeam(num y);
    void close_output(num y);
    void resort_ael(num y);
    void build_output(path_traverse_funcs write_func, void* output);
    void join_output(out_bound* a, out_bound* b, num y);
    void new_output(edge* a, edge* b);
    void update_bound(out_bound* bound, edge* new_edge);
    void delete_obl_bound(out_bound*);
    edge* do_maxima(edge* e);
    edge* delete_active_edge(edge*);

    clip_type cliptype_ = clip_type::intersection;
    fill_rule fillrule_ = fill_rule::positive;
    //fill_rule fillpos = fill_rule::positive;

    // 使用 object_pool 提高内存申请的效率和放置内存泄露
    boost::object_pool<vertex> vertex_pool;
    boost::object_pool<seg_lineto> line_pool;
    boost::object_pool<seg_cubicto> cubic_pool;
    boost::object_pool<edge> edge_pool;
    boost::object_pool<out_bound> out_bound_pool;
    boost::object_pool<local_minima> local_minima_pool;
    boost::object_pool<out_polygon> out_polygon_pool;

    std::vector<vertex*> paths_start;
    std::vector<break_info> break_info_list;
    std::vector<local_minima*> local_minima_list;
    std::vector<local_minima*>::iterator cur_locmin_it;
    std::priority_queue<num, std::vector<num>, std::greater<num>> scanline_list;
    edge* ael_first = nullptr;
    out_bound* obl_first;
    std::vector<num> windcnt_change_list;
    std::vector<out_polygon*> output_list;

    //boost::pool<out_seg> seg_pool;
    //boost::pool<double> seg_data_pool;

    bool succeeded_ = true;

  };
}

