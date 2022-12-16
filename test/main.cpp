#include "river.h"

#include <qapplication.h>
#include <QMainWindow>
#include <QVBoxLayout>
#include <qboxlayout.h>
#include <QLabel>
#include <qgraphicsscene.h>
#include <qlabel.h>
#include <qimage.h>
#include <qgraphicsview.h>
#include <QThread>
#include <qtmetamacros.h>
#include <qwidget.h>
#include "qpushbutton.h"

#include "debug_util.h"
#include "web_server.h"
#include "qtview.h"
#include "central_widget.h"



int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    QMainWindow mainwindow;

    mainwindow.setCentralWidget(new central_widget);
    mainwindow.show();

    web_server_controller web_app;
    web_app.start();

    return app.exec();
}
