#include <iostream>
#include <iterator>
#include <queue>
#include <thread>

std::queue<int> mQueue;

void producers() {
  mQueue.push(1);
  mQueue.push(2);
  mQueue.push(3);
  mQueue.push(4);
  mQueue.push(5);
  mQueue.push(11);
  mQueue.push(0);
}

void costormer() {
  while (true) {
    while (!mQueue.empty()) {
      auto val = mQueue.front();
      if (val == 0) {
        mQueue.pop();
        return;
      }
      std::cout << "costormer get val:" << val << std::endl;
      mQueue.pop();
    }
  }
}

int main() {
  std::thread t1(producers);
  // 这里阻塞执行 join 和单线程无区别
  costormer();
  t1.join();
}
