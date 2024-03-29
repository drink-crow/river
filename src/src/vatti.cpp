#include "vatti.h"
#include "rmath.h"
#include "scan_line.h"
#include "util.h"

#include <algorithm>
#include <list>

namespace scan_line
{
  using namespace vatti;
  using namespace rmath;

  template<>
  struct all_function<vertex*>
  {
    typedef ::rmath::rect box;
    typedef double ct;
    typedef vertex* key;

    static bool intersect(const box& a, const box& b) {
      return a.intersect(b);
    }

    static ct get_min_x(const box& b) { return b.min.x; }
    static ct get_min_y(const box& b) { return b.min.y; }
    static ct get_max_x(const box& b) { return b.max.x; }
    static ct get_max_y(const box& b) { return b.max.y; }

    static box get_rect(const key& in)
    {
      return in->next_seg->get_boundary(in->pt);
    }

    static void intersect(const key& r, const key& l, void* user)
    {
      ((clipper*)(user))->intersect(r, l);
    }
  };
}

namespace vatti
{
  using namespace river;
  using namespace rmath;

  constexpr num num_max = std::numeric_limits<num>::max();
  constexpr num num_min = -num_max;

  constexpr double err = 1e-6;
  constexpr double zero = 0 + err;
  constexpr double one = 1 - err;

  // ToDO: 判断没齐全
  bool is_vaild(const path& p) {
    return p.data.size() > 1;
  }

  inline path_type get_polytype(const edge* e) {
    return e->local_min->polytype;
  }

  inline bool is_horz(const edge* e) { return e->bot.y == e->top.y; }

  inline bool is_open(const edge* e) { return false; }

  inline bool is_maxima(const edge* e) {
    return (e->vertex_top->flags
      & vertex_flags::local_max) != vertex_flags::none;
  }

  inline bool is_hot(const edge* e) { return e->bound; }

  void clipper::add_local_min(vertex* vert, path_type pt, bool is_open)
  {
    //make sure the vertex is added only once ...
    if ((vertex_flags::local_min & vert->flags) != vertex_flags::none) return;

    vert->flags = (vert->flags | vertex_flags::local_min);
    local_minima_list.push_back(new_local_minima(vert, pt, is_open));
  }

  /*         0
           pi/2
             |
   -inf pi---O---0  +inf*/
  num get_direction(const point& top, const point& bot) {
    double dy = double(top.y - bot.y);
    if (dy != 0)
      return (top.x - bot.x) / dy;
    else if (top.x > bot.x)
      return num_max;
    else
      return num_min;
  }

  void set_direction(edge* e) {
    e->dx = get_direction(e->top, e->bot);
  }

  segment* get_seg(const edge* e, point& out_from) {
    segment* res = nullptr;
    if (e->is_up()) {
      auto prev = e->vertex_top->prev;
      res = prev->next_seg;
      out_from = prev->pt;
    }
    else {
      res = e->vertex_top->next_seg;
      out_from = e->top;
    }

    return res;
  }

  segment* get_seg(const edge* e) {
    point unuse;
    return get_seg(e, unuse);
  }

  // ToDo handle other segment type
  segment* clipper::copy_seg(const edge* e, bool reverse) {
    point from;
    segment* ref = get_seg(e, from);
    segment* res = nullptr;
    switch (ref->get_type())
    {
    case seg_type::lineto: res = line_pool.malloc(); break;
    case seg_type::cubicto: res = cubic_pool.malloc(); break;
    default:
      { throw "unhandle seg type in function:copy_seg"; }
      break;
    }
    ref->copy(res);

    if (reverse) res->reverse(from);
    return res;
  }

  void set_segment(vertex* prev, vertex* mem, segment* move_seg)
  {
    prev->next = mem;
    prev->next_seg = move_seg;

    mem->prev = prev;
    mem->pt = move_seg->get_target();
    mem->flags = vertex_flags::none;
  }

  void update_intermediate_curr_x(edge* e, num y)
  {
    point from;
    auto seg = get_seg(e, from);
    e->curr_x = seg->curr_x(from, y);
  }

  vertex* next_vertex(const edge* e) {
    if (e->is_up()) return e->vertex_top->next;
    else return e->vertex_top->prev;
  }

  void next(edge* e) {
    e->vertex_top = next_vertex(e);
    e->bot = e->top;
    e->top = e->vertex_top->pt;
    e->curr_x = e->bot.x;
  }

  bool is_same(const edge* a, const edge* b)
  {
    auto a_seg = get_seg(a);
    auto b_seg = get_seg(b);
    if (a->bot == b->bot && a->top == b->top
      && a_seg->get_type() == b_seg->get_type()) {
      switch (a_seg->get_type())
      {
      case seg_type::lineto:
        return true;
        break;
      case seg_type::cubicto:
      {
        auto c1 = (seg_cubicto*)get_seg(a);
        auto c2 = (seg_cubicto*)get_seg(b);
        return (c1->ctrl1 == c2->ctrl1 && c1->ctrl2 == c2->ctrl2)
          || (c1->ctrl2 == c2->ctrl1 && c1->ctrl1 == c2->ctrl2);
        break;
      }
      default:
        // ToDo 增加别的曲线类型的判断
        break;
      }

    }

    return false;
  }

  struct process_path_data
  {
    class clipper * clipper = nullptr;
    path_type polytype = path_type::subject;
    bool is_open = false;
    vertex* prev = nullptr, * cur = nullptr;
    vertex* first = nullptr;

    void new_cur_vertex() {
      cur = clipper->new_vertex();
    }
    void next_loop() {
      prev = cur;
    }
  };

  void read_moveto(path_moveto_func_para) {
    auto data = (process_path_data*)(user);
    auto& first = data->first;

    if (first) {
      data->clipper->insert_vertex_list(first, data->prev,
        data->polytype, data->is_open);
      first = nullptr;
      data->prev = nullptr;
      data->cur = nullptr;
    }

    data->new_cur_vertex();
    data->first = data->cur;
    data->first->pt = to;
    data->next_loop();
  }

