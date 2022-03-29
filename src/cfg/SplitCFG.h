#ifndef _SPLIT_CFG_H_
#define _SPLIT_CFG_H_

#include <unordered_map>

#include "SplitCFGBlock.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/SmallPtrSet.h"

namespace systemc_clang {

class SupplementaryInfo {
 public:
  SupplementaryInfo(const SplitCFGBlock *block);
  SupplementaryInfo(const SupplementaryInfo &from);
  SupplementaryInfo &operator=(const SupplementaryInfo &from);
  virtual ~SupplementaryInfo();
  int getPathId() const;
  int getFalseId() const;
  const SplitCFGBlock* getSplitCFGBlock() const;

 public:
  const SplitCFGBlock *split_block_;
  int false_idx_;
  int path_idx_;
};

/// ===========================================
/// SplitCFGPathInfo
/// ===========================================
class SplitCFGPathInfo {
 public:
  using SplitCFGBlockPtrVector = llvm::SmallVector<const SplitCFGBlock *>;
  friend class SplitCFG;

 public:
  SplitCFGPathInfo(const SplitCFGBlock *block);

  /// \brief Copy constructor for SplitCFGPathInfo.
  SplitCFGPathInfo(const SplitCFGPathInfo &from);

  SplitCFGPathInfo &operator=(const SplitCFGPathInfo &from);

  virtual ~SplitCFGPathInfo();

  /// \brief Return if there is a valid TRUE path.
  bool isTruePathValid() const { return (true_path_.size() > 0); }

  /// \brief Return if there is a valid FALSE path.
  bool isFalsePathValid() const { return ((false_path_.size() > 0)); }

  /// \brief Return the list of blocks visited on the TRUE path.
  const SplitCFGBlockPtrVector &getTruePath() const { return true_path_; }

  /// \brief Return the list of blocks visited on the FALSE path.
  const SplitCFGBlockPtrVector &getFalsePath() const { return false_path_; }

  int getpathix() { return false_startix; }

  /// \brief Converts the TRUE path into a string for testing.
  std::string toStringFalsePath() const;

  /// \brief Converts the FALSE path into a string for testing.
  std::string toStringTruePath() const;

  /// \brief Dump the paths.
  void dump() const;

 private:
  const SplitCFGBlock *split_block_;
  const clang::CFGBlock *cfg_block_;
  SplitCFGBlockPtrVector true_path_;
  int false_startix;
  int path_idx_;
  SplitCFGBlockPtrVector false_path_;
};

/// ===========================================
/// SplitCFG
/// ===========================================
class SplitCFG {
 public:
  using SplitCFGPathPair = std::pair<const SplitCFGBlock *, SupplementaryInfo>;
  using SplitCFGPath = llvm::SmallVector<SplitCFGPathPair>;
  using FalseIx = std::pair<int, int>;
  using FalseIXVec = llvm::SmallVector<FalseIx>;

  // TODO: deprecated
  using VectorSplitCFGBlock = llvm::SmallVector<const SplitCFGBlock *>;
  using VectorSplitCFGBlockImpl = llvm::SmallVector<const SplitCFGBlock *>;
  using VectorCFGElementPtrImpl =
      llvm::SmallVectorImpl<const clang::CFGElement *>;
  using VectorCFGElementPtr = llvm::SmallVector<const clang::CFGElement *>;

 private:
  /// \brief The context necessary to access translation unit.
  clang::ASTContext &context_;

  /// \brief The saved CFG for a given method.
  std::unique_ptr<clang::CFG> cfg_;

  /// \brief The split blocks in the CFG.
  std::unordered_map<const clang::CFGBlock *, SplitCFGBlock> split_blocks_;

  /// \brief Paths of BBs generated.
  llvm::SmallVector<SplitCFGPath> paths_;

  /// \brief Paths vectors: for conditional blocks, record false path index,
  /// length
  // llvm::SmallVector<llvm::SmallVector<int>> paths_falseix;
  llvm::SmallVector<llvm::SmallVector<std::pair<int, int>>> paths_falseix;

