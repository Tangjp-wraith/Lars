#pragma once

#include <cstring>
#include <memory>

class IoBuffer : public std::enable_shared_from_this<IoBuffer> {
 public:
  explicit IoBuffer(int size);
  // 清空数据
  void Clear();
  // 将已经处理过的数据，清空,将未处理的数据提前至数据首地址
  void Adjust();
  // 将其他io_buf对象数据考本到自己中
  void Copy(const std::shared_ptr<IoBuffer>& other);
  // 处理长度为len的数据，移动head和修正length
  void Pop(int len);

  std::shared_ptr<IoBuffer> GetNext() const { return next_; }
  void SetNext(const std::shared_ptr<IoBuffer>& next) { next_ = next; }
  int GetCapacity() const { return capacity_; }
  int GetLength() const { return length_; }
  void SetLength(int length) { length_ = length; }
  char* GetData() const { return data_; }
  int GetHead() const { return head_; }

 private:
  /// 当前io_buf所保存的数据地址
  char* data_;
  /// 当前buffer的缓存容量大小
  int capacity_;
  /// 当前buffer有效数据长度
  int length_;
  /// 未处理数据的头部位置索引
  int head_;
  std::shared_ptr<IoBuffer> next_;
};