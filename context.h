#ifndef _CONTEXT_H_
#define _CONTEXT_H_

#include <memory>
#include <vector>

namespace graph_executor {

class Node;

// Virtaul base class of context switched between nodes.
class Context {
 public:
  // Constructs `Context` with pointers to producer and consumer `Node*`.
  // But not ownership.
  Context(const Node* producer, const std::vector<Node*>& consumers)
      : ref_count_(0), producer_(producer), consumers_(consumers) {}
  virtual ~Context() {}

  // Not copyable. Movable.
  Context(const Context& other) = delete;
  Context(Context&& other) = default;

  const Node* Producer() const { return producer_; };
  const std::vector<Node*>& Consumers() const { return consumers_; };

  // Whether this context is produced by this producer.
  bool IsProduced() const { return ref_count_ != 0; }
  // Whether this context is consumed by all this consumers.
  bool IsConsumed() const { return ref_count_ == 0; }

  // Producer uses this function to indicate the context is ready.
  template <typename T>
  void Produce(T&& value) {
    Store(reinterpret_cast<void*>(new T(value)));
    ref_count_ = consumers_.size();
  };
  // Consumer uses this function to access the context.
  template <typename T>
  const T& Evaluate() const {
    void* ptr;
    Read(&ptr);
    return *reinterpret_cast<T*>(ptr);
  }
  // Consumer uses this function to indicate the context is consumed.
  void Consume();

  friend class Node;

 protected:
  // Reades the value of the context.
  virtual void Read(void** value) const = 0;
  // Stores the value of the context.
  virtual void Store(void* value) = 0;
  // Releases the value of the context.
  virtual void Release() = 0;

 private:
  int ref_count_;
  const Node* producer_;
  const std::vector<Node*> consumers_;
};

template <typename T>
class GenericContext : public Context {
 public:
  using Context::Context;

 protected:
  virtual void Store(void* value) final {
    data_ = std::unique_ptr<T>(reinterpret_cast<T*>(value));
  }

  virtual void Release() final { data_ = nullptr; }

  virtual void Read(void** value) const final {
    *reinterpret_cast<T**>(value) = data_.get();
  };

 private:
  std::unique_ptr<T> data_;
};

}  // namespace graph_executor

#endif
