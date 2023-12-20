#include "graph.h"

namespace graph_executor {

void Graph::Execute() const {
  bool action = false;
  do {
    action = false;
    for (const std::unique_ptr<Node> &node : nodes_) {
      if (node->IsReady()) {
        node->Execute();
        action = true;
      }
    }
  } while (action);
};

} // namespace graph_executor
