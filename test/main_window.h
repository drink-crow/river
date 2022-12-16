#pragma once

#include "qmainwindow.h"

class main_window : public QMainWindow
{
    Q_OBJECT
public:
    main_window();
};

extern main_window* mainwindow;
