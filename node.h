#ifndef _NODE_H_
#define _NODE_H_

#include <vector>

#include "context.h"

namespace graph_executor {

// Virtual base class of execution nodes.
class Node {
 public:
  Node() {}
  virtual ~Node() {}

  // Not copyable. Movable.
  Node(const Node& other) = delete;
  Node(Node&& other) = default;

  const std::vector<Context*>& Inputs() const { return inputs_; }
  const std::vector<Context*>& Outputs() const { return outputs_; }

  void Bind(const std::vector<Context*>& inputs,
            const std::vector<Context*>& outputs);

  // Whether the node has all the inputs produced and all the previous iteration
  // outputs consumed.
  bool IsReady() const;
  // Notifies downstream that execution is finished.
  void NotifyComplete() const;
  void ExecuteWithNotification() const {
    Execute();
    NotifyComplete();
  };

  virtual void Execute() const = 0;

  friend class Context;

 protected:
  std::vector<Context*> inputs_;
  std::vector<Context*> outputs_;
};

}  // namespace graph_executor

#endif
