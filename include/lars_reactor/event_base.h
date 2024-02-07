#pragma once

#include <functional>
/**
 * 定义一些IO复用机制或者其他异常触发机制的事件封装
 *
 */
class EventLoop;
// IO事件触发的回调函数
using io_callback = std::function<void(EventLoop*, int, void*)>;
/**
 * 封装一次IO触发实现
 */
struct IoEvent {
  IoEvent()
      : mask_(0),
        read_callback_(nullptr),
        write_callback_(nullptr),
        rcb_args_(nullptr),
        wcb_args_(nullptr){};

  /// EPOLLIN EPOLLOUT
  int mask_;
  /// EPOLLIN事件 触发的回调
  io_callback read_callback_;
  /// EPOLLOUT事件 触发的回调
  io_callback write_callback_;
  /// read_callback的回调函数参数
  void* rcb_args_;
  /// write_callback的回调函数参数
  void* wcb_args_;
};