  void read_lineto(path_lineto_func_para) {
    auto data = (process_path_data*)(user);

    if (from != to) {
      data->new_cur_vertex();
      set_segment(data->prev, data->cur, data->clipper->new_lineto(to));
      data->next_loop();
    }
  }

  void read_arcto(path_arcto_func_para) {
    //auto data = (process_path_data*)(user);

    
  }

  void read_cubicto(path_cubicto_func_para) {
    auto data = (process_path_data*)(user);
    auto& prev = data->prev;
    auto& cur = data->cur;
    auto& clipper = data->clipper;

    bezier_cubic b{from, ctrl1, ctrl2, to};

    // 水平状态下视为直线
    if (b.p0.y == b.p1.y && b.p1.y == b.p2.y && b.p2.y == b.p3.y) {
      read_lineto(from, to, user);
      return;
    }

    double split_t[] = { -1,-1 };
    b.split_y(split_t, split_t + 1);
    // 在误差范围内检查
    std::vector<double> recheck_t;
    for (size_t i = 0; i < 2; ++i) {
      auto t = split_t[i];
      if (zero < t && t < one) recheck_t.push_back(t);
    }
    bezier_cubic tmp[3];
    b.split(recheck_t.data(), tmp, recheck_t.size());
    switch (recheck_t.size())
    {
    case 2:
      data->new_cur_vertex();
      set_segment(prev, cur, clipper->new_cubicto(tmp[0].p1, tmp[0].p2, tmp[0].p3));
      data->next_loop();

      data->new_cur_vertex();
      set_segment(prev, cur, clipper->new_cubicto(tmp[1].p1, tmp[1].p2, tmp[1].p3));
      data->next_loop();

      data->new_cur_vertex();
      set_segment(prev, cur, clipper->new_cubicto(tmp[2].p1, tmp[2].p2, tmp[2].p3));
      data->next_loop();
      break;
    case 1:
      data->new_cur_vertex();
      set_segment(prev, cur, clipper->new_cubicto(tmp[0].p1, tmp[0].p2, tmp[0].p3));
      data->next_loop();

      data->new_cur_vertex();
      set_segment(prev, cur, clipper->new_cubicto(tmp[1].p1, tmp[1].p2, tmp[1].p3));
      data->next_loop();
      break;
    default:
      data->new_cur_vertex();
      set_segment(prev, cur, clipper->new_cubicto(ctrl1, ctrl2, to));
      data->next_loop();
      break;
    }
  }

  void read_path(const void* path_, path_traverse_funcs segment_funcs, void* user)
  {
    auto p = (const path*)(path_);
    p->traverse(segment_funcs, user);
  }

  void write_moveto(path_moveto_func_para)
  {
    auto out = (paths*)(user);
    out->push_back(path());
    out->back().moveto(to);
  }

  void write_lineto(path_lineto_func_para)
  {
    auto out = (paths*)(user);
    out->back().lineto(to);
  }

  void write_cubicto(path_cubicto_func_para)
  {
    auto out = (paths*)(user);
    out->back().cubicto(ctrl1, ctrl2, to);;
  }

#if RIVER_GRAPHICS_DEBUG &  1
  void show_moveto(path_moveto_func_para) {}
  void show_lineto(path_lineto_func_para) {
    draw_path(rmath::line{ from, to }, QPen(Qt::red));
  }
  void show_cubicto(path_cubicto_func_para) {
    draw_path(rmath::bezier_cubic{ from,ctrl1,ctrl2,to }, QPen(Qt::red));
  }
#endif

  void clipper::reset()
  {
    // build init scan line list
    std::sort(local_minima_list.begin(), local_minima_list.end(),
      [](local_minima* const& l, local_minima* const& r) {
        if (l->vert->pt.y != r->vert->pt.y) { return l->vert->pt.y < r->vert->pt.y; }
        else { return l->vert->pt.x < r->vert->pt.x; }
      });

    for (auto it = local_minima_list.rbegin(); it != local_minima_list.rend(); ++it) {
      insert_scanline((*it)->vert->pt.y);
    }

    cur_locmin_it = local_minima_list.begin();
    ael_first = nullptr;
    obl_first = nullptr;
  }

  void clipper::add_path(const paths& paths, path_type polytype, bool is_open)
  {
    for (auto& path : paths)
    {
      if (!is_vaild(path)) continue;

      add_path(read_path, &path, polytype, is_open);
    }
  }

  void clipper::add_path(read_path_func read_path_func, const void* path,
    path_type pt, bool is_open)
  {
    process_path_data data;
    data.clipper = this;
    data.polytype = pt;
    data.is_open = is_open;

    path_traverse_funcs funcs;
    funcs.move_to = read_moveto;
    funcs.line_to = read_lineto;
    funcs.cubic_to = read_cubicto;

    read_path_func(path, funcs, &data);
    insert_vertex_list(data.first, data.prev, pt, is_open);
  }

  void clipper::process(clip_type operation, fill_rule fill, paths& output)
  {
    path_traverse_funcs write_func;
    write_func.move_to = write_moveto;
    write_func.line_to = write_lineto;
    write_func.cubic_to = write_cubicto;

    process(operation, fill, write_func, &output);
  }

  void clipper::process(clip_type operation, fill_rule fill, path_traverse_funcs write_func, void* output)
  {
    cliptype_ = operation;
    fillrule_ = fill;

    process_intersect();
    reset();

    num y = -std::numeric_limits<num>::max();
    if (!pop_scanline(y, y)) return;

    while (true)
    {
      insert_local_minima_to_ael(y);
      recalc_windcnt();
      update_ouput_bound(y);
      if (!pop_scanline(y, y)) break;  // y new top of scanbeam
      do_top_of_scanbeam(y);
      close_output(y);
      resort_ael(y);
    }

    if (succeeded_) {

#if RIVER_GRAPHICS_DEBUG &  1
      path_traverse_funcs show_funcs;
      show_funcs.move_to = show_moveto;
      show_funcs.line_to = show_lineto;
      show_funcs.cubic_to = show_cubicto;
      build_output(show_funcs, nullptr);
#endif

      build_output(write_func, output);
    }
  }

