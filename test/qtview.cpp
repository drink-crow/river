#include "qtview.h"

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
}

void QtView::clear()
{
    scene()->clear();
    scene()->addRect(QRect(-250,-250,500,500));
}

void QtView::draw_line(const QLineF& l, const QPen& pen)
{
    scene()->addLine(l,pen);
}

void QtView::draw_rect(const QRectF& r, const QPen& pen, const QBrush& brush)
{
    scene()->addRect(r,pen,brush);
}
