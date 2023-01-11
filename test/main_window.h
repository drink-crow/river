#pragma once

#include "qmainwindow.h"

class central_widget;

class main_window : public QMainWindow
{
  Q_OBJECT
public:
  main_window();

  void post_message(QString msg);
private:
  central_widget* central_widget_ = nullptr;
};

extern main_window* mainwindow;
