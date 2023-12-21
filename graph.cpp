#include "graph.h"

#include <iostream>
#include <unordered_set>

namespace graph_executor {

void Graph::SetUp(int num_threads) {
  // Figures out inputs and outputs.
  std::vector<Context *> inputs, outputs;
  for (const std::unique_ptr<Context> &c : contexts_) {
    if (!c->Producer()) {
      inputs.push_back(c.get());
    }
    if (c->Consumers().empty()) {
      outputs.push_back(c.get());
    }
  }

  // Uses special `NoOpNode` to represents them, starting the begining and
  // the end of execution.
  input_node_ = new NoOpNode();
  nodes_.emplace_back(input_node_);
  input_node_->Bind({}, inputs);

  output_node_ = new NoOpNode();
  nodes_.emplace_back(output_node_);
  output_node_->Bind(outputs, {});

  // Initiates all the threads.
  active_ = true;
  for (int i = 0; i < num_threads; ++i) {
    threads_.emplace_back(std::thread(&Graph::WorkerThread, this, i));
  }
}

void Graph::TearDown() {
  // Teardown all the threads.
  active_ = false;
  worker_cv_.notify_all();
  for (auto &t : threads_) {
    t.join();
  }
}

void Graph::WorkerThread(int thread_id) {
  while (true) {
    // Waits for the client to assign task.
    std::unique_lock<std::mutex> lk(worker_mutex_);
    worker_cv_.wait(lk);

    std::cerr << "Worker awake!\n";
    // The graph is being deconstructed, teardown the thread.
    if (!active_) {
      std::cerr << "Worker return as graph is deconstructing!\n";
      return;
    }

    // Try to get a task from the queue.
    Node *node = nullptr;
    {
      std::lock_guard<std::mutex> lk(queue_mutex_);
      if (!queue_.empty()) {
        node = queue_.front();
        queue_.pop();
      }
    }

    // If the node is empty, then thread failed to get a task, restart waiting.
    if (node == nullptr) {
      std::cerr << "Worker sleep as failed to get task!\n";
      continue;
    }

    // If the node is the output node, then the current execution finished,
    // notifies client, restart waiting.
    if (node == output_node_) {
      std::cerr << "Worker sleep as all tasks are finished!\n";
      client_cv_.notify_one();
      continue;
    }

    // Executes the node.
    std::cerr << "Worker " << thread_id << " execute node " << node->Name()
              << "!\n";
    node->Execute();

    // Puts all the downstream nodes that are ready into the queue.
    {
      std::lock_guard<std::mutex> lk(queue_mutex_);
      std::unordered_set<Node *> iterated_nodes;
      bool pushed = false;
      for (Context *c : node->Outputs()) {
        for (Node *n : c->Consumers()) {
          // Avoids repeated queires as they requires locking and is expensive.
          if (iterated_nodes.count(n)) {
            continue;
          }
          iterated_nodes.insert(n);
          if (n->IsReady()) {
            queue_.push(n);
            pushed = true;
          }
        }
      }
      // If any new tasks are created, notifies the client to dispatch tasks.
      if (pushed) {
        client_cv_.notify_one();
      }
    }
  }
}

void Graph::Execute() {
  std::cerr << "Client start!\n";
  // Executes input node. Let a worker thread figures out all the node ready
  // to dispath and put them into the queue.
  queue_.push(input_node_);

  while (!queue_.empty()) {
    std::cerr << "Client dispatch!\n";
    if (queue_.size() == 1) {
      worker_cv_.notify_one();
    } else {
      worker_cv_.notify_all();
    }

    // Wait for tasks to be generated.
    std::unique_lock<std::mutex> lk(client_mutex_);
    client_cv_.wait(lk);
  }
}

} // namespace graph_executor
