#pragma once

#include "reactor_buffer.h"
#include "event_loop.h"
//一个tcp的连接信息
class TcpConn {
 public:
  //初始化tcp_conn
  TcpConn(int connfd,EventLoop* loop);
  //处理读业务
  void DoRead();
  //处理写业务
  void DoWrite();
  //销毁tcp_conn
  void CleanConn();
  //发送消息的方法
  int SendMessage(const char* data,int msg_len,int msg_id);


 private:
  ///当前链接的fd
  int connfd_;
  ///该连接归属的event_poll
  EventLoop* loop_;
  ///输出buf
  OutputBuffer obuf_;
  ///输入buf
  InputBuffer ibuf_;
};

