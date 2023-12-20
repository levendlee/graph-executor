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
  void Execute() const {
    DataRef<T> lhs = inputs_[0]->template Get<T>();
    DataRef<T> rhs = inputs_[1]->template Get<T>();
    T result = *lhs + *rhs;
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
  Node *n = new Add<int>();
  Context *c0 = new GenericContext<int>();
  Context *c1 = new GenericContext<int>();
  Context *c2 = new StreamContext<int>();

  n->Bind({c0, c1}, {c2});
  
  Graph graph(WrapUniquePtr<Node>({n}), WrapUniquePtr<Context>({c0, c1, c2}));

  // 1st Execution.
  c0->Put(1);
  c1->Put(2);
  graph.Execute();
  std::cout << "Output: " << (c2->CanGet() ? "Produced" : "Not Produced")
            << "\n";
  std::cout << "Value: " << *c2->Get<int>() << "\n";
  
  // 2nd Execution.
  c0->Put(10);
  c1->Put(20);
  graph.Execute();
  std::cout << "Output: " << (c2->CanGet() ? "Produced" : "Not Produced")
            << "\n";
  std::cout << "Value: " << *c2->Get<int>() << "\n";
}

} // namespace graph_executor

int main() { graph_executor::SimpleGraphRun(); }
