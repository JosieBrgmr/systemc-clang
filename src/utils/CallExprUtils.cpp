#include "CallExprUtils.h"

#include "clang/AST/DeclCXX.h"
#include "clang/AST/ExprCXX.h"
#include "llvm/Support/Debug.h"

namespace sc_ast_matchers {
namespace utils {

using namespace clang;
using namespace llvm;

void collect_sugar(const Type *type,
                   std::vector<clang::Type *> &unwrapped_types) {
  const Type *desugared_final{type->getUnqualifiedDesugaredType()};
  Type *curr_type{const_cast<Type *>(type)};
  unwrapped_types.push_back(const_cast<Type *>(desugared_final));
  while (curr_type != desugared_final) {
    unwrapped_types.push_back(curr_type);
    curr_type = const_cast<Type *>(
        curr_type->getLocallyUnqualifiedSingleStepDesugaredType().getTypePtr());
  }
}

bool isInNamespace(const clang::Type *tp,
                   const std::vector<llvm::StringRef> &names) {
  if (!tp) {
    return false;
  }

  // Type *tap = const_cast<Type *>(tp->getUnqualifiedDesugaredType());

  /// Peel off every type and then check that each type (including typedef) is of a certain namespace or not.
  std::vector<clang::Type *> unwrapped_types{};
  collect_sugar(tp, unwrapped_types);

  for (auto tap : unwrapped_types) {
  llvm::dbgs() << "@@@@ isNS type\n";
    if (tap->isBuiltinType()) {
      return false;
    }

    DeclContext *dc{nullptr};
    if (tap->isArrayType()) {
      auto cat1 = dyn_cast<ArrayType>(tap);
      const Type *tp = cat1->getElementType().getTypePtr();

      // Unwrap array element type.
      Type *unwrap_tp{nullptr};
      while (tp && tp != unwrap_tp) {
        if (auto cat2 = dyn_cast<ArrayType>(tp)) {
          unwrap_tp = const_cast<Type *>(tp);
          tp = cat2->getElementType().getTypePtr();
        } else {
          unwrap_tp = const_cast<Type *>(tp);
        }
      }

      tap = unwrap_tp;
    }

    if (tap->isRecordType()) {
      const RecordDecl *rdecl = tap->getAsRecordDecl();
      dc = const_cast<clang::DeclContext *>(rdecl->getLexicalParent());
    }

    if (dc && dc->isNamespace()) {
      if (const auto *nd = llvm::dyn_cast<clang::NamespaceDecl>(dc)) {
        auto iinfo = nd->getIdentifier();
        // llvm::dbgs() << "@@ name is " << iinfo->getName() << " for ";
        for (const auto name : names) {
          if (iinfo->isStr(name)) {
            return true;
          }
        }
        return false;
      }
    }
  }

  return false;
}

bool isInNamespace(const CallExpr *cexpr,
                   const std::vector<llvm::StringRef> &names) {
  if (!cexpr) {
    return false;
  }
  return isInNamespace(cexpr->getType().getTypePtr(), names);
}

bool isInNamespace(const CallExpr *cexpr, llvm::StringRef name) {
  if (!cexpr) {
    return false;
  }
  std::vector<llvm::StringRef> names{name};
  return isInNamespace(cexpr->getType().getTypePtr(), names);
}
}  // namespace utils
}  // namespace sc_ast_matchers
