#include "lars_reactor/buffer_pool.h"
#include <cassert>
#include <iostream>

void BufferPool::PreAllocPool(std::shared_ptr<IoBuffer>& prev, MEM_CAP size,
                              int nums) {
  pool_[size] = std::make_shared<IoBuffer>(size);
  if (pool_[size] == nullptr) {
    std::cerr << "new io_buf error!\n";
    exit(1);
  }
  prev = pool_[size];
  for (int i = 1; i < nums; ++i) {
    prev->SetNext(std::make_shared<IoBuffer>(size));
    if (prev->GetNext() == nullptr) {
      std::cerr << "new io_buf error!\n";
      exit(1);
    }
    prev = prev->GetNext();
  }
  total_mem_ += size / 1024 * nums;
}

MEM_CAP BufferPool::FindNearestIndex(int n) {
  if (n <= m4K) {
    return m4K;
  } else if (n <= m16K) {
    return m16K;
  } else if (n <= m64K) {
    return m64K;
  } else if (n <= m256K) {
    return m256K;
  } else if (n <= m1M) {
    return m1M;
  } else if (n <= m4M) {
    return m4M;
  } else if (n <= m8M) {
    return m8M;
  }
  return unknown;
}

BufferPool::BufferPool() : total_mem_(0) {
  std::shared_ptr<IoBuffer> prev;
  PreAllocPool(prev, m4K, 5000);
  PreAllocPool(prev, m16K, 1000);
  PreAllocPool(prev, m64K, 500);
  PreAllocPool(prev, m256K, 200);
  PreAllocPool(prev, m1M, 50);
  PreAllocPool(prev, m4M, 20);
  PreAllocPool(prev, m8M, 10);
}

std::shared_ptr<IoBuffer> BufferPool::AllocBuffer(int n) {
  int index = static_cast<int>(FindNearestIndex(n));
  if (index == -1) {
    return nullptr;
  }
  std::lock_guard<std::mutex> lock(mutex_);
  if (pool_[index] == nullptr) {
    if (total_mem_ + index / 1024 >= EXTRA_MEM_LIMIT) {
      std::cerr << "already use too much memory!\n";
      exit(1);
    }
    auto new_buffer = std::make_shared<IoBuffer>(index);
    if (new_buffer == nullptr) {
      std::cerr << "new io_buf error\n";
      exit(1);
    }
    total_mem_ += index / 1024;
    return new_buffer;
  }
  std::shared_ptr<IoBuffer> target = pool_[index];
  pool_[index] = target->GetNext();
  target->SetNext(nullptr);
  return target;
}
std::shared_ptr<IoBuffer> BufferPool::AllocBuffer() {
  return AllocBuffer(m4K);
}
void BufferPool::revert(const std::shared_ptr<IoBuffer>& buffer) {
  std::lock_guard<std::mutex> lock(mutex_);
  int index = buffer->GetCapacity();
  buffer->Clear();
  assert(pool_.find(index) != pool_.end());
  buffer->SetNext(pool_[index]);
  pool_[index] = buffer;
}