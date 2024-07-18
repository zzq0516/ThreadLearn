#include <condition_variable>
#include <iostream>
#include <iterator>
#include <mutex>
#include <queue>
#include <thread>

std::queue<int> mQueue;
std::mutex mlock;
std::condition_variable cv;

void producers() {
  mlock.lock();
  mQueue.push(1);
  mlock.unlock();
  cv.notify_one();
  mlock.lock();
  mQueue.push(2);
  mlock.unlock();
  cv.notify_one();
  mlock.lock();
  mQueue.push(3);
  mlock.unlock();
  cv.notify_one();
  mlock.lock();
  mQueue.push(4);
  mlock.unlock();
  cv.notify_one();
  mlock.lock();
  mQueue.push(5);
  mlock.unlock();
  cv.notify_one();
  mlock.lock();
  mQueue.push(11);
  mlock.unlock();
  cv.notify_one();
  mlock.lock();
  mQueue.push(0);
  mlock.unlock();
  cv.notify_one();
  // 多通知的情况
  cv.notify_one();
  cv.notify_one();
}

void costormer() {
  while (true) {
    std::unique_lock<std::mutex> lk(mlock);
    cv.wait(lk, []() { return !mQueue.empty(); });
    // 现在已经上锁了不要重复上锁
    auto val = mQueue.front();
    mQueue.pop();
    lk.unlock();
    if (val == 0) {
      return;
    }
    std::cout << "constormer get " << val << std::endl;
  }
}

int main() {
  std::thread t1(producers);
  // 这里阻塞执行 join 和单线程无区别
  costormer();
  t1.join();
}
