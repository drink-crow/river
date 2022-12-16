#include "central_widget.h"

#include "QVBoxLayout"
#include "QHBoxLayout"
#include "QPushButton"

#include "qtview.h"

central_widget::central_widget(QWidget* parent) : QWidget(parent)
{
    view = new QtView(new QGraphicsScene);
    // view->setAcceptDrops(true);
    auto vlayout = new QVBoxLayout(this);
    vlayout->addWidget(view);
    auto hlayout = new QHBoxLayout;
    vlayout->addLayout(hlayout);

    auto btn_clear = new QPushButton("clear");
    vlayout->addWidget(btn_clear);

    connect(btn_clear, &QPushButton::clicked,this, &central_widget::clear);
}

void central_widget::clear(){
    view->scene()->clear();
}
