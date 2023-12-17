#ifndef _CONTEXT_H_
#define _CONTEXT_H_

#include <memory>
#include <vector>

namespace graph_executor {

class Node;

// Virtaul base class of context switched between nodes.
class Context {
public:
  // Constructs context with pointers to producer and consumer nodes.
  // No ownership.
  Context() : has_value_(false), ref_count_(0) {}
  virtual ~Context() = default;

  // Not copyable. Movable.
  Context(const Context &other) = delete;
  Context(Context &&other) = default;

  const Node *Producer() const { return producer_; };
  const std::vector<Node *> &Consumers() const { return consumers_; };

  // Whether this context is produced by this producer.
  bool IsProduced() const { return has_value_; }
  // Whether this context is consumed by all this consumers.
  bool IsAllConsumed() const { return ref_count_ == 0; }

  // Puts data to context.
  template <typename T> void Put(T &&value) {
    has_value_ = true;
    Write(reinterpret_cast<void *>(new T(value)));
  }
  // Gets data from context.
  template <typename T> const T &Get() const {
    return *reinterpret_cast<T *>(Read());
  }
  // Marks the context is produced by the producer.
  void MarkProduced();
  // Marks the context is consumed by one of the consumers.
  void MarkConsumed();

  friend class Node;

protected:
  void BindProducer(Node *producer);
  void BindConsumer(Node *consumer);

  // Reades the value of the context.
  virtual void *Read() const = 0;
  // Writes the value of the context.
  virtual void Write(void *value) = 0;
  // Releases the value of the context.
  virtual void Release() = 0;

private:
  bool has_value_;
  int ref_count_;
  Node *producer_;
  std::vector<Node *> consumers_;
};

template <typename T> class GenericContext : public Context {
public:
  using Context::Context;

protected:
  virtual void *Read() const final {
    return reinterpret_cast<void *>(data_.get());
  };

  virtual void Write(void *value) final {
    data_ = std::unique_ptr<T>(reinterpret_cast<T *>(value));
  }

  virtual void Release() final { data_ = nullptr; }

private:
  std::unique_ptr<T> data_;
};

} // namespace graph_executor

#endif
