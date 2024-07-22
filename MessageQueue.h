//
// Created by luolei on 24-7-21.
//

#ifndef THREADLEARN_MESSAGEQUEUE_H
#define THREADLEARN_MESSAGEQUEUE_H

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <optional>

template <typename T>
class MessageQueue {
 public:
  // 推入数据，如果满，则阻塞
  void push(T val) {
    std::unique_lock<std::mutex> lock(mMutes);
    while (mQueue.size() > mLimit) {
      mFullCV.wait(lock);
    }
    mQueue.push_front(std::move(val));
    mEmptyCV.notify_one();
  }

  // 尝试推数据，不阻塞，如果满，返回 false
  bool tryPush(T val) {
    std::unique_lock<std::mutex> lock(mMutes);
    if (mQueue.size() > mLimit) {
      return false;
    }
    mQueue.push_front(std::move(val));
    mEmptyCV.notify_one();
    return true;
  }

  // 尝试推数据，如果满，等待一段时间，超时返回 false
  bool tryPushFor(T val, std::chrono::system_clock::duration outTime) {
    std::unique_lock<std::mutex> lock(mMutes);
    if (!mFullCV.wait_for(mMutes, outTime, [&]() {
          return (mQueue.size() < mLimit ? true : false);
        })) {
      return false;
    }
    mQueue.push_front(std::move(val));
    mEmptyCV.notify_one();
    return true;
  }

  // 陷入阻塞，直到有数据
  T pop() {
    std::unique_lock lock(mMutes);
    while (mQueue.empty()) mEmptyCV.wait(lock);
    T value = std::move(mQueue.back());
    mQueue.pop_back();
    mFullCV.notify_one();
    return value;
  }

  // 尝试取数据，不阻塞，如果空，返回 nullopt
  std::optional<T> tryPop() {
    std::unique_lock<std::mutex> lock(mMutes);
    if (mQueue.size() == 0) {
      return std::nullopt;
    }
    T value = std::move(mQueue.back());
    mQueue.pop_back();
    mFullCV.notify_one();
    return value;
  }

  // 尝试取数据，如果空，等待一段时间，超时返回 nullopt
  std::optional<T> tryPopFor(std::chrono::system_clock::duration outTime) {
    std::unique_lock<std::mutex> lock(mMutes);
    if (!mEmptyCV.wait_for(mMutes, outTime,
                           [&]() { return !mQueue.empty(); })) {
      return std::nullopt;
    }
    T value = std::move(mQueue.back());
    mQueue.pop_back();
    mFullCV.notify_one();
    return value;
  }

  // 取得size大小
  size_t size() { return mQueue.size(); }

 public:
  MessageQueue(size_t limit) : mLimit(limit) {}

  MessageQueue() { mLimit = SIZE_MAX; }

 private:
  std::deque<T> mQueue;
  std::mutex mMutes;
  std::condition_variable mEmptyCV;
  std::condition_variable mFullCV;
  std::size_t mLimit;
};

#endif  // THREADLEARN_MESSAGEQUEUE_H
