#ifndef _GRAPH_H_
#define _GRAPH_H_

#include <condition_variable>
#include <memory>
#include <queue>
#include <thread>
#include <vector>

#include "context.h"
#include "node.h"

namespace graph_executor {

// Virtual base class of execution graph.
class Graph {
public:
  Graph(int num_threads, std::vector<std::unique_ptr<Node>> &&nodes,
        std::vector<std::unique_ptr<Context>> &&contexts)
      : nodes_(std::move(nodes)), contexts_(std::move(contexts)) {
    SetUp(num_threads);
  }
  virtual ~Graph() { TearDown(); };

  // Not copyable. Not movable.
  Graph(const Graph &other) = delete;
  Graph(Graph &&other) = delete;

  void Execute(int num_executions = 1);

private:
  // Set up threads.
  void SetUp(int num_threads);
  // TearDown threads.
  void TearDown();

  // Worker thread.
  void WorkerThread(int thread_id);

  bool active_;
  int concurrent_execution_;

  std::mutex mutex_;
  std::condition_variable client_cv_;
  std::condition_variable worker_cv_;

  std::queue<Node *> queue_;
  std::vector<std::thread> threads_;

  NoOpNode *input_node_;
  NoOpNode *output_node_;
  std::vector<std::unique_ptr<Node>> nodes_;
  std::vector<std::unique_ptr<Context>> contexts_;
};

} // namespace graph_executor

#endif
