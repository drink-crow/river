#pragma once

#include <qwidget.h>
#include "qpushbutton.h"

class central_widget : public QWidget{
    Q_OBJECT
public:
    central_widget(QWidget* parent = nullptr);

public slots:
    void clear();

private:

};