  vertex* clipper::new_vertex()
  {
    auto v = vertex_pool.malloc();
    construct_at<vertex>(v);
    return v;
  }

  seg_lineto* clipper::new_lineto(const point& target)
  {
    auto l = line_pool.malloc();
    construct_at<seg_lineto>(l, target);
    return l;
  }

  seg_cubicto* clipper::new_cubicto(const point& ctrl1, const point& ctrl2,
    const point& target)
  {
    auto c = cubic_pool.malloc();
    construct_at<seg_cubicto>(c, ctrl1, ctrl2, target);
    return c;
  }

  edge* clipper::new_edge()
  {
    auto e = edge_pool.malloc();
    construct_at<edge>(e);
    return e;
  }

  void clipper::insert_vertex_list(vertex* first, vertex* end, 
    path_type polytype, bool is_open)
  {
    if (!first || !end || !(first->next) || first->next == first) return;

    if (first->pt != end->pt) { // 末点不重合则连接至 first
      set_segment(end, first, new_lineto(first->pt));
    }
    else { // 否则丢弃cur，重新连接至 first
      auto prev = end->prev;
      set_segment(prev, first, prev->next_seg);
    }

    // 双重检查
    if (first->next == first) return;

#if  RIVER_GRAPHICS_DEBUG & 1
    {
      auto cur_v = first;
      QPen pen;
      if (polytype == path_type::clip) pen.setColor(Qt::blue);
      do {
        switch (cur_v->next_seg->get_type())
        {
        case seg_type::lineto:
          draw_path(rmath::line{ cur_v->pt, cur_v->next_seg->get_target() }, pen);
          break;
        case seg_type::cubicto:
        {
          auto cubic = (const seg_cubicto*)(cur_v->next_seg);
          draw_path(cubic->get_cubic(cur_v->pt), pen);
          break;
        }
        default:
          break;
        }

        cur_v = cur_v->next;
      } while (cur_v != first);
    }
#endif

    // form local minma
    bool going_down, going_down0;
    {
      bool total_horz = true;
      auto e = first;
      do {
        if (e->pt.y == e->prev->pt.y) {
          e = e->prev;
        }
        else {
          going_down = e->pt.y < e->prev->pt.y;
          total_horz = false;
          break;
        }
      } while (e != first);
      // 完全水平扁平的内容会被跳过
      if (total_horz) return;
      going_down0 = going_down;
    }
    
    auto prev = first;
    auto cur = first->next;
    while (cur != first)
    {
      // 水平线会被跳过
      // 水平线的的放置规则在后面的扫描过程处理
      if (cur->pt.y > prev->pt.y && going_down) {
        going_down = false;
        add_local_min(prev, polytype, is_open);
      }
      else if (cur->pt.y < prev->pt.y && !going_down) {
        prev->flags = prev->flags | vertex_flags::local_max;
        going_down = true;
      }

      prev = cur;
      cur = cur->next;
    }

    // 处理最后的连接，实际判断的点是 first->prev
    if (going_down != going_down0) {
      if (going_down0) {
        prev->flags = prev->flags | vertex_flags::local_max;
      }
      else {
        add_local_min(prev, polytype, is_open);
      }
    }

    paths_start.push_back(first);
#if RIVER_GRAPHICS_DEBUG &  0
    // 图形调试
    QPen redpen(Qt::red);
    QPen bluePen(Qt::blue);
    for (auto& minima : local_minima_list) {
      debug_util::show_point(toqt(minima->pt->pt), redpen);
    }

    auto _s = first;
    do {
      if ((_s->flags & vertex_flags::local_max) != vertex_flags::none) {
        debug_util::show_point(toqt(_s->pt), bluePen);
      }

      _s = _s->next;
    } while (_s != first);

#endif
  }

  void clipper::process_intersect()
  {
    break_info_list.clear();

    // 先求交点
    scan_line::scan_line<vertex*> scaner;
    for (auto& start : paths_start) {
      auto cur = start;
      do {
        scaner.add_segment(cur);
        cur = cur->next;
      } while (cur != start);
    }
    scaner.process(this);

    // 处理得到的打断信息
#if RIVER_GRAPHICS_DEBUG &  0
    {
      QPen redpen(Qt::red);
      for (auto& b : break_info_list) {
        debug_util::show_point(toqt(b.break_point), redpen);
      }
    }
#endif

    std::sort(break_info_list.begin(), break_info_list.end(),
      [](break_info const& l, break_info const& r) {
        if (l.vert != r.vert) return l.vert < r.vert;
    return l.t < r.t;
      });
    break_info_list.erase(std::unique(break_info_list.begin(), break_info_list.end()), break_info_list.end());

    auto start = break_info_list.begin();
    auto end = start;

    while (start != break_info_list.end()) {
      auto start_v = start->vert;
      auto cur_v = start_v;
      auto end_v = cur_v->next;

      while (end != break_info_list.end() && end->vert == cur_v) {
        ++end;
      }

      auto start_p = cur_v->pt;
      auto start_seg = cur_v->next_seg;
      //auto end_p = cur_v->next->pt;
      switch (start_seg->get_type())
      {
      case seg_type::lineto:
      {
        std::vector<break_info> cur_break_list(start, end);
        //auto dx = end_p.x >= start_p.x;
        //auto dy = end_p.y >= start_p.y;
        ((seg_lineto*)start_seg)->target = cur_break_list[0].break_point;
        auto next = new_vertex();
        set_segment(start_v, next, start_seg);
        cur_v = next;
        for (size_t i = 1; i < cur_break_list.size(); ++i) {
          set_segment(cur_v, new_vertex(), new_lineto(cur_break_list[i].break_point));
          cur_v = cur_v->next;
        }
        auto end_flags = end_v->flags;
        set_segment(cur_v, end_v, new_lineto(end_v->pt));
        end_v->flags = end_flags;
        break;
      }
      case seg_type::cubicto:
      {
        std::vector<break_info> cur_break_list(start, end);
        std::sort(cur_break_list.begin(), cur_break_list.end(),
          [](break_info const& li, break_info const& ri) {
            return li.t < ri.t;
          });
        std::vector<bezier_cubic> split_c(cur_break_list.size() + 1);
        std::vector<double> split_t;
        for (auto& info : cur_break_list) split_t.push_back(info.t);
        auto first_cubicto = (seg_cubicto*)start_seg;
        first_cubicto->get_cubic(start_p).split(split_t.data(), split_c.data(), split_t.size());

        first_cubicto->ctrl1 = split_c[0].p1;
        first_cubicto->ctrl2 = split_c[0].p2;
        first_cubicto->target = cur_break_list[0].break_point;
        auto next = new_vertex();
        set_segment(start_v, next, first_cubicto);
        cur_v = next;
        for (size_t i = 1; i < cur_break_list.size(); ++i) {
          set_segment(cur_v, new_vertex(), new_cubicto(
            split_c[i].p1, split_c[i].p2, cur_break_list[i].break_point));
          cur_v = cur_v->next;
        }
        auto end_flags = end_v->flags;
        set_segment(cur_v, end_v, new_cubicto(split_c.back().p1, split_c.back().p2, end_v->pt));
        end_v->flags = end_flags;

        break;
      }
      default:
        break;
      }

      start = end;
    }

#if RIVER_GRAPHICS_DEBUG &  0
    // 图形测试打断是否正确处理
    for (auto& first : paths_start)
    {
      auto cur_v = first;
      QPen redpen(Qt::red);
      //redpen.setWidth(3);
      do {
        switch (cur_v->next_seg->get_type())
        {
        case seg_type::lineto:
          debug_util::show_line(QLineF(toqt(cur_v->pt), toqt(cur_v->next_seg->get_target())), redpen);
          break;
        case seg_type::cubicto:
        {
          auto cubic = (const seg_cubicto*)(cur_v->next_seg);
          debug_util::show_cubic(toqt(cur_v->pt), toqt(cubic->ctrl1), toqt(cubic->ctrl2), toqt(cur_v->next->pt), redpen);
          break;
        }
        default:
          break;
        }


        cur_v = cur_v->next;
      } while (cur_v != first);
    }
#endif
  }

