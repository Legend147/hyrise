#include "array_expression.hpp"

#include "expression_utils.hpp"
#include "utils/assert.hpp"

namespace opossum {

ArrayExpression::ArrayExpression(const std::vector<std::shared_ptr<AbstractExpression>>& elements):
AbstractExpression(ExpressionType::Array, elements){

}

DataType ArrayExpression::data_type() const {
  Fail("An ArrayExpression doesn't have a single type, each of its elements might have a different type");
}

bool ArrayExpression::is_nullable() const {
  if (elements().empty()) return false;

  const auto nullable = elements().front()->is_nullable();
  Assert(std::all_of(elements().begin(), elements().end(), [&](const auto& value) { return value->is_nullable() == nullable;}), "Nullability of Array elements is inconsistent");

  return nullable;
}

const std::vector<std::shared_ptr<AbstractExpression>>& ArrayExpression::elements() const {
  return arguments;
}

std::shared_ptr<AbstractExpression> ArrayExpression::deep_copy() const {
    return std::make_shared<ArrayExpression>(expressions_copy(arguments));
}

std::string ArrayExpression::as_column_name() const {
  Fail("Notyetimplemented");
  return "";
}

}  // namespace opossum
