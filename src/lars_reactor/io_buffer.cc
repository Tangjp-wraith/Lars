#include "lars_reactor/io_buffer.h"

IoBuffer::IoBuffer(int size)
    : data_(new char[size]),
      capacity_(size),
      length_(0),
      head_(0),
      next_(nullptr) {}

void IoBuffer::Clear() {
  length_ = head_ = 0;
}

void IoBuffer::Adjust() {
  if (head_ != 0) {
    if (length_ != 0) {
      memmove(data_, data_ + head_, length_);
    }
    head_ = 0;
  }
}

void IoBuffer::Copy(const std::shared_ptr<IoBuffer>& other) {
  memcpy(data_, other->data_ + other->head_, other->length_);
  head_= 0;
  length_ = other->length_;
}

void IoBuffer::Pop(int len) {
  length_ -= len;
  head_ += len;
}
