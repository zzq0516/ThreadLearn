#include <list>
#include <mutex>

#include "ListAtomic.h"
#include "SpinMutex.h"
#include "test.h"

struct TestNaive {
  std::list<int> data;

  void entry(MTIndex<0>) { data.push_back(1); }

  void entry(MTIndex<1>) { data.push_back(2); }

  void teardown() {
    MTTest::result = data.size();  // 应为 2，但实际上出错
  }
};

struct TestNaiveUseMutex {
  std::list<int> data;
  std::mutex lock;
  void entry(MTIndex<0>) {
    lock.lock();
    data.push_back(1);
    lock.unlock();
  }

  void entry(MTIndex<1>) {
    lock.lock();
    data.push_back(2);
    lock.unlock();
  }

  void teardown() {
    MTTest::result = data.size();  // 应为 2，但实际上出错
  }
};

struct TestNaiveUseMySpin {
  std::list<int> data;
  SpinxMutex lock;
  void entry(MTIndex<0>) {
    lock.lock();
    data.push_back(1);
    lock.unlock();
  }

  void entry(MTIndex<1>) {
    lock.lock();
    data.push_back(2);
    lock.unlock();
  }

  void teardown() {
    MTTest::result = data.size();  // 应为 2，但实际上出错
  }
};

struct TestNaiveUseMyList {
  ListAtomic<int> data;
  SpinxMutex lock;
  void entry(MTIndex<0>) {
    lock.lock();
    data.push(1);
    lock.unlock();
  }

  void entry(MTIndex<1>) {
    lock.lock();
    data.push(2);
    lock.unlock();
  }

  void teardown() {
    MTTest::result = data.size_unsafe();  // 应为 2，但实际上出错
  }
};

int main() {
  MTTest::runTest<TestNaive>();
  MTTest::runTest<TestNaiveUseMutex>();
  MTTest::runTest<TestNaiveUseMySpin>();
  MTTest::runTest<TestNaiveUseMyList>();
}
