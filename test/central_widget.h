#pragma once

#include <qwidget.h>
#include "qpushbutton.h"
#include "qlabel.h"

class central_widget : public QWidget {
  Q_OBJECT
public:
  central_widget(QWidget* parent = nullptr);

public slots:
  void clear();
  void post_message(QString msg);

private:
  QLabel* msg_lb_;
};