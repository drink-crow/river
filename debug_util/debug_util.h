#pragma once

#include "qgraphicsscene.h"
#include <qbrush.h>

namespace debug_util {
    QGraphicsScene* get_debug_scene();

    bool connect();
    void disconnect();

    void show_line(const QLineF& l, const QPen& pen = QPen());
    void show_rect(const QRectF& r, const QPen& pen = QPen(), const QBrush& brush = QBrush());
}


