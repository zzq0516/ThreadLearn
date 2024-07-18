#include <algorithm>
#include <condition_variable>
#include <cstddef>
#include <format>
#include <iostream>
#include <iterator>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <valarray>
#include <vector>

constexpr int PRODUCT_MAX = 200;

std::queue<int> mQueue;
std::mutex mlock;
std::mutex mProductLock;
std::condition_variable cv;
std::condition_variable cv1;
int valNow = 1;

template <typename T>
void privodeVal(T val) {
  std::unique_lock<std::mutex> lk(mlock);
  mQueue.push(static_cast<int>(val));
  lk.unlock();
  cv.notify_all();
}

void producers(int id) {
  while (true) {
    std::unique_lock<std::mutex> lkProducer(mProductLock);
    cv1.wait(lkProducer, []() { return true; });
    if (mQueue.empty()) {
      if (valNow < PRODUCT_MAX) {
        std::cout << std::format("producers id:{} will product val {}\n", id,
                                 valNow);
        privodeVal(valNow);
        valNow++;
      } else {
        privodeVal(0);
        std::cout << std::format("quit pro {}\n", id);
        return;
      }
      lkProducer.unlock();
    } else {
      lkProducer.unlock();
      if (mQueue.front() == 0) {
        std::cout << std::format("quit pro {}\n", id);
        return;
      }
    }
  }
}

void costormer(int id) {
  while (true) {
    // std::this_thread::sleep_for(std::chrono::microseconds(1));
    std::unique_lock<std::mutex> lk(mlock);
    cv.wait(lk, []() { return !mQueue.empty(); });
    // 现在已经上锁了不要重复上锁
    auto val = mQueue.front();
    if (val != 0) mQueue.pop();
    lk.unlock();
    if (val == 0) {
      std::cout << std::format("quit cos {}\n", id);
      return;
    }
    std::cout << std::format("constormer id :{} get {}\n", id, val);
    cv1.notify_all();
  }
}

int main() {
  std::vector<std::thread> threadVec;
  for (size_t i = 0; i < 2; i++) {
    std::thread costormerThread(costormer, i);
    threadVec.push_back(std::move(costormerThread));
  }
  for (size_t i = 0; i < 2; i++) {
    std::thread producersThread(producers, i);
    threadVec.push_back(std::move(producersThread));
  }
  for (size_t i = 0; i < threadVec.size(); i++) {
    threadVec[i].join();
  }
  std::cout << "get there\n";
}
