/*
 *  Used to turn logging off.
 *  NoLogger does not log anything and just calls the commit callback of corresponding transactions.
 */

#pragma once

#include "abstract_logger.hpp"

#include "types.hpp"

namespace opossum {

class NoLogger : public AbstractLogger {
 public:
  NoLogger(const NoLogger&) = delete;
  NoLogger& operator=(const NoLogger&) = delete;

  void log_commit(const TransactionID transaction_id, std::function<void(TransactionID)> callback) override {
    callback(transaction_id);
  };

  void log_value(const TransactionID transaction_id, const std::string& table_name, const RowID row_id,
                 const std::vector<AllTypeVariant>& values) override{};

  void log_invalidate(const TransactionID transaction_id, const std::string& table_name, const RowID row_id) override{};

  void log_load_table(const std::string& file_path, const std::string& table_name) override{};

  void log_flush() override{};

  uint32_t recover() override { return 0u; };

  NoLogger() : AbstractLogger(nullptr) {}
};

}  // namespace opossum