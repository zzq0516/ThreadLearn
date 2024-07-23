//
// Created by luolei on 24-7-22.
//

#ifndef THREADLEARN_SPINMUTEX_H
#define THREADLEARN_SPINMUTEX_H
#include <process.h>

#include <atomic>

class SpinxMutex {
 public:
  bool try_lock() {
    int refData = 0;
    if (flag.compare_exchange_strong(refData, 1)) return true;
    return false;

    // 下面演示不正确的方法
    // while (flag==0);
    // 中间可能被其他的线程穿插打断,不是安全的
    // flag = 1;
  }

  void lock() { while (!try_lock()); }

  void unlock() { flag = 0; }

 private:
  std::atomic<int> flag{0};
};

#endif  // THREADLEARN_SPINMUTEX_H
