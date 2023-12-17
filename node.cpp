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
    if (!c->IsAllConsumed()) {
      return false;
    }
  }
  // All current inputs are produced.
  for (const Context *c : inputs_) {
    if (!c->IsProduced()) {
      return false;
    }
  }
  return true;
}

void Node::NotifyComplete() const {
  // All outputs are produced.
  for (Context *c : outputs_) {
    c->MarkProduced();
  }
  // All inputs are consumed.
  for (Context *c : inputs_) {
    c->MarkConsumed();
  }
}

} // namespace graph_executor
