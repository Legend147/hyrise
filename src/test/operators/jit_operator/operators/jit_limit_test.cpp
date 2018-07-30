#include "../../../base_test.hpp"
#include "operators/jit_operator/operators/jit_limit.hpp"

namespace opossum {

namespace {

// Mock JitOperator that records whether tuples are passed to it
class MockSink : public AbstractJittable {
 public:
  std::string description() const final { return "MockSink"; }

  void reset() const { _consume_was_called = false; }

  bool consume_was_called() const { return _consume_was_called; }

 private:
  void _consume(JitRuntimeContext& context) const final { _consume_was_called = true; }

  // Must be static, since _consume is const
  static bool _consume_was_called;
};

bool MockSink::_consume_was_called = false;

// Mock JitOperator that passes on individual tuples
class MockSource : public AbstractJittable {
 public:
  std::string description() const final { return "MockSource"; }

  void emit(JitRuntimeContext& context) { _emit(context); }

 private:
  void _consume(JitRuntimeContext& context) const final {}
};

}  // namespace

class JitLimitTest : public BaseTest {};

TEST_F(JitLimitTest, FiltersTuplesAccordingToLimit) {
  ChunkOffset chunk_offset{123};

  JitRuntimeContext context;
  context.chunk_offset = chunk_offset;
  context.limit_rows = 1;

  auto source = std::make_shared<MockSource>();
  auto limit = std::make_shared<JitLimit>();
  auto sink = std::make_shared<MockSink>();

  // Link operators to pipeline
  source->set_next_operator(limit);
  limit->set_next_operator(sink);

  // limit not reached
  sink->reset();
  source->emit(context);
  ASSERT_TRUE(sink->consume_was_called());
  ASSERT_EQ(context.chunk_offset, chunk_offset);
  ASSERT_EQ(context.limit_rows, int64_t(0));

  // limit reached
  sink->reset();
  source->emit(context);
  ASSERT_FALSE(sink->consume_was_called());
  ASSERT_EQ(context.chunk_offset, std::numeric_limits<ChunkOffset>::max() - 1);
  ASSERT_EQ(context.limit_rows, int64_t(-1));
}

}  // namespace opossum
