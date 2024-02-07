#pragma once
#include <sys/epoll.h>

#include <array>
#include <unordered_map>
#include <unordered_set>

#include "event_base.h"

#define MAXEVENTS 10

class EventLoop {
 public:
  // 构造，初始化epoll堆
  EventLoop();
  // 阻塞循环处理事件
  void EventProcess();
  // 添加一个io事件到loop中
  void AddIoEvent(int fd, io_callback proc, int mask, void* args = nullptr);
  // 删除一个io事件从loop中
  void DelIoEvent(int fd);
  // 删除一个io事件的EPOLLIN/EPOLLOUT
  void DelIoEvent(int fd, int mask);
  // 从事件循环中获取与给定文件描述符相关联的数据
  IoEvent* GetData(int fd);

 private:
  /// epoll fd
  int epoll_fd_;
  /// 当前event_loop 监控的fd和对应事件的关系
  std::unordered_map<int, IoEvent> io_evs_;
  /// 当前event_loop 一共哪些fd在监听
  std::unordered_set<int> listen_fds;
  /// 一次性最大处理的事件
  std::array<struct epoll_event, MAXEVENTS> fired_evs_{};
};