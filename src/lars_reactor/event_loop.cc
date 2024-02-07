#include "lars_reactor/event_loop.h"

#include <cassert>
#include <iostream>
#include <utility>

EventLoop::EventLoop() {
  epoll_fd_ = epoll_create1(0);
  if (epoll_fd_ == -1) {
    std::cerr << "epoll_create error!\n";
    exit(1);
  }
}

void EventLoop::EventProcess() {
  while (true) {
    int nfds = epoll_wait(epoll_fd_, fired_evs_.data(), MAXEVENTS, 10);
    for (int i = 0; i < nfds; ++i) {
      // 通过触发的fd找到对应的绑定事件
      auto ev_itr = io_evs_.find(fired_evs_[i].data.fd);
      assert(ev_itr != io_evs_.end());
      IoEvent* ev = &(ev_itr->second);
      if (fired_evs_[i].events & EPOLLIN) {
        // 读事件，调读回调函数
        void* args = ev->rcb_args_;
        ev->read_callback_(this, fired_evs_[i].data.fd, args);
      } else if (fired_evs_[i].events & EPOLLOUT) {
        // 写事件，调写回调函数
        void* args = ev->wcb_args_;
        ev->write_callback_(this, fired_evs_[i].data.fd, args);
      } else if (fired_evs_[i].events & (EPOLLHUP | EPOLLERR)) {
        // 水平触发未处理，可能会出现HUP事件，正常处理读写，没有则清空
        if (ev->read_callback_ != nullptr) {
          void* args = ev->rcb_args_;
          ev->read_callback_(this, fired_evs_[i].data.fd, args);
        } else if (ev->write_callback_ != nullptr) {
          void* args = ev->wcb_args_;
          ev->write_callback_(this, fired_evs_[i].data.fd, args);
        } else {
          // 删除
          std::cerr << "fd " << fired_evs_[i].data.fd
                    << " get error,delete it from epoll!\n";
          this->DelIoEvent(fired_evs_[i].data.fd);
        }
      }
    }
  }
}

/**
 * 这里我们处理的事件机制是
 * 如果EPOLLIN 在mask中， EPOLLOUT就不允许在mask中
 * 如果EPOLLOUT 在mask中， EPOLLIN就不允许在mask中
 * 如果想注册EPOLLIN|EPOLLOUT的事件， 那么就调用add_io_event() 方法两次来注册。
 */

void EventLoop::AddIoEvent(int fd, io_callback proc, int mask, void* args) {
  int final_mask, op;
  // 找到当前fd是否已经有事件
  auto itr = io_evs_.find(fd);
  if (itr == io_evs_.end()) {
    // 如果没有操作动作就是ADD
    // 没有找到
    final_mask = mask;
    op = EPOLL_CTL_ADD;
  } else {
    // 如果有操作动作是MOD
    // 添加事件标识位
    final_mask = itr->second.mask_ | mask;
    op = EPOLL_CTL_MOD;
  }
  // 注册回调函数
  if (mask & EPOLLIN) {
    io_evs_[fd].read_callback_ = std::move(proc);
    io_evs_[fd].rcb_args_ = args;
  } else if (mask & EPOLLOUT) {
    io_evs_[fd].write_callback_ = std::move(proc);
    io_evs_[fd].wcb_args_ = args;
  }
  // epoll_ctl添加到epoll堆里
  io_evs_[fd].mask_ = final_mask;
  // 创建原生epoll事件
  struct epoll_event event {};
  event.events = final_mask;
  event.data.fd = fd;
  if (epoll_ctl(epoll_fd_, op, fd, &event) == -1) {
    std::cerr << "epoll_ctr " << fd << " error!\n";
    return;
  }
  // 将fd添加到监听集合中
  listen_fds.insert(fd);
}

void EventLoop::DelIoEvent(int fd) {
  // 将事件从_io_evs删除
  io_evs_.erase(fd);
  // 将fd从监听集合中删除
  listen_fds.erase(fd);
  // 将fd从epoll堆删除
  epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
}

void EventLoop::DelIoEvent(int fd, int mask) {
  // 如果没有该事件，直接返回
  auto itr = io_evs_.find(fd);
  if (itr == io_evs_.end()) {
    return;
  }
  int& o_mask = itr->second.mask_;
  // 修正mask
  o_mask = o_mask & (~mask);
  if (o_mask == 0) {
    // 如果修正之后 mask为0，则删除
    this->DelIoEvent(fd);
  } else {
    // 如果修正之后，mask非0，则修改
    struct epoll_event event {};
    event.events = o_mask;
    event.data.fd = fd;
    epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &event);
  }
}
IoEvent* EventLoop::GetData(int fd) {
  auto itr = io_evs_.find(fd);
  if (itr != io_evs_.end()) {
    return &(itr->second);
  }
  return nullptr;
}
