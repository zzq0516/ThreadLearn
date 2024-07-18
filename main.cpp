#include <iostream>
#include <iterator>
#include <mutex>
#include <queue>
#include <thread>

std::queue<int> mQueue;
std::mutex mlock;

void producers() {
  mlock.lock();
  mQueue.push(1);
  mlock.unlock();
  mlock.lock();
  mQueue.push(2);
  mlock.unlock();
  mlock.lock();
  mQueue.push(3);
  mlock.unlock();
  mlock.lock();
  mQueue.push(4);
  mlock.unlock();
  mlock.lock();
  mQueue.push(5);
  mlock.unlock();
  mlock.lock();
  mQueue.push(11);
  mlock.unlock();
  mlock.lock();
  mQueue.push(0);
  mlock.unlock();
}

void costormer() {
  while (true) {
    while (!mQueue.empty()) {
      mlock.lock();
      auto val = mQueue.front();
      if (val == 0) {
        mQueue.pop();
        mlock.unlock();
        return;
      }
      std::cout << "costormer get val:" << val << std::endl;
      mQueue.pop();
      mlock.unlock();
    }
  }
}

int main() {
  std::thread t1(producers);
  // 这里阻塞执行 join 和单线程无区别
  costormer();
  t1.join();
}
