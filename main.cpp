#include <condition_variable>
#include <iostream>
#include <iterator>
#include <mutex>
#include <queue>
#include <thread>

std::queue<int> mQueue;
std::mutex mlock;
std::condition_variable cv;

template <typename T>
void privodeVal(T val) {
  std::unique_lock<std::mutex> lk(mlock);
  mQueue.push(static_cast<int>(val));
  lk.unlock();
  cv.notify_one();
}

void producers() {
  privodeVal(1);
  privodeVal(2);
  privodeVal(3.14);
  privodeVal(8);
  privodeVal(11);
  privodeVal(0);
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
  std::thread t2(costormer);
  t1.join();
  t2.join();
}
