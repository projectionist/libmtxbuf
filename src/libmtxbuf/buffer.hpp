#pragma once

#include <queue>
#include <condition_variable>
#include <mutex>
#include <atomic>
#include <string>

namespace mtxbuf {
  struct buffer_closed : public std::logic_error {
    explicit buffer_closed(std::string msg="buffer is closed"): std::logic_error(msg) {}
  };

  template<typename T>
  class buffer {
    std::queue<T> ns;
    size_t max;
    std::mutex mtx;
    std::condition_variable c_empty;
    std::condition_variable c_full;
    std::atomic<bool> closed;

  public:
    explicit buffer(size_t max = 1024): max(max), closed{false} {};
    ~buffer(){
      close();
    };
    void enqueue(T);
    T dequeue();
    void close();
    bool is_closed() { return bool(closed); };
  };


  template <typename T>
  void buffer<T>::enqueue(T e) {

    std::unique_lock<std::mutex> lck{mtx};

    c_full.wait(lck, [this] { return ns.size() < max || closed; });

    if (closed) return;

    ns.push(e);
    c_empty.notify_one();
  }

  template <typename T>
  T buffer<T>::dequeue() {
    std::unique_lock<std::mutex> lck{mtx};

    c_empty.wait(lck, [this] { return ns.size() > 0 || closed; });

    if (closed && ns.empty()) throw buffer_closed();

    T e = ns.front();

    ns.pop();
    c_full.notify_one();

    return e;
  }

  template <typename T>
  void buffer<T>::close() {
    closed = true;
    c_empty.notify_all();
    c_full.notify_all();
  }
}
