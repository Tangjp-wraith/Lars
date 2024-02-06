#pragma once

#include "io_buffer.h"
#include "buffer_pool.h"

class ReactorBuffer {
 public:
  ReactorBuffer();
  ~ReactorBuffer();

  int Length() const;
  void Pop(int len);
  void Clear();

 protected:
  std::shared_ptr<IoBuffer> buffer_;
};

class InputBuffer : public ReactorBuffer {
 public:
  //从一个fd中读取数据到reactor_buf中
  int ReadData(int fd);
  //取出读到的数据
  char *Data() const;
  //重置缓冲区
  void Adjust();
};

class OutputBuffer : public ReactorBuffer {
 public:
  //将一段数据 写到一个reactor_buf中
  int SentData(const char* data,int len);
  //将reactor_buf中的数据写到一个fd中
  int WriteFd(int fd);
};