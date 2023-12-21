#include "graph.h"

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

} // namespace graph_executor

int main() { graph_executor::SimpleGraphRun(); }
