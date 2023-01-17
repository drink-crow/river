#pragma once

#include "river.h"
#include "segment.h"

#if RIVER_GRAPHICS_DEBUG
#include <qbrush.h>
#include <qline.h>
#include <qnamespace.h>
#include <qpainterpath.h>
#include <qpen.h>
#include <qpoint.h>
#include <qsize.h>

#include "debug_util.h"

QPointF toqt(const rmath::vec2& p);
QLineF toqt(const rmath::line& l);


struct offset_rect
{
  QPointF move = QPointF(0, 0);
  QPointF space = QPointF(140, 140);
  int cow = 0;

  void next() {
    if (cow == 4) {
      move.rx() += space.x();
      move.ry() = 0;
      cow = 0;
    }
    else {
      ++cow;
      move.ry() += space.y();
    }
  }

  auto rect() const { return QRectF(QPointF(0, 0), space).translated(move); }
  auto operator()() const { return move; }
  auto operator()(const QLineF& l) const { return l.translated(move); }
  auto operator()(const QPointF& p) const { return p + move; }
};

void draw_path(const rmath::line& l, const QPen& pen = QPen(), const QPointF& offset = QPointF(0, 0));
void draw_path(const rmath::bezier_cubic& c, const QPen& pen = QPen(), const QPointF& offset = QPointF(0, 0));
void draw_path(const river::path& show, const QPen& pen = QPen(), const QPointF& offset = QPointF(0, 0));
void draw_path(const river::paths& show, const QPen& pen = QPen(), const QPointF& offset = QPointF(0, 0));

#endif

inline constexpr uint32_t flags(river::seg_type t) {
  return 1 << (uint32_t)t;
}