  bool clipper::pop_scanline(num old_y, num& new_y)
  {
    if (scanline_list.empty()) return false;
    new_y = scanline_list.top();
    scanline_list.pop();

    while (new_y == old_y) {
      if (scanline_list.empty()) return false;
      new_y = scanline_list.top();
      scanline_list.pop();
    }

    while (!scanline_list.empty() && new_y == scanline_list.top())
      scanline_list.pop();  // Pop duplicates.
    return true;
  }

  void clipper::insert_local_minima_to_ael(num y)
  {
    local_minima* local_minima;
    edge* a, * b;

    while (pop_local_minima(y, &local_minima))
    {
      a = new_edge();
      a->bot = local_minima->vert->pt;
      // 这里确定一条边的朝向很简单，因为是 local minima，所以总是有以上一下，所以我们直接设定一条为vert->next
      // 则它的朝向肯定就是朝上的
      a->vertex_top = local_minima->vert->prev;  // ie descending
      a->wind_dx = 1;
      a->top = a->vertex_top->pt;
      a->local_min = local_minima;
      a->bound = nullptr;
      insert_windcnt_change(a->bot.x);
      while (is_horz(a)) {
        next(a);
        insert_windcnt_change(a->bot.x);
      }
      a->curr_x = a->bot.x;


      b = new_edge();
      b->bot = local_minima->vert->pt;
      b->vertex_top = local_minima->vert->next;  // ie ascending
      b->wind_dx = -1;
      b->top = b->vertex_top->pt;
      b->local_min = local_minima;
      b->bound = nullptr;
      insert_windcnt_change(b->bot.x);
      while (is_horz(b)) {
        next(b);
        insert_windcnt_change(b->bot.x);
      }
      b->curr_x = b->bot.x;

      insert_into_ael(a);
      insert_into_ael(a->prev_in_ael, b);
      insert_scanline(a->top.y);
      insert_scanline(b->top.y);
    }
  }

  void clipper::recalc_windcnt()
  {
    std::sort(windcnt_change_list.begin(), windcnt_change_list.end());
    windcnt_change_list.erase(std::unique(windcnt_change_list.begin(), windcnt_change_list.end()), windcnt_change_list.end());

    auto curr_e = ael_first;
    for (auto x : windcnt_change_list)
    {
      while (curr_e) {
        if (curr_e->curr_x < x) {
          curr_e = curr_e->next_in_ael;
          continue;
        }
        else if (curr_e->curr_x > x) {
          break;
        }
        else {
          if (curr_e->bound) curr_e->bound->curr_edge = nullptr;
          curr_e->bound = nullptr;
          calc_windcnt(curr_e);
          curr_e = curr_e->next_in_ael;
        }
      }
    }
  }

