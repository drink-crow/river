#include "debug_util.h"

#include <qiodevicebase.h>
#include <qiodevice.h>
#include <QDebug>
#include <QString>

#include <curl/curl.h>



namespace debug_util {
    bool connect()
    {
        curl_global_init(CURL_GLOBAL_ALL);
        
        return true;
    }

    void disconnect()
    {
        curl_global_cleanup();
    }

    int my_trace(CURL* handle, curl_infotype type,
        char* data, size_t size,
        void* userp)
    {
        switch (type) {
        case CURLINFO_TEXT:
            qDebug() << "== Info:";
            break;
            /* FALLTHROUGH */
        case CURLINFO_HEADER_OUT:
            qDebug() << "=> Send header";
            break;
        case CURLINFO_DATA_OUT:
            qDebug() << "=> Send data";
            break;
        case CURLINFO_SSL_DATA_OUT:
            qDebug() << "=> Send SSL data";
            break;
        case CURLINFO_HEADER_IN:
            qDebug() << "<= Recv header";
            break;
        case CURLINFO_DATA_IN:
            qDebug() << "<= Recv data";
            break;
        case CURLINFO_SSL_DATA_IN:
            qDebug() << "<= Recv SSL data";
            break;
        default: /* in case a new one is introduced to shock us */
            return 0;
        }

        qDebug().noquote() << QString::fromLocal8Bit(data, size);
        return 0;
    }

    void set_curl_debug(CURL* curl)
    {
        curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, my_trace);
        curl_easy_setopt(curl, CURLOPT_DEBUGDATA, nullptr);
        /* the DEBUGFUNCTION has no effect until we enable VERBOSE */
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        /* example.com is redirected, so we tell libcurl to follow redirection */
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    }

    void push_data(const char* data, size_t size, const char* url)
    {
        CURL* curl = curl_easy_init();

        //set_curl_debug(curl);

        curl_mime* multipart = curl_mime_init(curl);
        curl_mimepart* part = curl_mime_addpart(multipart);
        curl_mime_data(part, data, size);
        curl_mime_filename(part, "file");
        curl_mime_name(part, "path.bin");
        curl_mime_type(part, "multipart/form-data");
        curl_easy_setopt(curl, CURLOPT_MIMEPOST, multipart);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        auto res = curl_easy_perform(curl);
        curl_mime_free(multipart);
        curl_easy_cleanup(curl);
    }

    void show_line(const QLineF& l, const QPen& pen)
    {
        QByteArray buffer;
        QDataStream stream(&buffer, QIODeviceBase::WriteOnly);
        stream << l << pen;

        push_data(buffer.data(), buffer.size(), "127.0.0.1:5569/debug_util/v1/user/line");
    }

    void show_rect(const QRectF& r, const QPen& pen, const QBrush& brush)
    {
        QByteArray buffer;
        QDataStream stream(&buffer, QIODeviceBase::WriteOnly);
        stream << r << pen << brush;

        push_data(buffer.data(), buffer.size(), "127.0.0.1:5569/debug_util/v1/user/rect");

    }

    void show_cubic(const QPointF& p0, const QPointF& p1, const QPointF& p2, const QPointF& p3, const QPen& pen)
    {
        QByteArray buffer;
        QDataStream stream(&buffer, QIODeviceBase::WriteOnly);
        stream << p0 << p1 << p2 << p3 << pen;

        push_data(buffer.data(), buffer.size(), "127.0.0.1:5569/debug_util/v1/user/cubic");
    }

    void show_point(const QPointF& p, const QPen& pen)
    {
        QByteArray buffer;
        QDataStream stream(&buffer, QIODeviceBase::WriteOnly);
        stream << p << pen;

        push_data(buffer.data(), buffer.size(), "127.0.0.1:5569/debug_util/v1/user/point");
    }
}
