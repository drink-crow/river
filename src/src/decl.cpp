#include "dcel.h"
#include <algorithm>
#include <vector>
#include <cmath>

#include "boost/token_functions.hpp"

#include "river.h"
#include "rmath.h"
#include "scan_line.h"
#include "util.h"

namespace scan_line {
  using namespace river;

  template<>
  struct all_function<dcel::half_edge*>
  {
    typedef ::rmath::rect box;
    typedef double ct;
    typedef dcel::half_edge* key;

    static box get_rect(const key& in)
    {
      return in->get_boundary();
    }

    // 端点重合不用处理
    static void intersect(const key& r, const key& l, void* user)
    {
      auto sort = flags(r->get_seg_type()) & flags(l->get_seg_type());

      switch (sort)
      {
      case flags(seg_type::lineto):
        dcel::intersect((dcel::line_half_edge*)r, (dcel::line_half_edge*)l);
        break;
      default:
        break;
      }
    }
  };
}

namespace dcel {

#if RIVER_GRAPHICS_DEBUG

  QPointF qstart(const half_edge* e) {
    return toqt(e->start->p);
  }
  QPointF qend(const half_edge* e) {
    return toqt(e->end->p);
  }

  QLineF qline(const half_edge* e, bool inverse = false)
  {
    if (!inverse) {
      return QLineF(qstart(e), qend(e));
    }
    else {
      return QLineF(qend(e), qstart(e));
    }
  }

  void qline_arrow(const half_edge* e, const QPen& pen = QPen())
  {
    debug_util::show_line(qline(e), pen);
    vec2 center = (e->start->p + e->end->p) / 2;

    vec2 v = (e->end->p - e->start->p).normalizate() * 5;
    debug_util::show_line(QLineF(toqt(center), toqt(v.rotated(pi * 3 / 4) + center)), pen);
    debug_util::show_line(QLineF(toqt(center), toqt(v.rotated(-pi * 3 / 4) + center)), pen);
  }

#endif

  bool vec2_compare_func(const ::rmath::vec2& r, const ::rmath::vec2& l)
  {
    if (r.x < l.x) return true;
    if (r.x == l.x) return r.y < l.y;
    return false;
  }

  bool point_between_two_point(const vec2& p, const vec2& p0, const vec2& p1)
  {
    return ((p0.x < p.x) && (p.x < p1.x))
      || ((p0.y < p.y) && (p.y < p1.y));
  }

  void intersect(line_half_edge* l, line_half_edge* r)
  {
    const vec2& p0 = l->start->p;
    const vec2& p1 = l->end->p;
    const vec2& p2 = r->start->p;
    const vec2& p3 = r->end->p;

    auto res = intersect(p0, p1, p2, p3);
    if (res.count == 1) {
      r->add_break_point(res.data[0].p);
      l->add_break_point(res.data[0].p);
    }
    else if (res.count == 2) {
      for (int i = 0; i < res.count; ++i) {
        auto& data = res.data[i];
        if (0 < data.t0 && data.t0 < 1) l->add_break_point(data.p);
        if (0 < data.t1 && data.t1 < 1) r->add_break_point(data.p);
      }
    }
  }

  void point::add_edge(half_edge* e, bool is_emit)
  {
    edges.push_back({ e, is_emit });
  }

  void point::remove(half_edge* e)
  {
    edges.erase(std::remove_if(edges.begin(), edges.end(), find_private_edge(e)), edges.end());
  }

  void point::sort()
  {
    struct sort_p
    {
      private_edge edge;
      sort_data data;
    };

    std::vector<sort_p> sort_vector;
    for (auto& e : edges) {
      if (e.is_emit) {
        sort_vector.push_back({ e,e.edge->get_sort_data() });
      }
      else {
        sort_vector.push_back({ e,e.edge->twin->get_sort_data() });
      }
    }

    constexpr auto sort_algw = [](const sort_p& l, const sort_p& r) -> bool
    {
      auto le = l.edge;
      auto re = r.edge;

      if (le.edge->get_seg_type() == seg_type::lineto && re.edge->get_seg_type() == seg_type::lineto)
      {
        if (l.data.angle < r.data.angle) return true;
        if (l.data.angle > r.data.angle) return false;

        if (l.edge.is_emit && !(r.edge.is_emit)) return true;
        if (!(l.edge.is_emit) && r.edge.is_emit) return false;

        return l.edge.edge > r.edge.edge;
      }

      throw "out of sort condition";
      return false;
    };

    std::sort(sort_vector.begin(), sort_vector.end(), sort_algw);
    edges.clear();
    for (auto& e : sort_vector) {
      edges.push_back(e.edge);
    }
  }

