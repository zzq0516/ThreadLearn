//
// Created by luolei on 24-7-19.
//

#ifndef THREADLEARN_THREADPOOL_H
#define THREADLEARN_THREADPOOL_H

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <future>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

enum class ThreadPoolState { CLOSED, RUNNING, WAITING, PAUSED };

class ThreadPool {
  // 定义任务结构体

 public:
  ThreadPool(size_t count = 8, bool destroy_idle = true)
      : threadTotol(count), destroyIdle(destroy_idle) {
    threadVec.resize(THREADS_MAX + 1);
    createPool(count);
    manager = std::thread(&ThreadPool::manager_call, this);
  }
  ThreadPool(ThreadPool&&) = delete;
  ~ThreadPool() {
    waitUntilDone();
    destroyPool();
  }

 public:
  // 判断是否有没有运行起来的任务
  bool is_empty() const noexcept { return taskCount == 0; }
  // 判断线程池当前的状态
  bool is_running() const noexcept { return state == ThreadPoolState::RUNNING; }

  bool is_closed() const noexcept { return state == ThreadPoolState::CLOSED; }

  bool is_waiting() const noexcept { return state == ThreadPoolState::WAITING; }

  bool is_paused() const noexcept { return state == ThreadPoolState::PAUSED; }

 private:
  // 实现线程池创建
  void createPool(size_t count) {
    state = ThreadPoolState::RUNNING;
    for (size_t i = 0; i < count; ++i) {
      threadVec[i] = std::thread(&ThreadPool::worker_call, this);
    }
  }
  void destroyPool() {
    state = ThreadPoolState::CLOSED;
    taskAvailableCV.notify_all();
    if (manager.joinable()) manager.join();
    for (size_t i = 0; i <= THREADS_MAX; ++i) {
      if (threadVec[i].joinable()) threadVec[i].join();
    }
  }
  // 这是工作线程
  void worker_call() {
    std::function<void()> task;
    while (true) {
      std::unique_lock<std::mutex> lk(mtxTask);
      // 当task队列为空时，阻塞在此处
      // 什么时候开始？
      // 条件1：task队列不为空
      // 条件2：空闲线程太多了，需要销毁一部分线程
      taskAvailableCV.wait(lk, [this]() {
        return (!is_empty() || is_closed() || threadsDestroy > 0) &&
               !is_paused();
      });
      // 空闲线程太多了销毁一部分
      if (threadsDestroy > 0 && is_empty() && !is_closed()) {
        --threadsDestroy;
        ids.emplace(std::this_thread::get_id());
        break;
      }
      // 线程池被关闭了而且没有任务需要调度了销毁这个线程
      if (is_closed() && is_empty()) break;
      task = std::move(mTaskList.front());
      mTaskList.pop_front();
      --taskCount;
      lk.unlock();
      ++threadRuning;
      task();
      --threadRuning;
      // Wait for closing pool when there are no threads that are running and
      // ready to run.
      if (is_waiting() && !threadRuning && is_empty()) {
        poolDoneCV.notify_all();
      }
    }
  }
  // 实现调度器
  void manager_call() {
    while (true) {
      if (is_closed()) return;
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
      // 动态控制线程
      if ((taskCount + threadRuning) > threadTotol &&
          threadTotol < THREADS_MAX) {
        // 求出Step,有可能不允许加到THREADS_ADD
        size_t add = std::min<size_t>(THREADS_ADD, THREADS_MAX - threadTotol);

        size_t j = 0;
        std::lock_guard<std::mutex> lk(mtxTask);
        for (size_t i = 0; i < THREADS_MAX && j < add && !ids.empty(); ++i) {
          if (!threadVec[i].joinable()) continue;
          auto id = threadVec[i].get_id();
          if (ids.contains(id)) {
            threadVec[i].join();
            threadVec[i] = std::thread(&ThreadPool::worker_call, this);
            ids.erase(id);
          }
        }

        for (size_t i = 0; i < THREADS_MAX && j < add; ++i) {
          if (!threadVec[i].joinable()) {
            threadVec[i] = std::thread(&ThreadPool::worker_call, this);
            ++threadTotol;
            ++j;
          }
        }
      }
      // 回收多余线程
      {
        if (!ids.empty()) {
          std::lock_guard<std::mutex> lk(mtxTask);
          for (size_t i = 0; i < THREADS_MAX && !ids.empty(); ++i) {
            if (!threadVec[i].joinable()) continue;
            auto id = threadVec[i].get_id();
            if (ids.contains(id)) {
              threadVec[i].join();
              --threadTotol;
              ids.erase(id);
            }
          }
        }
      }

      if (destroyIdle) {
        if (threadRuning * 2 < threadTotol && threadTotol > THREADS_MIN) {
          size_t add = std::min<size_t>(THREADS_ADD, threadTotol - THREADS_MIN);
          this->threadsDestroy = add;
          taskAvailableCV.notify_all();
        }
      }
    }
  }

 public:
  template <typename F, typename... Args>
  decltype(auto) push(F&& f, Args&&... args) {
    // When the pool has been stopped, adding tasks is not allowed.
    if (is_closed())
      throw std::runtime_error("Error: Adding tasks on a closed thread-pool.");

    using return_type = std::invoke_result_t<F, Args...>;
    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...));
    auto res = task->get_future();
    {
      std::lock_guard<std::mutex> lk(mtxTask);
      mTaskList.push_back([task]() { (*task)(); });
      taskCount++;
    }
    taskAvailableCV.notify_one();
    return res;
  }

 private:
  void waitUntilDone() {
    if (is_closed() || is_paused()) return;
    std::unique_lock<std::mutex> lk(mtxTask);
    state = ThreadPoolState::WAITING;
    poolDoneCV.wait(lk, [this] { return !threadRuning && is_empty(); });
  }

 private:
  // 线程池中现有的线程个数
  std::atomic<size_t> threadTotol;
  // 任务队列
  std::deque<std::function<void()>> mTaskList;
  // 正在工作线程个数
  std::atomic<size_t> threadRuning;
  // 空闲的线程个数
  std::atomic<size_t> threadFree;
  // 队列中的任务数，不包括正在运行的任务
  std::atomic<size_t> taskCount;
  // 需要销毁的线程数
  std::atomic<size_t> threadsDestroy = 0;
  // 线程id表
  std::unordered_set<std::thread::id> ids;
  // 条件变量:任务队列就绪
  std::condition_variable taskAvailableCV = {};
  // 条件变量：用于通知主线程所有任务都已完成
  std::condition_variable poolDoneCV = {};
  // 锁
  std::mutex mtxTask;
  // 线程队列
  std::vector<std::thread> threadVec;
  // 状态
  std::atomic<ThreadPoolState> state = ThreadPoolState::CLOSED;
  // 线程池运行最大的任务数
  static inline size_t THREADS_MAX = 4 * std::thread::hardware_concurrency();
  // 线程池运行运行的最小线程数
  static inline size_t THREADS_MIN =
      std::min<size_t>(2, std::thread::hardware_concurrency() / 2);
  // 增加和销毁线程的步进
  static constexpr size_t THREADS_ADD = 4;

  bool destroyIdle = true;

  std::thread manager;
};

#endif  // THREADLEARN_THREADPOOL_H
