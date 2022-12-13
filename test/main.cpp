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

#include "debug_util.h"

class QtView : public QGraphicsView
{
public:
    QtView(QGraphicsScene* scene = nullptr, QWidget* parent = nullptr) : QGraphicsView(scene, parent){
        setBaseSize(800, 600);

    }

    

protected:


};

QtView* view = nullptr;

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    QMainWindow mainwindow;

    auto scene = debug_util::get_debug_scene();
    scene->addText("const QString &text");

    view = new QtView(scene);
    mainwindow.setCentralWidget(view);
    mainwindow.show();

    return app.exec();
}
