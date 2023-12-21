#include "node.h"

#include "context.h"

namespace graph_executor {

void Node::Bind(const std::vector<Context *> &inputs,
                const std::vector<Context *> &outputs) {
  inputs_ = inputs;
  outputs_ = outputs;
  for (Context *c : inputs) {
    c->BindConsumer(this);
  }
  for (Context *c : outputs) {
    c->BindProducer(this);
  }
}

bool Node::IsReady() const {
  // Ensure we can put outputs to context.
  for (Context *c : outputs_) {
    if (!c->CanPut()) {
      return false;
    }
  }
  // Ensure we can get inputs from context.
  for (Context *c : inputs_) {
    if (!c->CanGet()) {
      return false;
    }
  }
  return true;
}

} // namespace graph_executor
