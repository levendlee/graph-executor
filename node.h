#ifndef _NODE_H_
#define _NODE_H_

#include <string>
#include <vector>

#include "context.h"

namespace graph_executor {

class Graph;

// `Node` class itself doesn't handle any concurrency issues as it is stateless.
// The user of `Node` class should handle it.

// Virtual base class of execution nodes.
class Node {
public:
  explicit Node(const std::string &name = "") : name_(name){};
  virtual ~Node() = default;

  // Not copyable. Movable.
  Node(const Node &other) = delete;
  Node(Node &&other) = default;

  const std::string &Name() const { return name_; }
  const std::vector<Context *> &Inputs() const { return inputs_; }
  const std::vector<Context *> &Outputs() const { return outputs_; }

  // Binds the node to input and output contexts.
  void Bind(const std::vector<Context *> &inputs,
            const std::vector<Context *> &outputs);

  // Whether the node can get inputs and put outputs.
  bool IsReady() const;

  // Executes the node.
  virtual void Execute() const = 0;

protected:
  std::string name_;
  std::vector<Context *> inputs_;
  std::vector<Context *> outputs_;
};

// A special execution node that does nothing. It can be used as inputs/outputs
// nodes to concatenate contexts and create dependency.
class NoOpNode : public Node {
public:
  using Node::Node;

  virtual void Execute() const {}
};

} // namespace graph_executor

#endif
