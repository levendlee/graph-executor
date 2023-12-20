#include <cassert>
#include <iostream>

#include "context.h"

namespace graph_executor {

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
