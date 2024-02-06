#include "lars_reactor/reactor_buffer.h"
#include <sys/ioctl.h>
#include <cassert>
#include <csignal>
#include <iostream>

ReactorBuffer::ReactorBuffer() : buffer_(nullptr) {}

ReactorBuffer::~ReactorBuffer() {
  Clear();
}

int ReactorBuffer::Length() const {
  return buffer_ != nullptr ? buffer_->GetLength() : 0;
}

void ReactorBuffer::Pop(int len) {
  assert(buffer_ != nullptr && len <= buffer_->GetLength());
  buffer_->Pop(len);
  //当此时_buf的可用长度已经为0
  if (buffer_->GetLength() == 0) {
    //将_buf重新放回buf_pool中
    BufferPool::instance().revert(buffer_);
    buffer_ = nullptr;
  }
}

void ReactorBuffer::Clear() {
  if (buffer_ != nullptr) {
    BufferPool::instance().revert(buffer_);
    buffer_ = nullptr;
  }
}
int InputBuffer::ReadData(int fd) {
  //硬件有多少数据可以读
  int need_read;
  //一次性读出所有的数据
  //需要给fd设置FIONREAD,
  //得到read缓冲中有多少数据是可以读取的
  if (ioctl(fd, FIONREAD, &need_read) == -1) {
    std::cerr << "ioctl FIONREAD!\n";
    return -1;
  }
  if (buffer_ == nullptr) {
    //如果io_buf为空,从内存池申请
    buffer_ = BufferPool::instance().AllocBuffer(need_read);
    if (buffer_ == nullptr) {
      std::cerr << "no idle buffer for alloc!\n";
      return -1;
    }
  } else {
    //如果io_buf可用，判断是否够存
    assert(buffer_->GetHead() == 0);
    if (buffer_->GetCapacity() - buffer_->GetLength() < need_read) {
      //不够存，冲内存池申请
      auto new_buffer =
          BufferPool::instance().AllocBuffer(need_read + buffer_->GetLength());
      if (new_buffer == nullptr) {
        std::cerr << "no idle buffer for alloc!\n";
        return -1;
      }
      //将之前的_buf的数据考到新申请的buf中
      new_buffer->Copy(buffer_);
      //将之前的_buf放回内存池中
      BufferPool::instance().revert(buffer_);
      //新申请的buf成为当前io_buf
      buffer_ = new_buffer;
    }
  }
  //读取数据
  ssize_t already_read;
  do {
    //读取的数据拼接到之前的数据之后
    if (need_read == 0) {
      //可能是read阻塞读数据的模式，对方未写数据
      already_read = read(fd, buffer_->GetData() + buffer_->GetLength(), m4K);
    } else {
      already_read =
          read(fd, buffer_->GetData() + buffer_->GetLength(), need_read);
    }
  } while (already_read == -1 &&
           errno == EINTR);  //systemCall引起的中断 继续读取
  if (already_read > 0) {
    if (need_read != 0) {
      assert(already_read == need_read);
    }
    buffer_->SetLength(buffer_->GetLength() + static_cast<int>(already_read));
  }
  return static_cast<int>(already_read);
}
char* InputBuffer::Data() const {
  return buffer_ != nullptr ? buffer_->GetData() + buffer_->GetHead() : nullptr;
}
void InputBuffer::Adjust() {
  if (buffer_ != nullptr) {
    buffer_->Adjust();
  }
}

int OutputBuffer::SentData(const char* data, int len) {
  if (buffer_ == nullptr) {
    //如果io_buf为空,从内存池申请
    buffer_ = BufferPool::instance().AllocBuffer(len);
    if (buffer_ == nullptr) {
      std::cerr << "no idle buffer for alloc!\n";
      return -1;
    }
  } else {
    //如果io_buf可用，判断是否够存
    assert(buffer_->GetHead() == 0);
    if (buffer_->GetCapacity() - buffer_->GetLength() < len) {
      //不够存，冲内存池申请
      auto new_buffer =
          BufferPool::instance().AllocBuffer(len + buffer_->GetLength());
      if (buffer_ == nullptr) {
        std::cerr << "no idle buffer for alloc!\n";
        return -1;
      }
      //将之前的_buf的数据考到新申请的buf中
      new_buffer->Copy(buffer_);
      //将之前的_buf放回内存池中
      BufferPool::instance().revert(buffer_);
      //新申请的buf成为当前io_buf
      buffer_ = new_buffer;
    }
  }
  //将data数据拷贝到io_buf中,拼接到后面
  memcpy(buffer_->GetData() + buffer_->GetLength(), data, len);
  buffer_->SetLength(buffer_->GetLength() + len);
  return 0;
}

int OutputBuffer::WriteFd(int fd) {
  assert(buffer_ != nullptr && buffer_->GetHead() == 0);
  ssize_t already_write;
  do {
    already_write = write(fd, buffer_->GetData(), buffer_->GetLength());
  } while (already_write == -1 &&
           errno == EINTR);  //systemCall引起的中断，继续写
  if (already_write > 0) {
    //已经处理的数据清空
    buffer_->Pop(static_cast<int>(already_write));
    //未处理数据前置，覆盖老数据
    buffer_->Adjust();
  }
  //如果fd非阻塞，可能会得到EAGAIN错误
  if (already_write == -1 && errno == EAGAIN) {
    //不是错误，仅仅返回0，表示目前是不可以继续写的
    already_write = 0;
  }
  return static_cast<int>(already_write);
}
