#include "lars_reactor/tcp_server.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <csignal>
#include <cstring>

TcpServer::TcpServer(const char* ip, uint16_t port) {
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
  //创建socket
  sockfd_ = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, IPPROTO_TCP);
  if (sockfd_ == -1) {
    std::cerr << "TcpServer::socket()\n";
    exit(1);
  }
  //初始化地址
  struct sockaddr_in server_addr {};
  bzero(&server_addr, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  inet_aton(ip, &server_addr.sin_addr);
  server_addr.sin_port = htons(port);
  //可以多次监听，设置REUSE属性
  int op = 1;
  if (setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &op, sizeof(op)) < 0) {
    std::cerr << "setsockopt SO_REUSEADDR\n";
  }
  //绑定端口
  if (bind(sockfd_, (const struct sockaddr*)&server_addr, sizeof(server_addr)) <
      0) {
    std::cerr << "bind error \n";
    exit(1);
  }
  //监听ip端口
  if (listen(sockfd_, 500) == -1) {
    std::cerr << "listen error\n";
    exit(1);
  }
}

TcpServer::~TcpServer() {
  close(sockfd_);
}

void TcpServer::DoAccept() {
  int connfd;
  while (true) {
    //accept与客户端创建链接
    std::cout << "begin accept\n";
    connfd = accept(sockfd_, (struct sockaddr*)&connaddr_, &addrlen_);
    if (connfd == -1) {
      if (errno == EINTR) {
        std::cerr << "accept errno = EINTR\n";
        continue;
      } else if (errno == EMFILE) {
        //建立链接过多，资源不够
        std::cerr << "accept errno = EMFILE\n";
      } else if (errno == EAGAIN) {
        std::cerr << "accept errno = EAGAIN\n";
        break;
      } else {
        std::cerr << "accept error\n";
      }
    } else {
      int writed;
      std::string str = "hello Lars\n";
      do {
        writed = static_cast<int>(write(connfd, str.data(), str.size() + 1));
      } while (writed == -1 && errno == EINTR);
      if (writed > 0) {
        std::cout << "write success!\n";
      }
      if (writed == -1 && errno == EAGAIN) {
        //不是错误，仅返回0表示此时不可继续写
        writed = 0;
      }
    }
  }
}
