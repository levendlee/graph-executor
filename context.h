#ifndef _CONTEXT_H_
#define _CONTEXT_H_

#include <cassert>
#include <memory>
#include <vector>

namespace graph_executor {

class Context;
class Node;

template <typename T> class DataRef final {
public:
  DataRef(T *data, Context* context)
      : data_(data), context_(context) {}
  ~DataRef();

  const T &operator*() const { return *data_; }

private:
  T *data_;
  Context* context_;
};

// Virtaul base class of context switched between nodes.
class Context {
public:
  // Constructs context with pointers to producer and consumer nodes.
  // No ownership.
  Context() = default;
  virtual ~Context() = default;

  // Not copyable. Movable.
  Context(const Context &other) = delete;
  Context(Context &&other) = default;

  const Node *Producer() const { return producer_; };
  const std::vector<Node *> &Consumers() const { return consumers_; };

  // Whether can put data to this context.
  virtual bool CanPut() const = 0;
  // Whether can get data from this context.
  virtual bool CanGet() const = 0;

  // Puts data to this context.
  // Each producer should only call this function once for the same data.
  template <typename T> void Put(T &&value) {
    assert(CanPut());
    PutGeneric(reinterpret_cast<void *>(new T(value)));
  }
  // Gets data from this context.
  // Conceptually, there are N copies for N consumers.
  // Each consumer should only call this function once for the same data.
  template <typename T> DataRef<T> Get() {
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
  virtual void *GetGeneric() const = 0;

  Node * producer_;
  std::vector<Node *> consumers_;
};

template <typename T>
DataRef<T>::~DataRef() {
  context_->Release();
}


template <typename T> class GenericContext : public Context {
public:
  using Context::Context;

  virtual bool CanPut() const  { return data_ == nullptr; }
  virtual bool CanGet() const { return data_ != nullptr; }

protected:
  virtual void PutGeneric(void *value) {
    ref_count_ = consumers_.size();
    data_ = std::unique_ptr<T>(reinterpret_cast<T *>(value));
  }

  virtual void *GetGeneric() const {
    return reinterpret_cast<void *>(data_.get());
  }

  virtual void Release() {
    if (!--ref_count_) {
      data_ = nullptr;
    }
  }

private:
  int ref_count_;
  std::unique_ptr<T> data_;
};

template <typename T> class StreamContext : public GenericContext<T> {
public:

protected:
  // Unconditionally put.
  virtual bool CanPut() const { return true; }
  
  // Unconditionally hold.
  virtual void Relase() final {};
};

} // namespace graph_executor

#endif