  void clipper::calc_windcnt(edge* curr_e)
  {
    edge* left = curr_e->prev_in_ael;
    path_type pt = get_polytype(curr_e);
    while (left && (get_polytype(left) != pt || is_open(left))) left = left->prev_in_ael;

    if (!left) {
      curr_e->wind_cnt = curr_e->wind_dx;
      curr_e->wind_cnt2 = 0;
      left = ael_first;
    }
    else if (fillrule_ == fill_rule::even_odd) {
      curr_e->wind_cnt = curr_e->wind_dx;
      curr_e->wind_cnt2 = left->wind_cnt2;
      left = left->next_in_ael;
    }
    else {
      //NonZero, positive, or negative filling here ...
      //if e's WindCnt is in the SAME direction as its WindDx, then polygon
      //filling will be on the right of 'e'.
      //NB neither e2.WindCnt nor e2.WindDx should ever be 0.
      if (left->wind_cnt * left->wind_dx < 0)
      {
        //opposite directions so 'e' is outside 'left' ...
        if (abs(left->wind_cnt) > 1)
        {
          //outside prev poly but still inside another.
          if (left->wind_dx * curr_e->wind_dx < 0)
            //reversing direction so use the same WC
            curr_e->wind_cnt = left->wind_cnt;
          else
            //otherwise keep 'reducing' the WC by 1 (ie towards 0) ...
            curr_e->wind_cnt = left->wind_cnt + curr_e->wind_dx;
        }
        else
          //now outside all polys of same polytype so set own WC ...
          curr_e->wind_cnt = (is_open(curr_e) ? 1 : curr_e->wind_dx);
      }
      else
      {
        //'e' must be inside 'left'
        if (left->wind_dx * curr_e->wind_dx < 0)
          //reversing direction so use the same WC
          curr_e->wind_cnt = left->wind_cnt;
        else
          //otherwise keep 'increasing' the WC by 1 (ie away from 0) ...
          curr_e->wind_cnt = left->wind_cnt + curr_e->wind_dx;
      }
      curr_e->wind_cnt2 = left->wind_cnt2;
      left = left->next_in_ael;  // ie get ready to calc WindCnt2
    }

    //update wind_cnt2 ...
    if (fillrule_ == fill_rule::even_odd) {
      while (left != curr_e) {
        if (get_polytype(left) != pt && !is_open(left))
          curr_e->wind_cnt2 = (curr_e->wind_cnt2 == 0 ? 1 : 0);
        left = left->next_in_ael;
      }
    }
    else {
      while (left != curr_e) {
        if (get_polytype(left) != pt && !is_open(left))
          curr_e->wind_cnt2 += left->wind_dx;
        left = left->next_in_ael;
      }
    }
  }

  void clipper::insert_windcnt_change(num x)
  {
    windcnt_change_list.push_back(x);
  }

  void clipper::update_ouput_bound(num y)
  {
    if (windcnt_change_list.empty()) return;
    windcnt_change_list.clear();

    struct new_bound {
      class edge* edge = nullptr;
      bool is_done = false;
    };
    std::vector<new_bound> bound_edge;
    // 开始构建新的输出
    auto e = ael_first;
    while (e) {
      if (is_contributing(e)) {
        // ToDo 这里需要验证是否一上一下
        if (!bound_edge.empty() && is_same(e, bound_edge.back().edge))
          bound_edge.pop_back();
        else
          bound_edge.push_back({ e, false });
      }
      e = e->next_in_ael;
    }

    // 下部早已合并，现在来合并上部可以合并的，依旧是只合并 curr_x 重合的左右边
    for (size_t i = 1; i < bound_edge.size(); i += 2)
    {
      if (bound_edge[i - 1].edge->curr_x == bound_edge[i].edge->curr_x)
      {
        bound_edge[i - 1].is_done = true;
        bound_edge[i - 0].is_done = true;

        new_output(bound_edge[i - 1].edge, bound_edge[i].edge);
      }
    }

    struct bound_data {
      class edge* edge;
      out_bound* bound;
      num curr_x;
      size_t obl_index;
    };

    // bound_edge 和 obl 剩下的依次按 curr_x, is_up, obl_index, ael, obl原来的顺序排序

    std::list<bound_data> sbl;
    for (auto& new_b : bound_edge) {
      if (new_b.is_done == false) sbl.push_back({
          new_b.edge, nullptr, new_b.edge->curr_x, sbl.size() });
    }
    auto curr_b = obl_first;
    while (curr_b) {
      sbl.push_back({ nullptr, curr_b, curr_b->stop_x, sbl.size() });
      curr_b = curr_b->next_in_obl;
    }
    sbl.sort(
      [](const bound_data& l, const bound_data& r) -> bool {
        if (l.curr_x != r.curr_x) return l.curr_x < r.curr_x;
    return l.obl_index < r.obl_index;
      });


    // 此时必定没有可以相连接的 out_bound, 也没有可以相连接组成闭合区域的 bound_edge
    // 只剩下可以上下相连的 out_bound 和 edge，按顺序两两相连即可

    {
      auto it = sbl.begin();
      while (it != sbl.end())
      {
        auto next = std::next(it);
        while (next != sbl.end() && next->curr_x == it->curr_x
          && bool(next->edge) == bool(it->edge)) {
          ++next;
        }
        if (next != sbl.end() && next->curr_x == it->curr_x
          && bool(next->edge) != bool(it->edge)) {
          // 同 curr_x 有多个候选时, 优先上下相连
          if (it->edge) update_bound(next->bound, it->edge);
          else update_bound(it->bound, next->edge);
          sbl.erase(next);
          ++it;
          continue;
        }

        // 只剩下可以按顺序两两相连即可
        next = std::next(it);
        if (it->edge && next->edge) new_output(it->edge, next->edge);
        else if (it->bound && next->bound) join_output(it->bound, next->bound, y);
        else if (it->edge) update_bound(next->bound, it->edge);
        else update_bound(it->bound, next->edge);
        // connect_bound(*it, *next);
        it = std::next(it, 2);
      }
    }

    // 解散旧的 obl，构造新的 obl
    // 按道理来说，旧的 obl 中的 out_bound 已经完全更新，直接构造新的 obl 即可
    {
      if (bound_edge.empty()) {
        obl_first = nullptr;
        return;
      }
      auto it = bound_edge.begin();
      obl_first = it->edge->bound;
      obl_first->prev_in_obl = nullptr; obl_first->next_in_obl = nullptr;
      auto last = obl_first;
      ++it;
      while (it != bound_edge.end()) {
        auto next_b = it->edge->bound;
        last->next_in_obl = next_b;
        next_b->prev_in_obl = last;
        last = next_b;
        ++it;
      }
    }
  }

