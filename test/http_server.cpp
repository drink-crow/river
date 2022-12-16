#include "http_server.h"
#include <QString>
#include <qbrush.h>
#include <qpen.h>

#include "qtview.h"

using namespace debug_util::v1;

// Add definition of your processing function here

void User::login(const HttpRequestPtr &req,
                 std::function<void (const HttpResponsePtr &)> &&callback,
                 std::string &&userId,
                 const std::string &password)
{
    LOG_DEBUG<<"User "<<userId<<" login";
    //Authentication algorithm, read database, verify, identify, etc...
    //...
    Json::Value ret;
    ret["result"]="ok";
    ret["token"]=drogon::utils::getUuid();
    auto resp=HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}
void User::getInfo(const HttpRequestPtr &req,
                   std::function<void (const HttpResponsePtr &)> &&callback,
                   std::string userId,
                   const std::string &token) const
{
    LOG_DEBUG<<"User "<<userId<<" get his information";

    //Verify the validity of the token, etc.
    //Read the database or cache to get user information
    Json::Value ret;
    ret["result"]="ok";
    ret["user_name"]="Jack";
    ret["user_id"]=userId;
    ret["gender"]=1;
    auto resp=HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}

void User::draw_path(const HttpRequestPtr &req,
                 std::function<void (const HttpResponsePtr &)> &&callback)
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

    auto &file = fileUpload.getFiles()[0];
    QByteArray buffer(file.fileData(), file.fileLength());
    view->paser_file_thread(buffer, "line");

    auto resp = HttpResponse::newHttpResponse();
    resp->setBody("success\n");
    callback(resp);
}
