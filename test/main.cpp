#include "river.h"

#include <qapplication.h>
#include <QMainWindow>
#include <QVBoxLayout>
#include <qboxlayout.h>
#include <QLabel>
#include <qlabel.h>

class QtView : public QWidget
{
public:
    QtView(QWidget* parent = nullptr) : QWidget(parent){
        setBaseSize(800, 600);
        auto layout = new QVBoxLayout(this);
        layout->addWidget(new QLabel("Hello"));
    }

};

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    QMainWindow mainwindow;

    mainwindow.setCentralWidget(new QtView);
    mainwindow.show();

    return app.exec();
}