  bool clipper::is_contributing(edge* e)
  {
    switch (fillrule_)
    {
    case fill_rule::even_odd:
      break;
    case fill_rule::non_zero:
      if (abs(e->wind_cnt) != 1) return false;
      break;
    case fill_rule::positive:
      if (e->wind_cnt != 1) return false;
      break;
    case fill_rule::negative:
      if (e->wind_cnt != -1) return false;
      break;
    }

    switch (cliptype_)
    {
    case clip_type::none:
      return false;
    case clip_type::intersection:
      switch (fillrule_)
      {
      case fill_rule::positive:
        return (e->wind_cnt2 > 0);
      case fill_rule::negative:
        return (e->wind_cnt2 < 0);
      default:
        return (e->wind_cnt2 != 0);
      }
      break;

    case clip_type::union_:
      switch (fillrule_)
      {
      case fill_rule::positive:
        return (e->wind_cnt2 <= 0);
      case fill_rule::negative:
        return (e->wind_cnt2 >= 0);
      default:
        return (e->wind_cnt2 == 0);
      }
      break;

    case clip_type::difference:
      bool result;
      switch (fillrule_)
      {
      case fill_rule::positive:
        result = (e->wind_cnt2 <= 0);
        break;
      case fill_rule::negative:
        result = (e->wind_cnt2 >= 0);
        break;
      default:
        result = (e->wind_cnt2 == 0);
      }
      if (get_polytype(e) == path_type::subject)
        return result;
      else
        return !result;
      break;

    case clip_type::xor_: return true;  break;
    }

    return false;
  }

  bool clipper::pop_local_minima(num y, local_minima** out)
  {
    if (cur_locmin_it == local_minima_list.end() || (*cur_locmin_it)->vert->pt.y != y) return false;
    *out = (*cur_locmin_it);
    ++cur_locmin_it;
    return true;
  }

  void clipper::insert_into_ael(edge* newcomer)
  {
    if (!ael_first) {
      // newcomer being ael_first
      ael_first = newcomer;
      newcomer->prev_in_ael = nullptr;
      newcomer->next_in_ael = nullptr;
    }
    else if (!is_valid_ael_order(ael_first, newcomer))
    {
      // let newcomer being ael_first
      newcomer->prev_in_ael = nullptr;
      newcomer->next_in_ael = ael_first;
      ael_first->prev_in_ael = newcomer;
      ael_first = newcomer;
    }
    else {
      // find last position can insert
      insert_into_ael(ael_first, newcomer);
    }
  }

  void clipper::insert_into_ael(edge* left, edge* newcomer)
  {
    if (left == nullptr) return insert_into_ael(newcomer);

    edge* resident = left;
    edge* next = resident->next_in_ael;
    while (next && is_valid_ael_order(next, newcomer))
    {
      resident = resident->next_in_ael;
      next = next->next_in_ael;
    }
    newcomer->next_in_ael = next;
    if (next) next->prev_in_ael = newcomer;
    newcomer->prev_in_ael = resident;
    resident->next_in_ael = newcomer;
  }

  void clipper::take_from_ael(edge* e)
  {
    auto prev = e->prev_in_ael;
    auto next = e->next_in_ael;

    if (prev) prev->next_in_ael = next;
    else ael_first = next;
    if (next) next->prev_in_ael = prev;

    e->prev_in_ael = nullptr;
    e->next_in_ael = nullptr;
  }

  bool clipper::is_valid_ael_order(const edge* resident, const edge* newcomer) const
  {
    if (newcomer->curr_x != resident->curr_x)
      return newcomer->curr_x > resident->curr_x;

    // 永远只有端点相交的情况，top 也永远位于 top 上方
    // 同一点排序，以点为原点，9点钟方向，顺时针排序
    // subject 在 clip 前面

    auto min = (std::min)(resident->bot.y, newcomer->bot.y);
    auto max = (std::min)(resident->top.y, newcomer->top.y);
    auto mid = (min + max) / 2;

    point rp, np;
    auto rx = get_seg(resident, rp)->curr_x(rp, mid);
    auto nx = get_seg(newcomer, np)->curr_x(np, mid);
    if (rx != nx) return rx < nx;

    return get_polytype(resident) < get_polytype(newcomer);
  }

  void clipper::insert_scanline(num y)
  {

    scanline_list.push(y);
  }

  void clipper::do_top_of_scanbeam(num y)
  {
    windcnt_change_list.clear();
    num last_x = num_min;
    auto e = ael_first;
    while (e)
    {
      if (e->top.y == y) {
        e->curr_x = e->top.x;
        if (e->curr_x == last_x) insert_windcnt_change(last_x);
        if (is_hot(e)) {
          auto b = e->bound;
          bool reverse = e->is_up() != b->is_up();
          auto new_seg = copy_seg(e, reverse);
          if (b->is_up()) {
            b->owner->up_path.push_back(new_seg);
          }
          else {
            b->owner->down_path.push_back(new_seg);
          }

          b->stop_x = e->curr_x;
        }

        last_x = e->curr_x;
        if (is_maxima(e)) {
          e = do_maxima(e);
          continue;
        }

        next(e);

        while (e)
        {
          if (is_horz(e)) {
            insert_windcnt_change(e->top.x);
            if (is_maxima(e)) {
              e = do_maxima(e);
              break;
            }
          }
          else {
            insert_scanline(e->top.y);
            e = e->next_in_ael;
            break;
          }
          next(e);
        }
      }
      else {
        update_intermediate_curr_x(e, y);
        if (is_hot(e)) e->bound->stop_x = e->curr_x;
        if (e->curr_x == last_x) insert_windcnt_change(last_x);
        last_x = e->curr_x;
        e = e->next_in_ael;
      }
    }
  }

  void clipper::close_output(num y)
  {
    // 只能左和右闭合，右和左闭合的等待后续处理
    if (!obl_first) return;

    auto curr = obl_first;
    auto next = curr->next_in_obl;
    while (next) {
      auto nextnext = next->next_in_obl;
      if (curr->stop_x == next->stop_x) {
        join_output(curr, next, y);
      }

      curr = nextnext;
      if (!curr) return;
      next = curr->next_in_obl;
    }
  }

