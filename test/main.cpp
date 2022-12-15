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

#include "debug_util.h"
#include "web_server.h"

class QtView : public QGraphicsView
{
public:
    QtView(QGraphicsScene* scene = nullptr, QWidget* parent = nullptr) : QGraphicsView(scene, parent){
        setBaseSize(800, 600);
        setDragMode(QGraphicsView::DragMode::ScrollHandDrag);
    }

    

protected:


};

QtView* view = nullptr;

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    QMainWindow mainwindow;

    auto scene = debug_util::get_debug_scene();

    view = new QtView(scene);
    view->setAcceptDrops(true);
    mainwindow.setCentralWidget(view);
    mainwindow.show();

    web_server_controller web_app;
    web_app.start();

    return app.exec();
}