  half_edge* point::get_next(const half_edge* e) const
  {
    auto it = std::find_if(edges.begin(), edges.end(), find_private_edge(e));
    if (it == edges.end()) {
      return nullptr;
    }

    constexpr auto next = [](decltype(it)& iter, const decltype(edges)& container) {
      ++iter;
      if (iter == container.end()) {
        iter = container.begin();
      }
    };

    next(it, edges);
    while (it->is_emit == false || it->edge->prev) {
      next(it, edges);
    }

    return it->edge;
  }

  half_edge* point::get_prev(const half_edge* e) const
  {
    auto it = std::find_if(edges.begin(), edges.end(), find_private_edge(e));
    if (it == edges.end()) {
      return nullptr;
    }

    if (it == edges.begin()) {
      return edges.back().edge;
    }
    else {
      --it;
      return it->edge;
    }
  }

  std::vector<half_edge*> line_half_edge::process_break_point(dcel* parent)
  {
    std::vector<vec2> bp{ get_break_points().begin(),get_break_points().end() };
    if (bp.empty()) return {};

    struct compare
    {
      compare(const vec2& start, const vec2& end)
        : dx(end.x >= start.x), dy(end.y >= start.y) {}

      bool operator()(const vec2& r, const vec2& l) const {
        if (dx && r.x < l.x) return true;
        if ((!dx) && r.x > l.x) return true;
        if (dy && r.y < l.y) return true;
        if ((!dy) && r.y > l.y) return true;

        return false;
      }

      const bool dx;
      const bool dy;
    };

    std::sort(bp.begin(), bp.end(), compare(start->p, end->p));

    std::vector<half_edge*> res;

    auto cur_start = start;
    for (auto p : bp)
    {
      auto cur_end = parent->find_or_add_point(p.x, p.y);
      auto line = parent->new_line_edge(cur_start, cur_end);
      res.push_back(line);

      cur_start = cur_end;
    }
    if (bp.size() > 0) {
      res.push_back(parent->new_line_edge(cur_start, end));
    }

    return res;
  }

  half_edge* line_half_edge::build_twin(dcel* parent)
  {
    auto out = parent->new_line_edge(end, start);
    out->twin = this;
    this->twin = out;
    return out;
  }

  sort_data line_half_edge::get_sort_data() const
  {
    sort_data data;

    auto l = end->p - start->p;
    data.angle = std::atan2(l.y, l.x);
    if (data.angle == -rmath::pi) {
      data.angle = -data.angle;
    }

    return data;
  }

  void dcel::set_current_path_type(path_type new_type)
  {
    current_path_type = new_type;
  }

  path_type dcel::get_current_path_type() const
  {
    return current_path_type;
  }

  void dcel::add_line(num x1, num y1, num x2, num y2)
  {
    // 过滤掉点
    if (x1 == x2 && y1 == y2) return;

    point* start_p = find_or_add_point(x1, y1);
    point* end_p = find_or_add_point(x2, y2);

    // 只添加半边，剩下半边等待求交点后再补充
    new_line_edge(start_p, end_p);
  }

  void dcel::process()
  {
    process_break();
    build_twin();
    build_face();
  }

  void dcel::process_break()
  {
    scan_line::scan_line<half_edge*> scan_line;
    for (auto e : edges) {
      scan_line.add_segment(e);
    }
    scan_line.process(nullptr);

    // 这里很重要，应为对 edges 插入删除会扰乱 set 的遍历
    std::vector<half_edge*> tmp_vector(edges.begin(), edges.end());

    for (auto edge : tmp_vector)
    {
      // 这里可能会对 set 进行插入
      auto replace = edge->process_break_point(this);
      if (replace.empty()) {
        continue;
      }

      // 在 set 中移除旧的 edge
      edges.erase(edge);
      edge->start->remove(edge);
      edge->end->remove(edge);
      delete edge;
    }
  }

