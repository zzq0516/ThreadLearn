#include <chrono>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <vector>

#include "MessageQueue.h"

MessageQueue<std::optional<int>> counterQueue;
MessageQueue<int> finishQueue;
MessageQueue<int> resBuff;

void counterTask() {
  int res = 0;
  while (finishQueue.size() < 10) {
    auto i = counterQueue.tryPop();
    if (i != std::nullopt) {
      auto j = i.value();
      if (j == std::nullopt) {
        finishQueue.push(-1);
        continue;
      }
      res += j.value();
    }
  }
  resBuff.push(res);
  // std::cout << std::format("{}\n", res);
}

void provideConterTask(int begin, int end) {
  for (int i = begin; i < end; i++) {
    counterQueue.push(i);
  }
  counterQueue.push(std::nullopt);
}

int main() {
  std::vector<std::thread> pool;

  for (int i = 0; i < 10000; i += 1000) {
    std::thread provideThread = std::thread(provideConterTask, i, i + 1000);
    pool.push_back(std::move(provideThread));
  }
  pool.push_back(std::thread(counterTask));
  pool.push_back(std::thread(counterTask));
  pool.push_back(std::thread(counterTask));
  pool.push_back(std::thread(counterTask));
  for (int i = 0; i < pool.size(); i++) {
    pool[i].join();
  }
  auto resVal = []() {
    int res = 0;
    for (int i = 0; i < 4; i++) {
      auto singleRes = resBuff.pop();
      res += singleRes;
    }
    return res;
  };
  std::cout << std::format("{}\n", resVal());
  return 0;
}
