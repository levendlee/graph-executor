#include "graph.h"

#include <iostream>
#include <unordered_set>

namespace graph_executor {

namespace {
std::unordered_set<Node*> GetConsumerNodes(Node* node) {
  std::unordered_set<Node*> consumers;
  for (Context *c : node->Outputs()) {
    for (Node *n : c->Consumers()) {
      consumers.insert(n);
    }
  }
  return consumers;
}
}

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
  input_node_ = new NoOpNode("input");
  nodes_.emplace_back(input_node_);
  input_node_->Bind({}, inputs);

  output_node_ = new NoOpNode("output");
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
  {
    std::unique_lock<std::mutex> lk(mutex_);
    active_ = false;
  }
  worker_cv_.notify_all();
  for (auto &t : threads_) {
    t.join();
  }
}

void Graph::WorkerThread(int thread_id) {
  while (true) {
    Node *node = nullptr;
    bool complete = false;
    {
      // Waits for the client to assign task or to tear down the thread.
      // Skip waiting if there is existing tasks or thread is being teared
      // down to avoid race condition.
      std::unique_lock<std::mutex> lk(mutex_);
      if (queue_.empty() && active_) {
        worker_cv_.wait(lk, [&] { return !queue_.empty() || !active_; });
      }

      // The graph is being deconstructed, teardown the thread.
      if (!active_) {
        return;
      }

      // Gets a task from the queue.
      node = queue_.front();
      queue_.pop();

      // Notifies sleeping client the work is done.
      if (node == output_node_) {
        if (!--concurrent_execution_) {
          complete = true;
        }
      }
    }

    if (complete) {
      client_cv_.notify_all();
      continue;
    }

    // Runs the task outside of lock.
    node->Execute();
   
    // Finds new tasks from downstream consumers.
    std::unordered_set<Node*> consumers = GetConsumerNodes(node);
    {
      std::unique_lock<std::mutex> lk(mutex_);
      for (Node* n : consumers) {
        if (n->IsReady()) {
          queue_.push(n);
        }
      }
    }

    // Notifies sleeping workers to run tasks.
    worker_cv_.notify_all();
  }
}

void Graph::Execute(int num_executions) {
  // Executes input nodes.
  {
    std::unique_lock<std::mutex> lk(mutex_);
    for (int i = 0; i < num_executions; ++i) {
      queue_.push(input_node_);
    }
    concurrent_execution_ = num_executions;
  }
  
  worker_cv_.notify_all();

  {
    std::unique_lock<std::mutex> lk(mutex_);
    // Waits for a worker to notify completion.
    // Skip waiting if the job is already completed to avoid race condition.
    if (!concurrent_execution_) {
      return;
    }
    client_cv_.wait(lk);
  }
}

} // namespace graph_executor
