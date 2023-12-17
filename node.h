#ifndef _NODE_H_
#define _NODE_H_

#include <vector>

#include "context.h"

namespace graph_executor {

class Graph;

// Virtual base class of execution nodes.
class Node {
public:
  Node() = default;
  virtual ~Node() = default;

  // Not copyable. Movable.
  Node(const Node &other) = delete;
  Node(Node &&other) = default;

  const std::vector<Context *> &Inputs() const { return inputs_; }
  const std::vector<Context *> &Outputs() const { return outputs_; }

  // Binds the node to input and output contexts.
  void Bind(const std::vector<Context *> &inputs,
            const std::vector<Context *> &outputs);

  // Whether the node has all the current inputs produced and all the previous
  // outputs consumed.
  bool IsReady() const;

  // Executes the node.
  virtual void Execute() const = 0;

  // Executes the node and notifies the downstream that execution is completed.
  // One can overwrite this function to eagerly marks inputs are consumed when
  // they are no longer needed, marks outputs are produced when they are
  // generated.
  virtual void ExecuteAndNotifyComplete() const {
    Execute();
    NotifyComplete();
  };

  friend class Context;
  friend class Graph;

protected:
  // Notifies downstream that execution is completed.
  void NotifyComplete() const;

  std::vector<Context *> inputs_;
  std::vector<Context *> outputs_;
};

} // namespace graph_executor

#endif
