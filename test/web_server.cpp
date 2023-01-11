#include "web_server.h"

#include "drogon/drogon.h"
#include <drogon/HttpAppFramework.h>
#include <qthread.h>
#include <QDebug>

#include "http_server.h"

void run_web_server(const web_server_setting& setting)
{
  // drogon::app().registerHandler(
  //     "/",
  //     [](const HttpRequestPtr &,
  //        std::function<void(const HttpResponsePtr &)> &&callback) {
  //         auto resp = HttpResponse::newHttpViewResponse("FileUpload");
  //         callback(resp);
  //     });

  // drogon::app().registerHandler(
  //     "/upload_endpoint",
  //     [](const HttpRequestPtr &req,
  //        std::function<void(const HttpResponsePtr &)> &&callback) {
  //         MultiPartParser fileUpload;
  //         if (fileUpload.parse(req) != 0 || fileUpload.getFiles().size() != 1)
  //         {
  //             auto resp = HttpResponse::newHttpResponse();
  //             resp->setBody("Must only be one file");
  //             resp->setStatusCode(k403Forbidden);
  //             callback(resp);
  //             return;
  //         }

  //         auto &file = fileUpload.getFiles()[0];
  //         auto md5 = file.getMd5();
  //         auto resp = HttpResponse::newHttpResponse();
  //         resp->setBody(
  //             "The server has calculated the file's MD5 hash to be " + md5);
  //         file.save();
  //         LOG_INFO << "The uploaded file has been saved to the ./uploads "
  //                     "directory";
  //         callback(resp);
  //     },
  //     {Post});

  drogon::app()
    .setLogPath("./")
    .setLogLevel(trantor::Logger::kWarn)
    .addListener("127.0.0.1", 5569)
    .setThreadNum(16);

  drogon::app().run();
}

web_server_controller::~web_server_controller()
{
  if (worker) {
    worker->quit_loop();
    worker->quit();
  }
}

void web_server_controller::start()
{
  if (worker) {
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
  if (isRunning()) {
    drogon::app().quit();
  }
}
