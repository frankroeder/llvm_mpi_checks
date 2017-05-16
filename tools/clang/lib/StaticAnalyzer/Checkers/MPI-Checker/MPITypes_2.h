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
  enum State : unsigned char { Open, Close };

  MPIFile(State S) : CurrentState{S} {}

  void Profile(llvm::FoldingSetNodeID &Id) const {
    Id.AddInteger(CurrentState);
  }

  bool operator==(const MPIFile &ToCompare) const {
    return CurrentState == ToCompare.CurrentState;
  }

  const State CurrentState;
};


struct MPIFileMap {};
typedef llvm::ImmutableMap<const clang::ento::MemRegion *,
                           clang::ento::mpi::MPIFile>
    MPIFileMapImpl;

} // end of namespace: mpi

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
