#pragma once

#include "debug_util.h"
#include "rmath.h"

#if RIVER_GRAPHICS_DEBUG

QPointF toqt(const rmath::vec2& p);

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

    auto rect() const { return QRectF(QPointF(0,0), space).translated(move); }
    auto operator()() const { return move; }
    auto operator()(const QLineF& l) const { return l.translated(move); }
    auto operator()(const QPointF& p) const { return p + move; }
};
#endif
