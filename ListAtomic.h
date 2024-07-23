//
// Created by luolei on 24-7-23.
//

#ifndef THREADLEARN_LISTATOMIC_H
#define THREADLEARN_LISTATOMIC_H

#include <atomic>
#include <optional>

template <typename T>
struct ListAtomic {
  struct Node {
    T val;
    Node *next;
  };
  void push(T val) {
    Node *newNode = new Node;
    newNode->val = val;
    Node *oldHead = head.load();
    do {
      newNode->next = oldHead;
    } while (!head.compare_exchange_strong(oldHead, newNode));
  }

  std::optional<T> pop() {
    Node *oldHead = head.load();
    do {
      if (oldHead == nullptr) {
        return std::nullopt;
      }
    } while (!head(oldHead, oldHead->next));
    T val = oldHead->val;
    delete oldHead;
    return val;
  }
  int size_unsafe() {
    int count = 0;
    Node *node = head.load();
    while (node != nullptr) {
      count++;
      node = node->next;
    }
    return count;
  }

 private:
  std::atomic<Node *> head;
};

#endif  // THREADLEARN_LISTATOMIC_H
