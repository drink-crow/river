#pragma once

#include "qline.h"
#include "qpen.h"
#include "qbrush.h"
#include "QGraphicsScene"
#include "QGraphicsView"


class QtView : public QGraphicsView
{
    Q_OBJECT
public:
    QtView(QGraphicsScene* scene = nullptr, QWidget* parent = nullptr);

    void paser_file_thread(QByteArray buffer, QString type);
signals:
    void new_file(QByteArray buffer, QString type);

public slots:
    void paser_file(QByteArray buffer, QString type);

    void clear();
    void draw_line(const QLineF& l, const QPen& pen);
    void draw_rect(const QRectF& r, const QPen& pen, const QBrush& brush);
    void draw_cubic(const QPointF& p0, const QPointF& p1, const QPointF& p2, const QPointF& p3, const QPen& pen = QPen());
    void draw_point(const QPointF& p, const QPen& pen = QPen());

protected:
};

extern QtView* view;
