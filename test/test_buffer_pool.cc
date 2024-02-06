#include <thread>
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lars_reactor/buffer_pool.h"

// 测试 BufferPool 的 AllocBuffer 函数
TEST(BufferPoolTest, AllocBufferTest) {
  // 获取 BufferPool 单例对象的引用
  BufferPool& bufferPool = BufferPool::instance();

  // 分配一个 8K 的 IoBuffer
  std::shared_ptr<IoBuffer> buffer = bufferPool.AllocBuffer(8192);

  // 检查返回的 IoBuffer 是否为空
  ASSERT_NE(buffer, nullptr);

  // 检查 IoBuffer 的大小是否正确
  EXPECT_EQ(buffer->GetCapacity(), m16K);
}

// 测试 AllocBuffer 函数是否能够分配有效的 IoBuffer
TEST(BufferPoolTest, AllocBufferTest1) {
  // 获取 BufferPool 单例对象的引用
  BufferPool& bufferPool = BufferPool::instance();

  // 分配一个大小为 4K 的 IoBuffer
  auto buffer = bufferPool.AllocBuffer();

  // 检查返回的缓冲区是否有效
  EXPECT_NE(buffer, nullptr);

  // 检查返回的缓冲区大小是否符合预期
  EXPECT_EQ(buffer->GetCapacity(), m4K);
}

// 测试连续调用 AllocBuffer 函数是否会导致内存泄漏
TEST(BufferPoolTest, AllocBufferMemoryLeakTest) {
  // 获取 BufferPool 单例对象的引用
  BufferPool& bufferPool = BufferPool::instance();

  // 分配一些 IoBuffer
  auto buffer1 = bufferPool.AllocBuffer();
  auto buffer2 = bufferPool.AllocBuffer();
  auto buffer3 = bufferPool.AllocBuffer();

  // 手动释放 IoBuffer 对象
  buffer1.reset();
  buffer2.reset();
  buffer3.reset();

  // 手动释放 BufferPool 对象
//  bufferPool.~BufferPool();

  // 检查内存泄漏情况
  // 在这里，你可以使用 Valgrind 或其他内存泄漏检测工具来检查内存泄漏情况
}

// 测试 BufferPool 的 revert 函数
TEST(BufferPoolTest, RevertTest) {
  // 获取 BufferPool 单例对象的引用
  BufferPool& bufferPool = BufferPool::instance();

  // 分配一个 4K 的 IoBuffer
  std::shared_ptr<IoBuffer> buffer = bufferPool.AllocBuffer(4096);

  // 将 IoBuffer 放回 BufferPool
  bufferPool.revert(buffer);

  // 再次分配一个 4K 的 IoBuffer
  std::shared_ptr<IoBuffer> newBuffer = bufferPool.AllocBuffer(4096);

  // 检查新分配的 IoBuffer 是否与之前的相同
  EXPECT_EQ(buffer, newBuffer);
}

// 测试 BufferPool 的静态成员函数 FindNearestIndex
TEST(BufferPoolTest, FindNearestIndexTest) {
  // 测试不同大小的输入值是否返回正确的 MEM_CAP
  EXPECT_EQ(BufferPool::FindNearestIndex(3000), m4K);
  EXPECT_EQ(BufferPool::FindNearestIndex(10000), m16K);
  EXPECT_EQ(BufferPool::FindNearestIndex(70000), m256K);
}

// 测试 PreAllocPool 函数是否正确地预先分配指定大小和数量的 IoBuffer
TEST(BufferPoolTest, PreAllocPoolTest) {
  // 获取 BufferPool 单例对象的引用
  BufferPool& bufferPool = BufferPool::instance();

  // 预先分配 5 个大小为 4K 的 IoBuffer
  std::shared_ptr<IoBuffer> prev;
  bufferPool.PreAllocPool(prev, m4K, 5);

  // 检查 IoBuffer 是否按照预期分配
  auto pool = bufferPool.GetPool();
  EXPECT_EQ(pool[m4K] != nullptr, true);
  EXPECT_EQ(prev->GetCapacity(), m4K);

  // 检查 IoBuffer 的数量是否正确
  int count = 0;
  std::shared_ptr<IoBuffer> current = pool[m4K];
  while (current != nullptr) {
    current = current->GetNext();
    count++;
  }
  EXPECT_EQ(count, 5);
}
// 测试连续调用 PreAllocPool 函数是否会导致内存泄漏
TEST(BufferPoolTest, PreAllocPoolMemoryLeakTest) {
  // 获取 BufferPool 单例对象的引用
  BufferPool& bufferPool = BufferPool::instance();

  // 预先分配一些 IoBuffer
  std::shared_ptr<IoBuffer> prev;
  bufferPool.PreAllocPool(prev, m4K, 5);
  bufferPool.PreAllocPool(prev, m16K, 3);
  bufferPool.PreAllocPool(prev, m64K, 2);

  // 手动释放 BufferPool 对象
//  bufferPool.~BufferPool();

  // 检查内存泄漏情况
  // 在这里，你可以使用 Valgrind 或其他内存泄漏检测工具来检查内存泄漏情况
}

// 定义测试类
class MultiThreadBufferPoolTest : public ::testing::Test {
 protected:
  // 在每个测试用例执行之前调用
  void SetUp() override {
    // 获取 BufferPool 单例对象的引用
    bufferPool_ = &BufferPool::instance();
  }

  // 在每个测试用例执行之后调用
  void TearDown() override {
    // 释放所有分配的 IoBuffer 对象
    for (auto buffer : buffers_) {
      buffer.reset();
    }
  }

  // 用于存储分配的 IoBuffer 对象
  std::vector<std::shared_ptr<IoBuffer>> buffers_;
  // 用于保护 buffers_ 的互斥锁
  std::mutex mutex_;
  // BufferPool 单例对象的引用
  BufferPool* bufferPool_{};

 public:
  // 定义测试用的线程函数
  void ThreadFunction() {
    for (int i = 0; i < 100; ++i) {
      auto buffer = bufferPool_->AllocBuffer();
      // 将分配的 IoBuffer 加入到结果向量中
      std::lock_guard<std::mutex> lock(mutex_);
      buffers_.push_back(buffer);
    }
  }
};

// 测试多线程环境下 AllocBuffer 函数的线程安全性和正确性
TEST_F(MultiThreadBufferPoolTest, MultiThreadAllocBufferTest) {
  // 定义 10 个线程
  std::vector<std::thread> threads;
  for (int i = 0; i < 10; ++i) {
    threads.push_back(std::thread(&MultiThreadBufferPoolTest::ThreadFunction, this));
  }

  // 等待所有线程执行完毕
  for (auto& thread : threads) {
    thread.join();
  }

  // 检查分配的 IoBuffer 对象数量是否正确
  ASSERT_EQ(buffers_.size(), 1000);
}

