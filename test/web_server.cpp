#include "web_server.h"

#include "drogon/drogon.h"
#include <drogon/HttpAppFramework.h>
#include <qthread.h>
#include <QDebug>

void run_web_server(const web_server_setting& setting)
{
    drogon::app()
        .setLogPath("./")
        .setLogLevel(trantor::Logger::kWarn)
        .addListener("127.0.0.1", 5569)
        .setThreadNum(16)
        // .enableRunAsDaemon()
        .run();
}

web_server_controller::~web_server_controller()
{
    if(worker){
        worker->quit_loop();
        worker->quit();
    }
}

void web_server_controller::start()
{
    if(worker) {
        return;
    }

    worker = new web_server_worker();
    connect(worker, &web_server_worker::finished, worker, &QObject::deleteLater);
    worker->start();
}

web_server_worker::web_server_worker(QObject* parent)
    : QThread(parent)
{
    
}

web_server_worker::~web_server_worker()
{
    qDebug() << "~web_server_worker";
}

void web_server_worker::run()
{
    run_web_server(web_server_setting());
}

void web_server_worker::quit_loop()
{
    if(isRunning()){
        drogon::app().quit();
    }
}