  /// \brief The block id to block for SCCFG.
  std::unordered_map<unsigned int, SplitCFGBlock *> sccfg_;

  llvm::SmallVector<std::pair<VectorCFGElementPtrImpl, bool>> split_elements;

  /// \brief Predecessor SplitCFGBlock* => (Wait SplitCFGBlock*)
  std::unordered_map<const SplitCFGBlock *,
                     std::pair<const SplitCFGBlock *, unsigned int>>
      wait_next_state_;

  /// \brief Map a SplitCFGBlock* to its path information.  Stores the path
  /// information for the blocks that are important.  The important blocks are
  /// (1) conditionals, and (2) loop blocks with two outgoing edges.
  std::unordered_map<const SplitCFGBlock *, SplitCFGPathInfo> path_info_;

  unsigned int next_state_count_;

 private:
  /// \brief Checks if a CFGBlock has a wait() call in it.
  bool isElementWait(const clang::CFGElement &element) const;

  /// \brief Split a CFGBlock into respective SplitCFGBlock if the CFGBlock has
  /// wait statements in it.
  void splitBlock(clang::CFGBlock *block);

  /// \brief Add successors to the SplitCFGBlock.
  void addSuccessors(SplitCFGBlock *to, const clang::CFGBlock *from);

  /// \brief Add predecessors to the SplitCFGBlock.
  void addPredecessors(SplitCFGBlock *to, const clang::CFGBlock *from);

  void addNextStatesToBlocks();

  /// \brief Creates SplitCFGBlocks for all CFGBlocks that do not have a wiat.
  /// splitBlock() creates the SplitCFGBlocks used for splitting CFGBLocks that
  /// have wait statements.
  void createUnsplitBlocks();

  /// \brief Creates the SplitCFGBlocks for CFGBlock with a wait.
  void createWaitSplitCFGBlocks(
      clang::CFGBlock *block,
      const llvm::SmallVectorImpl<std::pair<VectorCFGElementPtr, bool>>
          &split_elements);

  /// \brief Copy false_ix from SplitGraphPathInfo of curr_path into
  /// paths_false_ix
  void setFalseix(
      // llvm::SmallVector<std::pair<const SplitCFGBlock *, SplitCFGPathInfo>>
      SplitCFGPath &curr_path);

  /// \brief Dump all the CFGElements that were split.
  void dumpSplitElements(
      const llvm::SmallVector<std::pair<VectorCFGElementPtr, bool>>
          &split_elements) const;

  void dumpSCCFG() const;

 public:
  /// \brief Constructor.
  SplitCFG(clang::ASTContext &context);
  /// \brief  Overloaded constructor.
  SplitCFG(clang::ASTContext &context, const clang::CXXMethodDecl *cxx_decl);

  /// \brief Disallow a copy constructor for SCCFG.
  SplitCFG(const SplitCFG &from) = delete;

  /// \brief Disallow assignment operator.
  SplitCFG &operator=(const SplitCFG &) = delete;

  /// \brief  Destructor that erases all SplitCFGBlocks created.
  virtual ~SplitCFG();

  /// \brief Returns the paths that were found in the SCCFG.
  // const llvm::SmallVectorImpl<VectorSplitCFGBlock> &

  const llvm::SmallVectorImpl<llvm::SmallVector<SplitCFGPathPair>>
      &getPathsFound();

  const llvm::SmallVector<FalseIXVec> &get_paths_falseix() {
    return paths_falseix;
  }

  /// \brief Construct the SCCFG.
  void construct_sccfg(const clang::CXXMethodDecl *method);

  /// \brief Generates the paths between wait statements.
  void generate_paths();

  const std::unordered_map<const SplitCFGBlock *, SplitCFGPathInfo>
      &getPathInfo() const;

  void preparePathInfo();
  /// \brief Returns the argument to a wait statement.
  /// Note that the only one supported are no arguments or integer arguments.
  llvm::APInt getWaitArgument(const clang::CFGElement &element) const;

