#pragma once
#include <netinet/in.h>

#include <iostream>
#include <memory>

#include "event_loop.h"

class TcpServer {
 public:
  TcpServer(EventLoop* loop, const char *ip, uint16_t port);
  ~TcpServer();

  void DoAccept();

 private:
  /// 套接字
  int sockfd_;
  /// 客户端链接地址
  struct sockaddr_in connaddr_ {};
  /// 客户端链接地址长度
  socklen_t addrlen_{};
  /// event_loop epoll事件机制
  EventLoop* loop_;
};