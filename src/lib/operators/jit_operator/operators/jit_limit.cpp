#include "jit_limit.hpp"

#include <limits>

namespace opossum {

std::string JitLimit::description() const { return "[Limit]"; }

void JitLimit::_consume(JitRuntimeContext& context) const {
  if (context.limit_rows-- == 0) {
    context.chunk_offset = std::numeric_limits<ChunkOffset>::max() - 1;
  } else {
    _emit(context);
  }
}

}  // namespace opossum
