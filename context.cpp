#include <cassert>
#include <iostream>

#include "context.h"

namespace graph_executor {

void Context::MarkProduced() {
  assert(ref_count_ == 0);
  ref_count_ = consumers_.size();
}

void Context::MarkConsumed() {
  assert(ref_count_ > 0);
  if (!--ref_count_) {
    Release();
    has_value_ = false;
  }
}

void Context::BindProducer(Node *producer) {
  assert(producer_ == nullptr);
  producer_ = producer;
}

void Context::BindConsumer(Node *consumer) {
  assert(std::find(consumers_.begin(), consumers_.end(), consumer) ==
         consumers_.end());
  consumers_.push_back(consumer);
}

} // namespace graph_executor