  /// Dump member functions.
  void dump() const;
  void dumpToDot() const;
  void dumpWaitNextStates() const;
  void dumpPaths() const;
  void dumpCurrPath(
      // llvm::SmallVector<std::pair<const SplitCFGBlock *, SplitCFGPathInfo>>
      SplitCFGPath &curr_path) const;

  void inline dumpFalseIx() {
    for (int i = 0; i < paths_falseix.size(); i++) {
      llvm::dbgs() << "S" << i << " falseix path index, length pair: ";
      for (std::pair<int, int> flsix_pair : paths_falseix[i]) {
        llvm::dbgs() << "(" << flsix_pair.first << ", " << flsix_pair.second
                     << ") ";
      }
      llvm::dbgs() << "\n";
    }
  }

  void dumpPathInfo() const;

  /// Rework
  //
  //

  template <typename T>
  void dumpSmallVector(llvm::SmallVectorImpl<T> &vlist) {
    for (const auto v : vlist) {
      llvm::dbgs() << v.first->getBlockID() << " ";
    }
  }

  /// \brief Modified DFS to create all paths within wait statements and from
  /// the root node.
  /// \param basic_block The current basic block to process.
  /// \param waits_in_stack The SplitCFGBlock that come after the wait
  /// statements. These need to be processed.
  /// \param visited_waits These are the SplitCFGBlocks that have waits and
  /// those that have been visited.

  const SplitCFGPath  // llvm::SmallVector<std::pair<const SplitCFGBlock *,
                      // SplitCFGPathInfo>>
  dfs_visit_wait(const SplitCFGBlock *BB,
                 llvm::SmallPtrSet<const SplitCFGBlock *, 32> &visited_blocks,
                 llvm::SmallVectorImpl<const SplitCFGBlock *> &waits_to_visit,
                 llvm::SmallPtrSetImpl<const SplitCFGBlock *> &visited_waits,
                 llvm::SmallVector<SplitCFGPathPair> &curr_path);
  void dfs_rework();
  bool isLoop(const SplitCFGBlock *block) const;
  bool isConditional(const SplitCFGBlock *block) const;
  bool getUnvisitedSuccessor(
      const SplitCFGBlock *curr_block, SplitCFGBlock::const_succ_iterator &I,
      llvm::SmallPtrSetImpl<const SplitCFGBlock *> &visited,
      const SplitCFGBlock *&block);
  bool isLoopWithTwoSuccessors(const SplitCFGBlock *block) const;
  void addSuccessorToVisitOrPop(
      bool parent_has_wait, const SplitCFGBlock *BB,
      llvm::SmallVector<
          std::pair<const SplitCFGBlock *, SplitCFGBlock::const_succ_iterator>,
          8> &to_visit,
      bool found);

  bool isTruePath(const SplitCFGBlock *parent_block,
                  const SplitCFGBlock *block) const;

  /// \brief Compute the set difference between two SmallPtrSets.
  void setDifference(
      const llvm::SmallPtrSetImpl<const SplitCFGBlock *> &larger,
      const llvm::SmallPtrSetImpl<const SplitCFGBlock *> &smaller,
      llvm::SmallPtrSetImpl<const SplitCFGBlock *> &to);
  void setTruePathInfo(const SplitCFGBlock *sblock,
                       // const llvm::SmallVector< std::pair<const SplitCFGBlock
                       // *, SplitCFGPathInfo>> &newly_visited,
                       const SplitCFGPath &newly_visited, int ix = -1);

  void setFalsePathInfo(const SplitCFGBlock *sblock,
                        const SplitCFGPath &newly_visited);
  // const llvm::SmallVector<
  // std::pair<const SplitCFGBlock *, SplitCFGPathInfo>> &newly_visited);

  void updateVisitedBlocks(
      llvm::SmallPtrSetImpl<const SplitCFGBlock *> &to,
      const llvm::SmallPtrSetImpl<const SplitCFGBlock *> &from);
  void dumpVisitedBlocks(llvm::SmallPtrSetImpl<const SplitCFGBlock *> &visited);

  bool popping_;
};

};  // namespace systemc_clang

#endif /* _SPLIT_CFG_H_ */
