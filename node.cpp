#include "node.h"

#include "context.h"

namespace graph_executor {

void Node::Bind(const std::vector<Context*>& inputs,
                const std::vector<Context*>& outputs) {
  inputs_ = inputs;
  outputs_ = outputs;
};

bool Node::IsReady() const {
  for (const Context* c : outputs_) {
    if (!c->IsConsumed()) {
      return false;
    }
  }
  for (const Context* c : inputs_) {
    if (!c->IsProduced()) {
      return false;
    }
  }
  return true;
};

void Node::NotifyComplete() const {
  for (Context* c : inputs_) {
    c->Consume();
  }
};

}  // namespace graph_executor