  void clipper::resort_ael(num y)
  {
    auto& list = windcnt_change_list;
    std::sort(list.begin(), list.end());
    list.erase(std::unique(list.begin(), list.end()), list.end());

    auto it = list.begin();
    auto e = ael_first;

    std::vector<edge*> reinsert_list;
    while (it != list.end() && e) {
      if (e->curr_x == *it) {
        reinsert_list.push_back(e);
        auto next = e->next_in_ael;
        // 重新排列也意味着原来的bound有可能失效
        if (e->bound) e->bound->curr_edge = nullptr;
        e->bound = nullptr;
        take_from_ael(e);
        e = next;
      }
      else if (*it < e->curr_x)
        ++it;
      else e = e->next_in_ael;
    }

    for (auto e : reinsert_list) insert_into_ael(e);
  }

  template<typename Iter>
  void write_path(path_traverse_funcs write_func, void* output, point& last_pt,
    Iter begin, Iter end)
  {
    auto cur = begin;
    while (cur != end) {
      segment* seg_ = *cur;

      switch (seg_->get_type()) 
      {
      case seg_type::cubicto:
      {
        auto cubicto_ = (seg_cubicto*)(seg_);
        write_func.cubic_to(last_pt, cubicto_->ctrl1, cubicto_->ctrl2, cubicto_->target, output);
        break;
      }
      default:
        write_func.line_to(last_pt, seg_->get_target(), output);
        break;
      }
      last_pt = seg_->get_target();

      ++cur;
    }
  }

  void clipper::build_output(path_traverse_funcs write_func, void* output)
  {
    for (auto out : output_list) {
      if (out->is_complete) {
        if (out->up_path.empty() && out->down_path.empty())
          continue;

        point last(0);
        write_func.move_to(last, out->origin, output);
        last = out->origin;

        write_path(write_func, output, last,
          out->up_path.begin(), out->up_path.end());
        write_path(write_func, output, last,
          out->down_path.rbegin(), out->down_path.rend());
      }
    }
  }

  // 到达顶点close了，left 和 right 都会从 obl 中移除
  void clipper::join_output(out_bound* a, out_bound* b, num y)
  {
    if (a->curr_edge) a->curr_edge->bound = nullptr;
    if (b->curr_edge) b->curr_edge->bound = nullptr;
    a->curr_edge = nullptr;
    b->curr_edge = nullptr;

    auto aout = a->owner;
    auto bout = b->owner;

    if (a->stop_x != b->stop_x) {
      if (a->is_up()) {
        aout->up_path.push_back(new_lineto(point(b->stop_x, y)));
      }
      else {
        aout->down_path.push_back(new_lineto(point(a->stop_x, y)));
      }
    }

    if (aout == bout) {
      aout->is_complete = true;
      delete_obl_bound(a);
      delete_obl_bound(b);
      return;
    }

    out_bound* dst_b = a;
    out_bound* src_b = b;
    if (aout->idx > bout->idx) {
      std::swap(dst_b, src_b);
    }
    auto dst_out = dst_b->owner;
    auto src_out = src_b->owner;

    if (dst_b->is_up()) {
      // move src to dst front;
      dst_out->up_path.insert(dst_out->up_path.end(),
        src_out->down_path.rbegin(), src_out->down_path.rend());
      dst_out->up_path.insert(dst_out->up_path.end(),
        src_out->up_path.begin(), src_out->up_path.end());

      dst_out->up_bound = src_out->up_bound;
      dst_out->up_bound->owner = dst_out;
    }
    else {
      dst_out->down_path.insert(dst_out->down_path.end(),
        src_out->up_path.rbegin(), src_out->up_path.rend());
      dst_out->down_path.insert(dst_out->down_path.end(),
        src_out->down_path.begin(), src_out->down_path.end());

      dst_out->down_bound = src_out->down_bound;
      dst_out->down_bound->owner = dst_out;
    }

    src_out->owner = dst_out;
    src_out->up_path.clear();
    src_out->down_path.clear();
    src_out->up_bound = nullptr;
    src_out->down_bound = nullptr;
    src_out->is_complete = true;

    delete_obl_bound(a);
    delete_obl_bound(b);
  }

  // a、b 的wind_dx必定相反
  void clipper::new_output(edge* a, edge* b)
  {
    auto output = new_out_polygon();
    output->idx = output_list.size();
    output_list.push_back(output);
    output->owner = nullptr; // ToDo 这里可以开始计算polytree了

    auto up = new_bound();
    up->owner = output;
    up->curr_edge = a;
    a->bound = up;
    up->wind_dx = a->wind_dx;
    up->stop_x = a->curr_x;

    auto down = new_bound();
    down->owner = output;
    down->curr_edge = b;
    b->bound = down;
    down->wind_dx = b->wind_dx;
    down->stop_x = b->curr_x;

    if (!up->is_up()) {
      std::swap(up, down);
    }

    output->up_bound = up;
    output->down_bound = down;

    output->origin = down->curr_edge->bot;
    if (up->curr_edge->bot != down->curr_edge->bot) {
      output->up_path.push_back(new_lineto(up->curr_edge->bot));
    }
  }

  // bound->curr_edge must be nullptr
  void clipper::update_bound(out_bound* bound, edge* new_edge)
  {
    if (new_edge->curr_x != bound->stop_x) {
      if (bound->is_up()) {
        bound->owner->up_path.push_back(new_lineto(new_edge->bot));
      }
      else {
        bound->owner->down_path.push_back(
          new_lineto(point(bound->stop_x, new_edge->bot.y))
        );
      }
    }

    bound->curr_edge = new_edge;
    new_edge->bound = bound;
  }

  out_bound* clipper::new_bound()
  {
    auto b = out_bound_pool.malloc();
    construct_at<out_bound>(b);
    return b;
  }

  out_polygon* clipper::new_out_polygon()
  {
    auto p = out_polygon_pool.malloc();
    construct_at<out_polygon>(p);
    return p;
  }

  local_minima* clipper::new_local_minima(vertex* v, path_type pt, bool open)
  {
    auto la = local_minima_pool.malloc();
    construct_at<local_minima>(la, v, pt, open);
    return la;
  }

  void clipper::delete_obl_bound(out_bound* b)
  {
    auto prev = b->prev_in_obl;
    auto next = b->next_in_obl;

    if (prev) prev->next_in_obl = next;
    else obl_first = next;

    if (next) next->prev_in_obl = prev;
  }

