#ifndef _CONTEXT_H_
#define _CONTEXT_H_

#include <cassert>
#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

namespace graph_executor {

class Context;
class Node;

template <typename T> class DataRef final {
public:
  DataRef(T *data, Context *context) : data_(data), context_(context) {}
  ~DataRef();

  const T &operator*() const { return *data_; }

private:
  T *data_;
  Context *context_;
};

// Virtaul base class of context switched between nodes.
class Context {
public:
  // Constructs context with pointers to producer and consumer nodes.
  // No ownership.
  explicit Context(const std::string &name = "") : name_(name){};
  virtual ~Context() = default;

  // Not copyable. Movable.
  Context(const Context &other) = delete;
  Context(Context &&other) = default;

  const std::string &Name() const { return name_; }
  const Node *Producer() const { return producer_; }
  const std::vector<Node *> &Consumers() const { return consumers_; }

  // Whether can put data to this context.
  virtual bool CanPut() = 0;
  // Whether can get data from this context.
  virtual bool CanGet() = 0;

  // Puts data to this context.
  // Each producer should only call this function once for the same data.
  template <typename T> void Put(T &&value) {
    if (!CanPut()) {
      std::cerr << Name() << "$\n";
      std::cerr.flush();
      std::this_thread::sleep_for(std::chrono::seconds(10));
    }
    assert(CanPut());
    PutGeneric(reinterpret_cast<void *>(new T(value)));
  }
  // Gets data from this context.
  // Conceptually, there are N copies for N consumers.
  // Each consumer should only call this function once for the same data.
  template <typename T> DataRef<T> Get() {
    if (!CanGet()) {
      std::cerr << Name() << "#\n";
      std::cerr.flush();
      std::this_thread::sleep_for(std::chrono::seconds(10));
    }
    assert(CanGet());
    return DataRef<T>(reinterpret_cast<T *>(GetGeneric()), this);
  }
  // Release data from this context.
  virtual void Release() = 0;

  friend class Node;

protected:
  void BindProducer(Node *producer);
  void BindConsumer(Node *consumer);

  // Puts data to this context using generic type (void*).
  virtual void PutGeneric(void *value) = 0;
  // Gets data from this context using generic type (void*).
  virtual void *GetGeneric() = 0;

  std::string name_;
  Node *producer_;
  std::vector<Node *> consumers_;
};

template <typename T> DataRef<T>::~DataRef() { context_->Release(); }

template <typename T> class GenericContext : public Context {
public:
  using Context::Context;

  virtual bool CanPut() {
    std::lock_guard<std::mutex> lock(mutex_);
    return data_ == nullptr;
  }
  virtual bool CanGet() {
    std::lock_guard<std::mutex> lock(mutex_);
    return data_ != nullptr;
  }

protected:
  virtual void PutGeneric(void *value) {
    std::lock_guard<std::mutex> lock(mutex_);
    ref_count_ = consumers_.size();
    data_ = std::unique_ptr<T>(reinterpret_cast<T *>(value));
  }

  virtual void *GetGeneric() {
    std::lock_guard<std::mutex> lock(mutex_);
    return reinterpret_cast<void *>(data_.get());
  }

  virtual void Release() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!--ref_count_) {
      data_ = nullptr;
    }
  }

private:
  std::mutex mutex_;
  int ref_count_;
  std::unique_ptr<T> data_;
};

// A buffered context that stores multiple instances of data.
template <typename T> class BufferedContext : public Context {
public:
  explicit BufferedContext(int max_size, const std::string &name = "")
      : Context(name), max_size_(max_size){};

  virtual bool CanPut() {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size() < max_size_;
  }
  virtual bool CanGet() {
    std::lock_guard<std::mutex> lock(mutex_);
    return !queue_.empty();
  }

protected:
  virtual void PutGeneric(void *value) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.empty()) {
      ref_count_ = consumers_.size();
    }
    queue_.push(std::unique_ptr<T>(reinterpret_cast<T *>(value)));
  }

  virtual void *GetGeneric() {
    std::lock_guard<std::mutex> lock(mutex_);
    return reinterpret_cast<void *>(queue_.front().get());
  }

  virtual void Release() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!--ref_count_) {
      queue_.pop();
      if (!queue_.empty()) {
        ref_count_ = consumers_.size();
      }
    }
  }

private:
  std::mutex mutex_;
  int max_size_;
  int ref_count_;
  std::queue<std::unique_ptr<T>> queue_;
};

} // namespace graph_executor

#endif
