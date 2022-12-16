#include "main_window.h"
#include "central_widget.h"

main_window* mainwindow = nullptr;

main_window::main_window()
{
    mainwindow = this;
    setCentralWidget(new central_widget);
}
