#include "graph.h"

namespace graph_executor {

void Graph::Execute() const {
  bool has_execution = false;
  do {
    has_execution = false;
    for (const std::unique_ptr<Node>& node : nodes_) {
      if (node->IsReady()) {
        node->ExecuteWithNotification();
        has_execution = true;
      }
    }
  } while (has_execution);
};

}  // namespace graph_executor
