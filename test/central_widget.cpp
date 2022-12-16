#include "central_widget.h"

#include "QVBoxLayout"
#include "QHBoxLayout"
#include "QPushButton"
#include "qcheckbox.h"

#include "main_window.h"
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
    hlayout->addWidget(btn_clear);
    connect(btn_clear, &QPushButton::clicked,this, &central_widget::clear);

    auto pin = new QCheckBox("pin");
    hlayout->addWidget(pin);
    connect(pin, &QCheckBox::clicked, [](bool checked) {
        mainwindow->setWindowFlag(Qt::WindowStaysOnTopHint, checked);
        });
    pin->click();
}

void central_widget::clear(){
    view->scene()->clear();
}
