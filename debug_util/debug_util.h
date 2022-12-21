#pragma once

#include "qgraphicsscene.h"
#include <qbrush.h>

namespace debug_util {
    bool connect();
    void disconnect();

    void show_line(const QLineF& l, const QPen& pen = QPen());
    void show_rect(const QRectF& r, const QPen& pen = QPen(), const QBrush& brush = QBrush());
    void show_cubic(const QPointF& p0, const QPointF& p1, const QPointF& p2, const QPointF& p3, const QPen& pen = QPen());
    void show_point(const QPointF& p, const QPen& pen = QPen());
}


