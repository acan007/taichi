#include <taichi/taichi>
#include <set>
#include "../ir.h"

TLANG_NAMESPACE_BEGIN

namespace irpass {

class Offloader {
 public:
  Offloader(IRNode *root) {
    run(root);
  }

  void run(IRNode *root) {
    auto root_block = dynamic_cast<Block *>(root);
    auto &&root_statements = std::move(root_block->statements);
    auto new_root_statements = std::vector<pStmt>();

    bool has_range_for = false;

    int unclassified = 3;
    for (int i = 0; i < (int)root_statements.size(); i++) {
      auto &stmt = root_statements[i];
      if (auto s = stmt->cast<RangeForStmt>()) {
        auto offloaded =
            Stmt::make_typed<OffloadedStmt>(OffloadedStmt::TaskType::range_for);
        offloaded->body_block = std::make_unique<Block>();
        offloaded->begin = s->begin->as<ConstStmt>()->val[0].val_int32();
        offloaded->end = s->end->as<ConstStmt>()->val[0].val_int32();
        has_range_for = true;
        for (int j = unclassified; j < i; j++) {
          offloaded->body_block->statements.push_back(
              std::move(root_statements[j]));
        }
        for (int j = 0; j < (int)s->body->statements.size(); j++) {
          offloaded->body_block->statements.push_back(
              std::move(s->body->statements[j]));
        }
        new_root_statements.push_back(std::move(offloaded));
        unclassified = i + 3;
        /*
      } else if (auto s = stmt->cast<StructForStmt>()) {
        // TODO: emit listgen
        auto offloaded =
            Stmt::make_typed<OffloadedStmt>(OffloadedStmt::TaskType::struct_for);
        offloaded->body_stmt = std::move(root_statements[i]);
        new_root_statements.push_back(std::move(offloaded));
      } else {
        // Serial stmt
        auto offloaded =
            Stmt::make_typed<OffloadedStmt>(OffloadedStmt::TaskType::serial);
        offloaded->body_stmt = std::move(root_statements[i]);
        new_root_statements.push_back(std::move(offloaded));
         */
      }
    }

    if (!has_range_for) {
      auto offload =
          Stmt::make_typed<OffloadedStmt>(OffloadedStmt::TaskType::serial);
      offload->body_block = std::make_unique<Block>();
      for (int i = 0; i < (int)root_statements.size(); i++) {
        auto &stmt = root_statements[i];
        offload->body_block->statements.push_back(std::move(stmt));
      }
      new_root_statements.push_back(std::move(offload));
    }
    root_block->statements = std::move(new_root_statements);
  }
};

void offload(IRNode *root) {
  Offloader _(root);
}

}  // namespace irpass

TLANG_NAMESPACE_END
