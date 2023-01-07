#include "qtview.h"
#include <qbrush.h>
#include <qpainterpath.h>
#include <qapplication.h>
#include <qdebug.h>

QtView* view = nullptr;

QtView::QtView(QGraphicsScene* scene, QWidget* parent) : QGraphicsView(scene, parent)
{
        setBaseSize(800, 600);
        setDragMode(QGraphicsView::DragMode::ScrollHandDrag);
        
        scale(1, -1);

        connect(this, &QtView::new_file, this, &QtView::paser_file);
        clear();
}

void QtView::paser_file_thread(QByteArray buffer, QString type)
{
    emit new_file(buffer, type);
}

void QtView::paser_file(QByteArray buffer, QString type)
{
    if(type == "line"){
        QLineF line;
        QPen pen;
        QDataStream steam(buffer);
        steam >> line >> pen ;

        draw_line(line,pen);
    }
    else if (type == "rect") {
        QRectF rect;
        QPen pen;
        QBrush brush;
        QDataStream steam(buffer);
        steam >> rect >> pen >> brush;
        draw_rect(rect,pen,brush);
    }
    else if (type == "cubic") {
        QPointF p0, p1, p2, p3;
        QPen pen;
        QDataStream steam(buffer);
        steam >> p0 >> p1 >> p2 >> p3 >> pen;
        draw_cubic(p0, p1, p2, p3, pen);
    }
    else if (type == "point") {
        QPointF p;
        QPen pen;
        QDataStream steam(buffer);
        steam >> p >> pen;
        draw_point(p, pen);
    }
}

void QtView::clear()
{
    scene()->clear();
    // scene()->addRect(QRect(-250,-250,500,500));
}

void QtView::draw_line(const QLineF& l, const QPen& pen)
{
    scene()->addLine(l,pen);
}

void QtView::draw_rect(const QRectF& r, const QPen& pen, const QBrush& brush)
{
    scene()->addRect(r,pen,brush);
}

void QtView::draw_cubic(const QPointF& p0, const QPointF& p1, const QPointF& p2, const QPointF& p3, const QPen& pen)
{
    QPainterPath path;
    path.moveTo(p0);
    path.cubicTo(p1, p2, p3);
    scene()->addPath(path, pen);
}

void QtView::draw_point(const QPointF& p, const QPen& pen)
{
    QPointF offset(2, 2);
    QRectF rect(p - offset, p + offset);
    QBrush brush(pen.color());
    brush.setStyle(Qt::SolidPattern);
    scene()->addEllipse(rect, pen, brush);
}

void QtView::wheelEvent(QWheelEvent* event)
{
    auto modifierPressed = QApplication::keyboardModifiers();
    if (modifierPressed & Qt::AltModifier) {
        auto sc = 1. + event->angleDelta().x() * 0.0005;
        if(sc!=1)
            scale(sc, sc);
    }
    else {
        return QGraphicsView::wheelEvent(event);
    }
}
