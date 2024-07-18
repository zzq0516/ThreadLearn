#include <iostream>
#include <iterator>
#include <queue>

std::queue<int> mQueue;

void producers() {
  mQueue.push(1);
  mQueue.push(2);
  mQueue.push(3);
  mQueue.push(4);
  mQueue.push(5);
}

void costormer() {
  while (!mQueue.empty()) {
    std::cout << "costormer get val:" << mQueue.front() << std::endl;
    mQueue.pop();
  }
}

int main() {
  producers();
  costormer();
}
