#include "ThreadPool.h"
#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <ctime>
#include <format>
#include <iostream>
#include <iterator>
#include <mutex>
#include <ostream>
#include <queue>
#include <thread>
#include <utility>
#include <valarray>
#include <vector>

long long calTask(int times, int beginVal) {
    long long ans = 0;
    for (int i = 0; i < times; i++) {
        ans += beginVal;
        beginVal++;
    }
    std::this_thread::sleep_for(std::chrono::seconds(times));
    std::cout << std::format("task {} finish\n", times);
    return ans;
}

void printTask(int id) {
    while (true) {
        // std::cout << std::format("print Task id : {}\n", id);
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

int main() {
    ThreadPool myThreadPool(8);
    // auto val = myThreadPool.push(calTask, 10, 0);
    //  for (int i = 0; i < 3; i++) {
    //    myThreadPool.push(printTask, i);
    //  }
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ::std::cout << "begin cal\n";
    // std::cout << val.get() << std::endl;
    std::vector<std::future<long long>> res(16);
    for (int i = 0; i < 16; i++) {
        auto r = myThreadPool.push(calTask, i, 0);
        res[i] = std::move(r);
    }
    std::cout << "end cal\n";
    for (int i = 0; i < 16; i++) {
        std::cout << std::format("val is {}\n", res[i].get());
    }
    std::cout << "end print\n";
}
