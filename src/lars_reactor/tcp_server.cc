#include "lars_reactor/tcp_server.h"

#include <arpa/inet.h>
#include <sys/socket.h>

#include <csignal>
#include <cstring>
#include <memory>
#include <utility>

#include "lars_reactor/reactor_buffer.h"

struct message {
  char data[m4K];
  int len;
};

struct message msgs;

void ServerReadCallback(EventLoop* loop, int fd, void* args);
void ServerWriteCallback(EventLoop* loop, int fd, void* args);

void ServerReadCallback(EventLoop* loop, int fd, void* args) {
  int ret;
  auto msg = static_cast<message*>(args);
  InputBuffer i_buf;
  ret = i_buf.ReadData(fd);
  if (ret == -1) {
    std::cerr << "input buffer read data error!\n";
    // 删除事件
    loop->DelIoEvent(fd);
    // 对端关闭
    close(fd);
    return;
  }
  if (ret == 0) {
    // 删除事件
    loop->DelIoEvent(fd);
    // 对端关闭
    close(fd);
    return;
  }
  std::cout << "input buffer length: " << i_buf.Length() << std::endl;
  // 将读到的数据放在msg中
  msg->len = i_buf.Length();
  bzero(msg->data, msg->len);
  memcpy(msg->data, i_buf.Data(), msg->len);
  i_buf.Pop(msg->len);
  i_buf.Adjust();
  std::cout << "receive data = " << msg->data << std::endl;
  // 删除读事件，添加写事件
  loop->DelIoEvent(fd, EPOLLIN);
  loop->AddIoEvent(fd, ServerWriteCallback, EPOLLOUT, msg);
}

void ServerWriteCallback(EventLoop* loop, int fd, void* args) {
  auto msg = static_cast<message*>(args);
  OutputBuffer o_buf;
  // 回显数据
  o_buf.SentData(msg->data, msg->len);
  while (o_buf.Length()) {
    int write_ret = o_buf.WriteFd(fd);
    if (write_ret == -1) {
      std::cerr << "write connfd error!\n";
      return;
    } else if (write_ret == 0) {
      break;
    }
  }
  // 删除写事件，添加读事件
  loop->DelIoEvent(fd, EPOLLOUT);
  loop->AddIoEvent(fd, ServerReadCallback, EPOLLIN, msg);
}

// listen fd 客户端有新链接请求过来的回调函数
auto accept_callback = [](EventLoop* loop, int fd, void* args) {
  auto server = reinterpret_cast<TcpServer*>(args);
  if (server) {
    server->DoAccept();
  }
};

TcpServer::TcpServer(EventLoop* loop, const char* ip, uint16_t port) {
  bzero(&connaddr_, sizeof(connaddr_));
  /**
   * 忽略一些信号 SIGHUP, SIGPIPE
   * SIGPIPE:如果客户端关闭，服务端再次write就会产生
   * SIGHUP:如果terminal关闭，会给当前进程发送该信号
   */
  if (signal(SIGHUP, SIG_IGN) == SIG_ERR) {
    std::cerr << "signal ignore SIGHUP\n";
  }
  if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
    std::cerr << "signal ignore SIGPIPE\n";
  }
  // 创建socket
  sockfd_ = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, IPPROTO_TCP);
  if (sockfd_ == -1) {
    std::cerr << "TcpServer::socket()\n";
    exit(1);
  }
  // 初始化地址
  struct sockaddr_in server_addr {};
  bzero(&server_addr, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  inet_aton(ip, &server_addr.sin_addr);
  server_addr.sin_port = htons(port);
  // 可以多次监听，设置REUSE属性
  int op = 1;
  if (setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &op, sizeof(op)) < 0) {
    std::cerr << "setsockopt SO_REUSEADDR\n";
  }
  // 绑定端口
  if (bind(sockfd_, reinterpret_cast<const struct sockaddr*>(&server_addr),
           sizeof(server_addr)) < 0) {
    std::cerr << "bind error \n";
    exit(1);
  }
  // 监听ip端口
  if (listen(sockfd_, 500) == -1) {
    std::cerr << "listen error\n";
    exit(1);
  }
  // 将_sockfd添加到event_loop中
  loop_ = loop;
  // 注册_socket读事件-->accept处理
  loop_->AddIoEvent(sockfd_, accept_callback, EPOLLIN, this);
}

TcpServer::~TcpServer() { close(sockfd_); }

void TcpServer::DoAccept() {
  int connfd;
  while (true) {
    // accept与客户端创建链接
    std::cout << "begin accept\n";
    connfd = accept(sockfd_, (struct sockaddr*)&connaddr_, &addrlen_);
    if (connfd == -1) {
      if (errno == EINTR) {
        std::cerr << "accept errno = EINTR\n";
        continue;
      } else if (errno == EMFILE) {
        // 建立链接过多，资源不够
        std::cerr << "accept errno = EMFILE\n";
      } else if (errno == EAGAIN) {
        std::cerr << "accept errno = EAGAIN\n";
        break;
      } else {
        std::cerr << "accept error\n";
      }
    } else {
      this->loop_->AddIoEvent(connfd, ServerReadCallback, EPOLLIN, &msgs);
      break;
    }
  }
}
