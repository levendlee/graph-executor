#ifndef _GRAPH_H_
#define _GRAPH_H_

#include <memory>
#include <vector>

#include "context.h"
#include "node.h"

namespace graph_executor {

// Virtual base class of execution graph.
class Graph {
 public:
  Graph(std::vector<std::unique_ptr<Node>>&& nodes,
        std::vector<std::unique_ptr<Context>>&& contexts)
      : nodes_(std::move(nodes)), contexts_(std::move(contexts)) {}
  virtual ~Graph() {}

  // Not copyable. Movable.
  Graph(const Graph& other) = delete;
  Graph(Graph&& other) = default;

  void Execute() const;

 private:
  std::vector<std::unique_ptr<Node>> nodes_;
  std::vector<std::unique_ptr<Context>> contexts_;
};

}  // namespace graph_executor

#endif
