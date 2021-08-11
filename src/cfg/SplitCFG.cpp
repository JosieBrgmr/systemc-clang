#include "SplitCFG.h"

#include "llvm/Support/Debug.h"
#include "llvm/ADT/PostOrderIterator.h"

using namespace systemc_clang;

/// ===========================================
/// SplitCFG
/// ===========================================
SplitCFG::SplitCFG(clang::ASTContext& context) : context_{context} {}

void SplitCFG::dfs_pop_on_wait(
    const clang::CFGBlock* BB,
    llvm::SmallVector<const clang::CFGBlock*>& waits_in_stack,
    llvm::SmallPtrSet<const clang::CFGBlock*, 8>& visited_waits) {
  if (BB->succ_empty()) {
    /// Empty CFG block
    return;
  }

  llvm::SmallPtrSet<const clang::CFGBlock*, 8> visited;
  llvm::SmallVector<
      std::pair<const clang::CFGBlock*, clang::CFGBlock::const_succ_iterator>,
      8>
      visit_stack;
  llvm::SmallVector<const clang::CFGBlock*, 8> in_stack;

  visited.insert(BB);
  visit_stack.push_back(std::make_pair(BB, BB->succ_begin()));

  do {
    std::pair<const clang::CFGBlock*, clang::CFGBlock::const_succ_iterator>&
        Top = visit_stack.back();
    const clang::CFGBlock* ParentBB = Top.first;
    clang::CFGBlock::const_succ_iterator& I = Top.second;

    /// If BB has a wait() then just return.
    if (isWait(*ParentBB)) {
      llvm::dbgs() << "BB# " << ParentBB->getBlockID() << " has a wait in it\n";

      /// The wait has not been processed yet so add it to the visited_wait
      /// sets. Then add it in the stack to process the waits.
      if (visited_waits.insert(ParentBB).second == true) {
        // Insert the successor of the block.
        llvm::dbgs() << "Visited BB#" << ParentBB->getBlockID() << "\n";
        while (I != ParentBB->succ_end()) {
          BB = *I++;
          if ((BB != nullptr)) {
            llvm::dbgs() << "Insert successor of BB#" << ParentBB->getBlockID()
                         << ": BB#" << BB->getBlockID() << "\n";
            waits_in_stack.push_back(BB);
          }
        }
      }
    }

    bool FoundNew = false;
    while (I != ParentBB->succ_end()) {
      BB = *I++;
      if ((BB != nullptr) && (!isWait(*ParentBB)) &&
          (visited.insert(BB).second)) {
        FoundNew = true;
        break;
      }
    }

    if (FoundNew) {
      // Go down one level if there is a unvisited successor.
      llvm::dbgs() << "Visited BB#" << ParentBB->getBlockID() << "\n";
      visit_stack.push_back(std::make_pair(BB, BB->succ_begin()));
    } else {
      // Go up one level.
      //llvm::dbgs() << "pop: " << visit_stack.size() << "\n";
      visit_stack.pop_back();
    }
  } while (!visit_stack.empty());
}

bool SplitCFG::isWait(const clang::CFGBlock& block) const {
  for (auto const& element : block.refs()) {
    if (auto cfg_stmt = element->getAs<clang::CFGStmt>()) {
      auto stmt{cfg_stmt->getStmt()};

      // stmt->dump();
      if (auto* expr = llvm::dyn_cast<clang::Expr>(stmt)) {
        if (auto cxx_me = llvm::dyn_cast<clang::CXXMemberCallExpr>(expr)) {
          if (auto direct_callee = cxx_me->getDirectCallee()) {
            auto name{direct_callee->getNameInfo().getAsString()};
            if (name == std::string("wait")) {
              // llvm::dbgs() << "@@@@ FOUND WAIT @@@@\n";

              /// Check that there is only 1 or 0 arguments
              auto args{cxx_me->getNumArgs()};
              if (args >= 2) {
                llvm::errs() << "wait() must have either 0 or 1 argument.\n";
                return false;
              }
              return true;
            }
          }
        }
      }
    }
  }

  return false;
};

void SplitCFG::dfs() {
  /// Set of visited wait blocks.
  llvm::SmallPtrSet<const clang::CFGBlock*, 8> visited_waits;
  llvm::SmallVector<const clang::CFGBlock*> waits_in_stack;

  /// G = cfg_
  const clang::CFGBlock* BB{&cfg_->getEntry()};
  waits_in_stack.push_back(BB);
  do {
    BB = waits_in_stack.pop_back_val();
    llvm::dbgs() << "Processing BB# " << BB->getBlockID() << "\n";
    dfs_pop_on_wait(BB, waits_in_stack, visited_waits);
    llvm::dbgs() << "\n";
  } while (!waits_in_stack.empty());
}

void SplitCFG::split_wait_blocks(const clang::CXXMethodDecl* method) {
  cfg_ = clang::CFG::buildCFG(method, method->getBody(), &context_,
                              clang::CFG::BuildOptions());

  clang::LangOptions lang_opts;
  cfg_->dump(lang_opts, true);

  // Go through all CFG blocks
  llvm::dbgs() << "Iterate through all CFGBlocks.\n";
  /// These iterators are not in clang 12.
  // for (auto const &block: CFG->const_nodes()) {
  for (auto begin_it = cfg_->nodes_begin(); begin_it != cfg_->nodes_end();
       ++begin_it) {
    auto block{*begin_it};

    /// Try to split the block.
    SplitCFGBlock sp{};
    sp.split_block(block);

    //////////////////////////////////////////
    //
    if (sp.hasWait()) {
      /// The block has been split.
      split_blocks_.insert(
          std::pair<const clang::CFGBlock*, SplitCFGBlock>(block, sp));
    }
  }
}

void SplitCFG::dump() const {
  llvm::dbgs() << "Dump all blocks that were split\n";
  for (auto const& sblock : split_blocks_) {
    llvm::dbgs() << "Block ID: " << sblock.first->getBlockID() << "\n";
    sblock.first->dump();
    const SplitCFGBlock* block_with_wait{&sblock.second};
    llvm::dbgs() << "Print all info for split block\n";
    block_with_wait->dump();
  }
}
