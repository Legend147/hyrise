#include "jit_write_offset.hpp"

#include "../jit_types.hpp"
#include "constant_mappings.hpp"
#include "resolve_type.hpp"
#include "storage/base_value_column.hpp"
#include "storage/value_column.hpp"

namespace opossum {

std::string JitWriteOffset::description() const {
  std::stringstream desc;
  desc << "[WriteOffset] ";
  for (const auto& output_column : _output_columns) {
    desc << output_column.column_name << " = Col#" << output_column.referenced_column_id << ", ";
  }
  return desc.str();
}

std::shared_ptr<Table> JitWriteOffset::create_output_table(const ChunkOffset input_table_chunk_size) const {
  TableColumnDefinitions column_definitions;

  for (const auto& output_column : _output_columns) {
    // Add a column definition for each output column
    const auto data_type = output_column.data_type;
    DebugAssert(data_type != DataType::Bool, "Jit columns cannot be added to a reference table");
    const auto is_nullable = output_column.is_nullable;
    column_definitions.emplace_back(output_column.column_name, data_type, is_nullable);
  }

  return std::make_shared<Table>(column_definitions, TableType::References, input_table_chunk_size);
}

void JitWriteOffset::before_query(const Table& in_table, Table& out_table, JitRuntimeContext& context) const {
  _create_output_chunk(context, in_table.get_chunk(ChunkID(0))->size());
}

void JitWriteOffset::after_chunk(const std::shared_ptr<const Table>& in_table, Table& out_table,
                                 JitRuntimeContext& context) const {
  if (context.output_pos_list->size() > 0) {
    ChunkColumns out_columns;
    out_columns.reserve(_output_columns.size());
    if (in_table->type() == TableType::References) {
      const auto chunk_in = in_table->get_chunk(context.chunk_id);

      auto filtered_pos_lists = std::map<std::shared_ptr<const PosList>, std::shared_ptr<PosList>>{};

      for (const auto& output_colum : _output_columns) {
        auto column_in = chunk_in->get_column(output_colum.referenced_column_id);

        auto ref_column_in = std::dynamic_pointer_cast<const ReferenceColumn>(column_in);
        DebugAssert(ref_column_in != nullptr, "All columns should be of type ReferenceColumn.");

        const auto pos_list_in = ref_column_in->pos_list();

        const auto table_out = ref_column_in->referenced_table();
        const auto column_id_out = ref_column_in->referenced_column_id();

        auto& filtered_pos_list = filtered_pos_lists[pos_list_in];

        if (!filtered_pos_list) {
          filtered_pos_list = std::make_shared<PosList>();
          filtered_pos_list->reserve(context.output_pos_list->size());

          for (const auto& match : *context.output_pos_list) {
            const auto row_id = (*pos_list_in)[match.chunk_offset];
            filtered_pos_list->push_back(row_id);
          }
        }

        auto ref_column_out = std::make_shared<ReferenceColumn>(table_out, column_id_out, filtered_pos_list);
        out_columns.push_back(ref_column_out);
      }
    } else {
      for (const auto& output_colum : _output_columns) {
        auto ref_column_out =
            std::make_shared<ReferenceColumn>(in_table, output_colum.referenced_column_id, context.output_pos_list);
        out_columns.push_back(ref_column_out);
      }
    }
    out_table.append_chunk(out_columns);
    // check if current chunk is last
    if (context.chunk_id + 1 < in_table->chunk_count()) {
      _create_output_chunk(context, in_table->get_chunk(ChunkID(context.chunk_id + 1))->size());
    }
  }
}

void JitWriteOffset::add_output_column(const JitOutputReferenceColumn& output_column) {
  _output_columns.emplace_back(output_column);
}

std::vector<JitOutputReferenceColumn> JitWriteOffset::output_columns() const { return _output_columns; }

void JitWriteOffset::_consume(JitRuntimeContext& context) const {
  context.output_pos_list->emplace_back(context.chunk_id, context.chunk_offset);
}

void JitWriteOffset::_create_output_chunk(JitRuntimeContext& context, const uint32_t in_chunk_size) const {
  context.output_pos_list = std::make_shared<PosList>();
  context.output_pos_list->reserve(in_chunk_size);
}

}  // namespace opossum
