#include "util.h"

#if RIVER_GRAPHICS_DEBUG

QPointF toqt(const rmath::vec2& p) {
    return QPointF(p.x, p.y);
}

#endif
