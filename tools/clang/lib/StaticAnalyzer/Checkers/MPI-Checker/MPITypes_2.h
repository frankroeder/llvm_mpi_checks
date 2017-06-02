#ifndef LLVM_CLANG_LIB_STATICANALYZER_CHECKERS_MPICHECKER_MPITYPES_2_H
#define LLVM_CLANG_LIB_STATICANALYZER_CHECKERS_MPICHECKER_MPITYPES_2_H

#include "clang/StaticAnalyzer/Checkers/MPIFunctionClassifier.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CallEvent.h"
#include "llvm/ADT/SmallSet.h"

namespace clang {
namespace ento {
namespace mpi {

class MPIFile {
public:
  // file can be Open or Close
  enum State : unsigned char { Open, Close };

  MPIFile(State S) : CurrentState{S} {}

  // FastFoldingSetNode - This is a subclass of FoldingSetNode which stores
  // a FoldingSetNodeID value rather than requiring the node to recompute it each time it is needed.
  void Profile(llvm::FoldingSetNodeID &Id) const {
    Id.AddInteger(CurrentState);
  }

  // current state = MPIFiles State
  bool operator==(const MPIFile &ToCompare) const {
    return CurrentState == ToCompare.CurrentState;
  }

  const State CurrentState;
};

// MemRegio Mapping lesen
struct MPIFileMap {};
// LLVM Immutable container to store data structures
// the maps should NOT be stored inside the checker class/state
typedef llvm::ImmutableMap<const clang::ento::MemRegion *,
                           clang::ento::mpi::MPIFile>
    MPIFileMapImpl;

} // end of namespace: mpi

// new defined trait
// state by calling State->get<TraitName>(), or set<MapName>(Key,Value)
template <>
struct ProgramStateTrait<mpi::MPIFileMap>
    : public ProgramStatePartialTrait<mpi::MPIFileMapImpl> {
  static void *GDMIndex() {
    static int index = 0;
    return &index;
  }
};

} // end of namespace: ento
} // end of namespace: clang
#endif
