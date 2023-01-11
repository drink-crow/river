#pragma once

#include <drogon/HttpController.h>
#include <drogon/HttpTypes.h>

using namespace drogon;

namespace debug_util
{
  namespace v1
  {
    class User : public drogon::HttpController<User>
    {
    public:
      METHOD_LIST_BEGIN
        // use METHOD_ADD to add your custom processing function here;
        // 参考命令 curl ip:port/debug_util/v1/user/line -F "file=@abc.txt"
        METHOD_ADD(User::get_file, "/{1}", Post);
      METHOD_LIST_END
        // your declaration of processing function maybe like this:
        void get_file(const HttpRequestPtr& req,
          std::function<void(const HttpResponsePtr&)>&& callback, const std::string& type);
    };
  }
}