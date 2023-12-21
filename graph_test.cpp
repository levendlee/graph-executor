#include "graph.h"

#include <chrono>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>

#include "context.h"
#include "node.h"

namespace graph_executor {

template <typename T> class Add : public Node {
public:
  using Node::Node;

  void Execute() const {
    DataRef<T> lhs = inputs_[0]->template Get<T>();
    DataRef<T> rhs = inputs_[1]->template Get<T>();
    T result = *lhs + *rhs;
    // std::cerr << "Add: " << *lhs << " + " << *rhs << " = " << result << "\n";
    outputs_[0]->Put(std::move(result));
  };
};

template <typename T> auto WrapUniquePtr(std::vector<T *> ptr_vec) {
  std::vector<std::unique_ptr<T>> uniq_ptr_vec;
  for (T *ptr : ptr_vec) {
    uniq_ptr_vec.emplace_back(ptr);
  }
  return uniq_ptr_vec;
}

void SimpleGraphRun() {
  std::vector<Node *> nodes;
  for (int i = 0; i < 10; ++i) {
    nodes.push_back(new Add<int>(std::to_string(i)));
  }

  std::vector<Context *> contexts;
  for (int i = 0; i < 12; ++i) {
    contexts.push_back(new GenericContext<int>(std::to_string(i)));
  }

  // Fabonacci series
  // y1 = x1 + x2
  // y2 = x2 + y1
  // y3 = y1 + y2
  // ...
  for (int i = 0; i < 10; ++i) {
    nodes[i]->Bind({contexts[i], contexts[i + 1]}, {contexts[i + 2]});
  }

  Graph graph(3, WrapUniquePtr(nodes), WrapUniquePtr(contexts));

  // 1st Execution.
  contexts[0]->Put(1);
  contexts[1]->Put(1);
  graph.Execute();
  std::cout << "Output: "
            << (contexts[11]->CanGet() ? "Produced" : "Not Produced") << "\n";
  std::cout << "Value: " << *contexts[11]->Get<int>() << "\n";

  // 2nd Execution.
  contexts[0]->Put(10);
  contexts[1]->Put(10);
  graph.Execute();
  std::cout << "Output: "
            << (contexts[11]->CanGet() ? "Produced" : "Not Produced") << "\n";
  std::cout << "Value: " << *contexts[11]->Get<int>() << "\n";
}

template <typename T> class AddAndDelay : public Node {
public:
  using Node::Node;

  void Execute() const {
    DataRef<T> lhs = inputs_[0]->template Get<T>();
    DataRef<T> rhs = inputs_[1]->template Get<T>();
    T result = *lhs + *rhs;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    outputs_[0]->Put(std::move(result));
  };
};

void ConcurrentGraphRun() {
  int num_threads = 7;
  int concurrent_runs = 10;
  // The ideal buffer size greatly depends on the architecture of the graph.
  // Only the input/output nodes need to have the buffer size same as the
  // number of concurrent runs.
  int buffer_size = concurrent_runs;

  std::vector<Node *> nodes;
  for (int i = 0; i < 7; ++i) {
    nodes.push_back(new AddAndDelay<int>(std::to_string(i)));
  }

  std::vector<Context *> contexts;
  for (int i = 0; i < 15; ++i) {
    contexts.push_back(
        new BufferedContext<int>(buffer_size, std::to_string(i)));
  }

  // Tree reduction
  // y1 = x1 + x2, y2 = x3 + x4, y3 = x5 + x6, y4 = x7 + x8
  // z1 = y1 + y2, z2 = y3 + y4
  // w1 = z1 + z2
  for (int i = 0; i < 4; ++i) {
    nodes[i]->Bind({contexts[2 * i], contexts[2 * i + 1]}, {contexts[8 + i]});
  }
  for (int i = 0; i < 2; ++i) {
    nodes[4 + i]->Bind({contexts[8 + 2 * i], contexts[8 + 2 * i + 1]},
                       {contexts[12 + i]});
  }
  nodes[6]->Bind({contexts[12], contexts[13]}, {contexts[14]});

  Graph graph(7, WrapUniquePtr(nodes), WrapUniquePtr(contexts));

  for (int i = 0; i < concurrent_runs; ++i) {
    for (int j = 0; j < 8; ++j) {
      contexts[j]->Put(i + j);
    }
  }

  auto start = std::chrono::high_resolution_clock::now();
  graph.Execute(concurrent_runs);
  auto stop = std::chrono::high_resolution_clock::now();

  std::cout
      << "Duration: "
      << std::chrono::duration_cast<std::chrono::seconds>(stop - start).count()
      << " s";
}

} // namespace graph_executor

int main() {
  std::cout << "SimpleGraphRun:\n";
  graph_executor::SimpleGraphRun();

  std::cout << "ConcurrentGraphRun:\n";
  graph_executor::ConcurrentGraphRun();
}
