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
  // All previous outputs are consumed.
  for (const Context *c : outputs_) {
    if (!c->CanPut()) {
      return false;
    }
  }
  // All current inputs are produced.
  for (const Context *c : inputs_) {
    if (!c->CanGet()) {
      return false;
    }
  }
  return true;
}

} // namespace graph_executor
