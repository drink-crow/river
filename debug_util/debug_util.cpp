#include "debug_util.h"

#include <qiodevicebase.h>
#include <curl/curl.h>

namespace debug_util {
    QGraphicsScene* debug_scene = nullptr;

    QGraphicsScene* get_debug_scene()
    {
        if(debug_scene == nullptr){
            debug_scene =  new QGraphicsScene;
        }
        return debug_scene;
    }

    

    bool connect()
    {
        curl_global_init(CURL_GLOBAL_ALL);
        
        return true;
    }

    void disconnect()
    {
        curl_global_cleanup();
    }

    size_t read_callback(char* ptr, size_t size, size_t nmemb, void* userdata)
    {
        auto steam = (QDataStream*)userdata;
        return steam->readRawData(ptr, size*nmemb);
        
    }


    void show_line(const QLineF& l, const QPen& pen, const QBrush& brush)
    {
        QByteArray buffer;
        QDataStream stream(&buffer, QIODeviceBase::WriteOnly);
        stream << l << pen << brush;

        //auto base64 = buffer.toBase64();

        CURL* curl = curl_easy_init();

        curl_easy_setopt(curl, CURLOPT_URL, "127.0.0.1:5569/debug_util/v1/user/path");
        //curl_easy_setopt(curl, CURLOPT_POST, 1L);
        //curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, 5L);
        //curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "data");

        /* pass in a pointer to the data - libcurl will not copy */



        //curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);

        /* enable uploading */
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

        /* now specify which pointer to pass to our callback */
        FILE* fd = fopen("C:/Users/wei/Desktop/kill.js", "r");
        struct stat file_info;
        fstat(fileno(fd), &file_info);
        curl_easy_setopt(curl, CURLOPT_READDATA, fd);
        curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)file_info.st_size);

        //struct curl_httppost *formpost = 0;
        //struct curl_httppost *lastptr  = 0;
        //curl_formadd(&formpost, &lastptr, CURLFORM_PTRNAME, "file", CURLFORM_PTRCONTENTS, base64.data(), CURLFORM_CONTENTSLENGTH, base64.size(), CURLFORM_CONTENTTYPE, "application/octet-stream", CURLFORM_END);


        //curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);

        //curl_mime* multipart = curl_mime_init(curl);
        //curl_mimepart* part = curl_mime_addpart(multipart);
        //curl_mime_data(part, buffer.data(), buffer.size());
        //curl_mime_filename(part, "image.png");
        //curl_mime_type(part, "image/png");

 
        //curl_mime_subparts
        //curl_mime_filedata(part, "kill.js");
        //part = curl_mime_addpart(multipart);
        //curl_mime_name(part, "project");
        //curl_mime_data(part, "curl", CURL_ZERO_TERMINATED);
        //part = curl_mime_addpart(multipart);
        //curl_mime_name(part, "logotype-image");
        //curl_mime_filedata(part, "curl.png");

        //curl_easy_setopt(curl, CURLOPT_MIMEPOST, multipart);

        auto res = curl_easy_perform(curl);
        //curl_formfree(formpost);        
        curl_easy_cleanup(curl);
    }

    void show_rect(const QRectF& r, const QPen& pen, const QBrush& brush)
    {

    }
}
