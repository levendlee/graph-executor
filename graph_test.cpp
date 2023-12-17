#include "graph.h"

#include <iostream>
#include <memory>
#include <utility>
#include <vector>

#include "context.h"
#include "node.h"

namespace graph_executor {

template <typename T>
class Add : public Node {
 public:
  void Execute() const {
    const T& lhs = inputs_[0]->template Evaluate<T>();
    const T& rhs = inputs_[1]->template Evaluate<T>();
    T result = lhs + rhs;
    outputs_[0]->Produce(std::move(result));
  };
};

template <typename T>
auto WrapUniquePtr(std::vector<T*> ptr_vec) {
  std::vector<std::unique_ptr<T>> uniq_ptr_vec;
  for (T* ptr : ptr_vec) {
    uniq_ptr_vec.emplace_back(ptr);
  }
  return uniq_ptr_vec;
}

void SimpleGraphRun() {
  std::vector<Node*> nodes(1);
  nodes[0] = new Add<int>();

  std::vector<Context*> contexts(3);
  std::vector<Node*> input_consumers = {nodes[0]};
  contexts[0] = new GenericContext<int>(nullptr, input_consumers);
  contexts[1] = new GenericContext<int>(nullptr, input_consumers);
  contexts[2] = new GenericContext<int>(nodes[0], {});

  contexts[0]->Produce(1);
  contexts[1]->Produce(2);

  nodes[0]->Bind({contexts[0], contexts[1]}, {contexts[2]});

  Graph graph(WrapUniquePtr(nodes), WrapUniquePtr(contexts));
  graph.Execute();

  std::cout << "Output produced: " << contexts[2]->IsProduced() << "\n";
  std::cout << "Output: " << contexts[2]->Evaluate<int>() << "\n";
}

}  // namespace graph_executor

int main() { graph_executor::SimpleGraphRun(); }
