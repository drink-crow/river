#include "main_window.h"
#include "central_widget.h"

main_window* mainwindow = nullptr;

main_window::main_window()
{
  mainwindow = this;
  setCentralWidget(central_widget_ = new central_widget);
}

void main_window::post_message(QString msg)
{
  central_widget_->post_message(msg);
}
