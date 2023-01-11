#pragma once

#include <qobject.h>
#include <qthread.h>
#include <qtmetamacros.h>
#include <string>
#include <QThread>

struct web_server_setting
{
  std::string log_path;
};

class web_server_worker : public QThread
{
  Q_OBJECT

public:
  web_server_worker(QObject* parent = nullptr);
  ~web_server_worker() override;

  void run() override;
  void quit_loop();
};

class web_server_controller : public QObject
{
public:
  ~web_server_controller() override;
  void start();
private:
  web_server_worker* worker = nullptr;
};