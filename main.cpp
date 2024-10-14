#include <exception>
#include <iostream>
#include <memory>
#include <stack>
#include <thread>

// 空栈异常
struct empty_stack : std::exception {
    char const *what() const throw() {
        return "empty stack!";
    };
};

template <typename T>
class stackThreadSafe {
public:
    stackThreadSafe() : mData(std::stack<int>()) {}

    stackThreadSafe(stackThreadSafe const &other) {
        // 这里这样设计是因为需要等待对面释放的锁之后才能进行拷贝
        std::lock_guard<std::mutex> lock(other.mLock);
        mData = other.mData;
    }

    // 删除拷贝赋值
    stackThreadSafe &operator=(stackThreadSafe const &) = delete;

    // 压栈
    void push(T new_value) {
        std::lock_guard<std::mutex> lock(mLock);
        mData.push(new_value);
    }

    // 出栈
    // 线程安全的堆栈类主要修改的是出栈接口
    // std原有的出栈接口，由于得到top和pop是分离的
    // 无法做到原子性
    std::shared_ptr<T> pop() {
        std::lock_guard<std::mutex> lock(mLock);
        if (mData.empty()) {
            throw empty_stack();
        }
        // 在修改堆栈前，分配出返回值
        std::shared_ptr<T> res(std::make_shared<T>(mData.top()));
        mData.pop();
        return res;
    }

    // 接口2
    void pop(T &value) {
        std::lock_guard<std::mutex> lock(mLock);
        if (mData.empty()) {
            throw empty_stack();
        }

        value = mData.top();
        mData.pop();
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(mLock);
        return mData.empty();
    }

private:
    std::stack<T> mData;
    mutable std::mutex mLock;
};

stackThreadSafe<int> stackTest;

void testFunc1() {
    for (int i = 0; i < 3; i++) {
        stackTest.push(i);
    }
}

void testFunc2() {
    for (int i = 3; i < 9; i++) {
        stackTest.push(i);
    }
}

void testFunc3() {
    while (1) {
        while (!stackTest.empty()) {
            auto i = stackTest.pop();
            std::cout << *i << std::endl;
        }
    }
}

int main(int argc, char const *argv[]) {
    std::thread thread2(testFunc2);
    std::thread thread1(testFunc1);
    std::thread thread3(testFunc3);
    thread1.join();
    thread2.join();
    thread3.join();
}