  void dcel::build_twin()
  {
    // 这里很重要，应为对 edges 插入删除会扰乱 set 的遍历
    std::vector<half_edge*> tmp_vector(edges.begin(), edges.end());
    for (auto e : tmp_vector)
    {
      e->build_twin(this);
    }
  }

  void dcel::build_face()
  {
    for (auto p : point_set)
    {
      p->sort();
    }


    for (auto e : edges)
    {
      if (e->next != nullptr) {
        continue;
      }

      auto f = new face;
      faces.push_back(f);
      f->start = e;

      auto cur_e = e;
      point* loop_start = e->start;
      while (cur_e->next == nullptr) {
        cur_e->face = f;
        auto next = cur_e->end->get_next(cur_e);
        {
          //// 图形调试get_next边的选择
          //qline_arrow(cur_e, QPen(Qt::red));
          //for (auto _e : cur_e->end->edges) {
          //    if (_e.is_emit && _e.edge->prev == nullptr) {
          //        debug_util::show_line(qline(_e.edge));
          //    }
          //}
          //qline_arrow(next, QPen(Qt::blue));
        }
        cur_e->next = next;
        next->prev = cur_e;

        if (next->end == loop_start) {
          next->next = e;
          e->prev = next;
          break;
        }
        cur_e = cur_e->next;
      }
    }

    // 建立好的 face 需要删除重合的部分
    decltype(faces) old_face;
    old_face.swap(faces);
    while (!old_face.empty())
    {
      auto f = old_face.back();
      auto new_in = split_face(f);

      if (new_in) {
        old_face.push_back(new_in);
      }
      else
      {
        old_face.pop_back();
        faces.push_back(f);
      }
    }

#if RIVER_GRAPHICS_DEBUG & 0
    {
      // 图形调试所有的 face
      offset_rect offset;
      offset.space = QPointF(120, 200);
      QPen red_pen(Qt::red);
      for (auto f : faces)
      {
        auto cur = f->start;

        do
        {
          debug_util::show_rect(QRectF(-10, -40, 120, 200).translated(offset()), red_pen);
          debug_util::show_line(offset(qline(cur)));
          vec2 center = (cur->start->p + cur->end->p) / 2;

          vec2 v = (cur->end->p - cur->start->p).normalizate() * 5;
          debug_util::show_line(offset(QLineF(toqt(center), toqt(v.rotated(pi * 3 / 4) + center))), red_pen);
          debug_util::show_line(offset(QLineF(toqt(center), toqt(v.rotated(-pi * 3 / 4) + center))), red_pen);

          cur = cur->next;
        } while (cur != f->start);

        offset.next();
      }
    }
#endif
  }

  face* dcel::split_face(face* f)
  {
    auto start = f->start;
    auto cur = start;
    std::map<point*, half_edge*> point_index;

    do {
      auto it = point_index.find(cur->end);
      if (it == point_index.end()) {
        point_index.insert(std::make_pair(cur->end, cur));
        cur = cur->next;
        continue;
      }

      // 发现重合的部分
      auto left = new face;
      auto e1 = it->second;
      auto e2 = e1->next;
      auto e3 = cur;
      auto e4 = cur->next;

      e1->next = e4;
      e4->prev = e1;

      left->start = e2;
      e2->prev = e3;
      e3->next = e2;

      return left;
    } while (cur != start);
    return nullptr;
  }

  line_half_edge* dcel::new_line_edge(point* start, point* end)
  {
    auto line = new line_half_edge;

    line->start = start;
    line->end = end;
    line->type = current_path_type;

    edges.insert(line);
    start->add_edge(line, true);
    end->add_edge(line, false);
    return line;
  }

  point* dcel::find_or_add_point(num x, num y)
  {
    point* res;

    auto it = point_set.find(vec2(x, y));
    if (it == point_set.end()) {
      res = new point(x, y);
      point_set.insert(res);
    }
    else {
      res = *it;
    }

    return res;
  }

}
