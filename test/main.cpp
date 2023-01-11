#include <qapplication.h>

#include "web_server.h"
#include "main_window.h"


int main(int argc, char** argv)
{
  QApplication app(argc, argv);
  main_window mainwindow;
  mainwindow.show();

  web_server_controller web_app;
  web_app.start();

  return app.exec();
}
