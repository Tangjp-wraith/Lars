#include "lars_reactor/tcp_server.h"

int main() {
  EventLoop loop;
  TcpServer server(&loop, "127.0.0.1", 8080);
  loop.EventProcess();
  return 0;
}