  // return next curr_edge in ael
  edge* clipper::do_maxima(edge* e)
  {
    if (is_hot(e)) {
      e->bound->curr_edge = nullptr;
      e->bound = nullptr;
    }
    return delete_active_edge(e);
  }

  edge* clipper::delete_active_edge(edge* e)
  {
    auto next = e->next_in_ael;
    auto prev = e->prev_in_ael;

    if (next) next->prev_in_ael = prev;
    if (prev) prev->next_in_ael = next;
    else ael_first = next;

    return next;
  }

  // 关键部分之一，只能传入 ael 中的 curr_edge
  // curr_edge 的 windcount 指的是 curr_edge 左右两边的区域的环绕数，两个相邻区域的环绕数最多差1
  void clipper::set_windcount_closed(edge* e)
  {
    edge* e2 = e->prev_in_ael;
    //find the nearest closed path curr_edge of the same PolyType in AEL (heading left)
    path_type pt = get_polytype(e);
    while (e2 && (get_polytype(e2) != pt || is_open(e2))) e2 = e2->prev_in_ael;

    if (!e2)
    {
      // 最左为空则取决于自身方向
      e->wind_cnt = e->wind_dx;
      e2 = ael_first;
    }
    else if (fillrule_ == fill_rule::even_odd)
    {
      // 奇偶原则使得相邻区域里外相反，间隔区域例外相同
      e->wind_cnt = e->wind_dx;
      e->wind_cnt2 = e2->wind_cnt2;
      e2 = e2->next_in_ael;
    }
    else
    {
      //NonZero, positive, or negative filling here ...
      //if e's WindCnt is in the SAME direction as its WindDx, then polygon
      //filling will be on the right of 'e'.
      //NB neither e2.WindCnt nor e2.WindDx should ever be 0.
      if (e2->wind_cnt * e2->wind_dx < 0)
      {
        // 左边的边朝向和环绕数反向的话

        //opposite directions so 'e' is outside 'e2' ...
        if (abs(e2->wind_cnt) > 1)
        {
          //outside prev poly but still inside another.
          if (e2->wind_dx * e->wind_dx < 0)
            //reversing direction so use the same WC
            e->wind_cnt = e2->wind_cnt;
          else
            //otherwise keep 'reducing' the WC by 1 (ie towards 0) ...
            e->wind_cnt = e2->wind_cnt + e->wind_dx;
        }
        else
          //now outside all polys of same polytype so set own WC ...
          e->wind_cnt = (is_open(e) ? 1 : e->wind_dx);
      }
      else
      {
        // 左边的边朝向和环绕数同向的话

        //'e' must be inside 'e2'
        if (e2->wind_dx * e->wind_dx < 0)
          // 朝向相反意味着他们的 wind_cnt 都指向同一个区域
          //reversing direction so use the same WC
          e->wind_cnt = e2->wind_cnt;
        else
          //otherwise keep 'increasing' the WC by 1 (ie away from 0) ...
          e->wind_cnt = e2->wind_cnt + e->wind_dx;
      }
      e->wind_cnt2 = e2->wind_cnt2;
      e2 = e2->next_in_ael;  // ie get ready to calc WindCnt2
    }

    //update wind_cnt2 ...
    // 往左循环，逐一增加相反类型的wind_dx
    if (fillrule_ == fill_rule::even_odd)
      while (e2 != e)
      {
        if (get_polytype(e2) != pt && !is_open(e2))
          e->wind_cnt2 = (e->wind_cnt2 == 0 ? 1 : 0);
        e2 = e2->next_in_ael;
      }
    else
      while (e2 != e)
      {
        if (get_polytype(e2) != pt && !is_open(e2))
          e->wind_cnt2 += e2->wind_dx;
        e2 = e2->next_in_ael;
      }
  }

  void clipper::intersect(vertex* const& l, vertex* const& r)
  {

    auto rseg = r->next_seg;
    auto lseg = l->next_seg;

    auto rt = r->next_seg->get_type();
    auto lt = l->next_seg->get_type();

    uint32_t t = flags(rt) & flags(lt);

    switch (t)
    {
    case flags(seg_type::lineto):
    {
      auto res = rmath::intersect(l->pt, lseg->get_target(), r->pt, rseg->get_target());
      for (int i = 0; i < res.count; ++i) {
        auto& data = res.data[i];
        if (zero < data.t0 && data.t0 < one) break_info_list.push_back({ l,data.p,data.t0 });
        if (zero < data.t1 && data.t1 < one) break_info_list.push_back({ r,data.p,data.t1 });
      }
      break;
    }
    case flags(seg_type::lineto)& flags(seg_type::cubicto):
    {
      vertex* line, * cubic;
      if (lt == seg_type::lineto) { line = l; cubic = r; }
      else { line = r; cubic = l; }

      rmath::line _line{ line->pt, line->next_seg->get_target() };
      auto _cubic_to = (const seg_cubicto*)(cubic->next_seg);
      auto _curve = _cubic_to->get_cubic(cubic->pt);
      auto res = rmath::intersect(_line, _curve);
      for (int i = 0; i < res.count; ++i) {
        auto& data = res.data[i];
        if (zero < data.t0 && data.t0 < one) break_info_list.push_back({ line,data.p,data.t0 });
        if (zero < data.t1 && data.t1 < one) break_info_list.push_back({ cubic,data.p,data.t1 });
      }
      break;
    }
    case flags(seg_type::cubicto):
    {
      auto c1 = (const seg_cubicto*)(lseg);
      auto c2 = (const seg_cubicto*)(rseg);

      auto res = rmath::intersect(c1->get_cubic(l->pt), c2->get_cubic(r->pt), 40);
      for (int i = 0; i < res.count; ++i) {
        auto& data = res.data[i];
        if (zero < data.t0 && data.t0 < one) break_info_list.push_back({ l,data.p,data.t0 });
        if (zero < data.t1 && data.t1 < one) break_info_list.push_back({ r,data.p,data.t1 });
      }
      break;
    }
    default:
      break;
    }

  }
}
