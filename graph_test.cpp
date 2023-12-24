#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "graph.h"

#include <chrono>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>

#include "context.h"
#include "doctest.h"
#include "node.h"

namespace graph_executor {

namespace {

template <typename T> class AddAndDelay : public Node {
public:
  AddAndDelay(const std::string &name = "", int delay_in_seconds = 0)
      : Node(name), delay_in_seconds_(delay_in_seconds) {}

  void Execute() const {
    DataRef<T> lhs = inputs_[0]->template Get<T>();
    DataRef<T> rhs = inputs_[1]->template Get<T>();
    T result = *lhs + *rhs;
    std::this_thread::sleep_for(std::chrono::seconds(delay_in_seconds_));
    outputs_[0]->Put(std::move(result));
  };

private:
  int delay_in_seconds_;
};

template <typename T> auto ToUniquePtrs(std::vector<T *> ptr_vec) {
  std::vector<std::unique_ptr<T>> uniq_ptr_vec;
  for (T *ptr : ptr_vec) {
    uniq_ptr_vec.emplace_back(ptr);
  }
  return uniq_ptr_vec;
}

} // namespace

// Concurrent node execution.
// Sequential graph execution.
TEST_CASE("SequentialGraphConcurrentNode") {
  std::cerr << "SequentialGraphConcurrentNode\n";
  constexpr int num_nodes = 10;
  constexpr int num_contexts = 12;
  constexpr int num_threads = 3;
  constexpr int num_concurrent_runs = 1;

  std::vector<std::unique_ptr<Node>> nodes;
  for (int i = 0; i < num_nodes; ++i) {
    nodes.push_back(std::make_unique<AddAndDelay<int>>(std::to_string(i), 1));
  }

  std::vector<std::unique_ptr<Context>> contexts;
  for (int i = 0; i < num_contexts; ++i) {
    contexts.push_back(
        std::make_unique<GenericContext<int>>(std::to_string(i)));
  }

  // Fabonacci series
  // y1 = x1 + x2
  // y2 = x2 + y1
  // y3 = y1 + y2
  // ...
  for (int i = 0; i < 10; ++i) {
    nodes[i]->Bind({&*contexts[i], &*contexts[i + 1]}, {&*contexts[i + 2]});
  }

  std::vector<Context *> input_contexts = {&*contexts[0], &*contexts[1]};
  Context *output_context = &*contexts[num_contexts - 1];

  // `graph` takes over ownership of `nodes` and `contexts`.
  Graph graph(num_threads, std::move(nodes), std::move(contexts));

  // 1st Execution.
  for (Context *c : input_contexts) {
    CHECK(c->CanPut());
    c->Put(1);
  }
  graph.Execute(num_concurrent_runs);
  CHECK(output_context->CanGet());
  {
    auto output_data = output_context->Get<int>();
    CHECK_EQ(*output_data, 144);
  }

  // 2nd Execution.
  for (Context *c : input_contexts) {
    CHECK(c->CanPut());
    c->Put(10);
  }
  graph.Execute(num_concurrent_runs);
  CHECK(output_context->CanGet());
  {
    auto output_data = output_context->Get<int>();
    CHECK_EQ(*output_data, 1440);
  }
}

// Concurrent node execution.
// Concurrent graph execution.
TEST_CASE("ConcurrentGraphConcurrentNode") {
  std::cerr << "ConcurrentGraphConcurrentNode\n";
  constexpr int num_input_contexts = 8;
  constexpr int num_contexts = 2 * num_input_contexts - 1;
  constexpr int num_nodes = num_input_contexts - 1;
  constexpr int num_threads = 2;
  constexpr int num_concurrent_runs = 10;

  // The ideal buffer size greatly depends on the architecture of the graph.
  // Only the input/output nodes need to have the buffer size same as the
  // number of concurrent runs.
  int buffer_size = num_concurrent_runs;

  std::vector<std::unique_ptr<Node>> nodes;
  for (int i = 0; i < num_nodes; ++i) {
    nodes.push_back(std::make_unique<AddAndDelay<int>>(std::to_string(i), 1));
  }

  std::vector<std::unique_ptr<Context>> contexts;
  for (int i = 0; i < num_contexts; ++i) {
    contexts.push_back(
        std::make_unique<BufferedContext<int>>(buffer_size, std::to_string(i)));
  }

  // Tree reduction
  // y1 = x1 + x2, y2 = x3 + x4, y3 = x5 + x6, y4 = x7 + x8
  // z1 = y1 + y2, z2 = y3 + y4
  // w1 = z1 + z2
  int base = 0;
  for (int width = num_input_contexts / 2; width >= 1; width /= 2) {
    for (int i = 0; i < width; ++i) {
      nodes[base + i]->Bind(
          {&*contexts[2 * (base + i)], &*contexts[2 * (base + i) + 1]},
          {&*contexts[2 * (base + width) + i]});
    }
    base += width;
  }

  std::vector<Context *> input_contexts(num_input_contexts);
  for (int i = 0; i < num_input_contexts; ++i) {
    input_contexts[i] = &*contexts[i];
  }
  for (int i = 0; i < num_concurrent_runs; ++i) {
    for (Context *c : input_contexts) {
      CHECK(c->CanPut());
      int rvalue = i;
      c->Put(std::move(rvalue));
    }
  }
  Context *output_context = &*contexts[num_contexts - 1];

  Graph graph(num_threads, std::move(nodes), std::move(contexts));

  auto start = std::chrono::high_resolution_clock::now();
  graph.Execute(num_concurrent_runs);
  auto stop = std::chrono::high_resolution_clock::now();

  std::cout
      << "Duration: "
      << std::chrono::duration_cast<std::chrono::seconds>(stop - start).count()
      << " s";

  for (int i = 0; i < num_concurrent_runs; ++i) {
    CHECK(output_context->CanGet());
    auto output_data = output_context->Get<int>();
    CHECK_EQ(*output_data, i * num_input_contexts);
  }
}

} // namespace graph_executor
