#include "context.h"

namespace graph_executor {

void Context::Consume() {
  if (!--ref_count_) {
    Release();
  }
};

}  // namespace graph_executor
