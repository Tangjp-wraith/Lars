#include "lars_reactor/tcp_conn.h"

#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>

#include <csignal>
#include <iostream>

#include "lars_reactor/message.h"
// 回显业务
auto callback_busi = [](const char* data, int len, int msg_id, void* args,
                        TcpConn* conn) {
  conn->SendMessage(data, len, msg_id);
};
// 连接的读事件回调
auto conn_read_callback = [](EventLoop* loop, int fd, void* args) {
  auto conn = static_cast<TcpConn*>(args);
  conn->DoRead();
};
// 连接的写事件回调
auto conn_write_callback = [](EventLoop* loop, int fd, void* args) {
  auto conn = static_cast<TcpConn*>(args);
  conn->DoWrite();
};

TcpConn::TcpConn(int connfd, EventLoop* loop) {
  connfd_ = connfd;
  loop_ = loop;
  // 1. 将connfd设置成非阻塞状态
  int flag = fcntl(connfd_, F_GETFL, 0);
  fcntl(connfd_, F_SETFL, O_NONBLOCK | flag);
  // 2. 设置TCP_NODELAY禁止做读写缓存，降低小包延迟
  int op = 1;
  setsockopt(connfd_, IPPROTO_TCP, TCP_NODELAY, &op, sizeof(op));
  // 3. 将该链接的读事件让event_loop监控
  loop_->AddIoEvent(connfd_, conn_read_callback, EPOLLIN, this);
}

void TcpConn::DoRead() {
  // 1. 从套接字读取数据
  int ret = ibuf_.ReadData(connfd_);
  if (ret == -1) {
    std::cerr << "read data from socket!\n";
    this->CleanConn();
    return;
  } else if (ret == 0) {
    // 对端正常关闭
    std::cerr << "connection closed by peer!\n";
    CleanConn();
    return;
  }
  // 2. 解析msg_head数据
  MsgHead head{};
  //[这里用while，可能一次性读取多个完整包过来]
  while (ibuf_.Length() >= MESSAGE_HEAD_LEN) {
    // 2.1 读取msg_head头部，固定长度MESSAGE_HEAD_LEN
    memcpy(&head, ibuf_.Data(), MESSAGE_HEAD_LEN);
    if (head.msg_len_ > MESSAGE_LENGTH_LIMIT || head.msg_len_ < 0) {
      std::cerr << "data format error, need close, msg_len: " << head.msg_len_
                << std::endl;
      this->CleanConn();
      break;
    }
    if (ibuf_.Length() < MESSAGE_HEAD_LEN + head.msg_len_) {
      // 缓存buf中剩余的数据，小于实际上应该接受的数据
      // 说明是一个不完整的包，应该抛弃
      break;
    }
    // 2.2 再根据头长度读取数据体，然后针对数据体处理 业务
    // TODO 添加包路由模式

    // 头部处理完了，往后偏移MESSAGE_HEAD_LEN长度
    ibuf_.Pop(MESSAGE_HEAD_LEN);
    // 处理ibuf.data()业务数据
    std::cout << "read data: " << ibuf_.Data() << std::endl;
    // 回显业务
    callback_busi(ibuf_.Data(), head.msg_len_, head.msg_id_, nullptr, this);
    // 消息体处理完了,往后便宜msg_len长度
    ibuf_.Pop(head.msg_len_);
  }
  ibuf_.Adjust();
}

void TcpConn::DoWrite() {
  // do_write是触发玩event事件要处理的事情，
  // 应该是直接将out_buf力度数据io写会对方客户端
  // 而不是在这里组装一个message再发
  // 组装message的过程应该是主动调用

  // 只要obuf中有数据就写
  while (obuf_.Length()) {
    int ret = obuf_.WriteFd(connfd_);
    if (ret == -1) {
      std::cerr << "WriteFd error, close conn!\n";
      return;
    }
    if (ret == 0) {
      // 不是错误，仅返回0表示不可继续写
      break;
    }
  }
  if (obuf_.Length() == 0) {
    loop_->DelIoEvent(connfd_, EPOLLOUT);
  }
}

void TcpConn::CleanConn() {
  // 链接清理工作
  // 1 将该链接从tcp_server摘除掉
  // TODO
  // 2 将该链接从event_loop中摘除
  loop_->DelIoEvent(connfd_);
  // 3 buf清空
  ibuf_.Clear();
  obuf_.Clear();
  // 4 关闭原始套接字
  int fd = connfd_;
  connfd_ = -1;
  close(fd);
}

int TcpConn::SendMessage(const char* data, int msg_len, int msg_id) {
  std::cout << "server sendMessage: " << data << " msg_len :" << msg_len
            << " msg_id: " << msg_id << std::endl;
  bool active_epollout = false;
  if(obuf_.Length() ==0 ) {
    //如果现在已经数据都发送完了，那么是一定要激活写事件的
    //如果有数据，说明数据还没有完全写完到对端，那么没必要再激活等写完再激活
    active_epollout = true;
  }
  //1 先封装message消息头
  MsgHead head{msg_id,msg_len};
  //1.1 写消息头
  int ret = obuf_.SentData(reinterpret_cast<char*>(&head),MESSAGE_HEAD_LEN);
  if(ret != 0) {
    std::cerr <<"send head error!\n";
    return -1;
  }
    //1.2 写消息体
  ret = obuf_.SentData(data,msg_len);
  if(ret != 0) {
    //如果写消息体失败，那就回滚将消息头的发送也取消
    obuf_.Pop(MESSAGE_HEAD_LEN);
    return -1;
  }
  if(active_epollout) {
    //2. 激活EPOLLOUT写事件
    loop_->AddIoEvent(connfd_,conn_write_callback,EPOLLOUT,this);
  }
  return 0;
}
