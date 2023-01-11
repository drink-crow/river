#include "http_server.h"
#include <QString>
#include <qbrush.h>
#include <qpen.h>

#include "qtview.h"

using namespace debug_util::v1;

// Add definition of your processing function here

void User::get_file(const HttpRequestPtr& req,
  std::function<void(const HttpResponsePtr&)>&& callback, const std::string& type)
{
  MultiPartParser fileUpload;
  if (fileUpload.parse(req) != 0 || fileUpload.getFiles().size() != 1)
  {
    auto resp = HttpResponse::newHttpResponse();
    resp->setBody("VVVVVVVVVVVVVV\nMust only be one file\n^^^^^^^^^^^^^\n");
    resp->setStatusCode(k403Forbidden);
    callback(resp);
    return;
  }

  auto& file = fileUpload.getFiles()[0];
  QByteArray buffer(file.fileData(), file.fileLength());
  view->paser_file_thread(buffer, type.c_str());

  auto resp = HttpResponse::newHttpResponse();
  resp->setBody("success\n");
  callback(resp);
}
