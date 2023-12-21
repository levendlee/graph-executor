#ifndef _NODE_H_
#define _NODE_H_

#include <string>
#include <vector>

#include "context.h"

namespace graph_executor {

class Graph;

// Virtual base class of execution nodes.
class Node {
public:
  Node(const std::string &name = "") : name_(name){};
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

  // Whether the node has all the current inputs produced and all the previous
  // outputs consumed.
  bool IsReady() const;

  // Executes the node.
  virtual void Execute() const = 0;

  friend class Context;
  friend class Graph;

protected:
  std::string name_;
  std::vector<Context *> inputs_;
  std::vector<Context *> outputs_;
};

class NoOpNode : public Node {
public:
  using Node::Node;

  virtual void Execute() const {}
};

} // namespace graph_executor

#endif
