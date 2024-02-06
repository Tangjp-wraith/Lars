#pragma once
#include <atomic>
#include <mutex>
#include <unordered_map>
#include "io_buffer.h"

using pool_t = std::unordered_map<int, std::shared_ptr<IoBuffer>>;

enum MEM_CAP {
  m4K = 4096,
  m16K = 16384,
  m64K = 65536,
  m256K = 262144,
  m1M = 1048576,
  m4M = 4194304,
  m8M = 8388608,
  unknown = -1
};

#define EXTRA_MEM_LIMIT (5U * 1024 * 1024)

class BufferPool {
 public:
  //获取单例方法
  static BufferPool& instance() {
    static BufferPool instance_;
    return instance_;
  }
  //开辟一个io_buf
  std::shared_ptr<IoBuffer> AllocBuffer(int n);
  std::shared_ptr<IoBuffer> AllocBuffer();
  //重置一个io_buf
  void revert(const std::shared_ptr<IoBuffer>& buffer);

  void PreAllocPool(std::shared_ptr<IoBuffer>& prev, MEM_CAP size, int nums);
  static MEM_CAP FindNearestIndex(int n);

  pool_t GetPool() const { return pool_; }

 private:
  BufferPool();
  //拷贝构造私有化
  BufferPool(const BufferPool&);
  const BufferPool& operator=(const BufferPool&);

  ///所有buffer的一个map集合句柄
  pool_t pool_;
  ///总buffer池的内存大小 单位为KB
  std::atomic<uint64_t> total_mem_;
  ///用户保护内存池链表修改的互斥锁
  std::mutex mutex_;
};