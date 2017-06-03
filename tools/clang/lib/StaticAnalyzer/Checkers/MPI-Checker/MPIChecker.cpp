//===-- MPIChecker.cpp - Checker Entry Point Class --------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file defines the main class of MPI-Checker which serves as an entry
/// point. It is created once for each translation unit analysed.
/// The checker defines path-sensitive checks, to verify correct usage of the
/// MPI API.
///
//===----------------------------------------------------------------------===//

#include "MPIChecker.h"
#include "../ClangSACheckers.h"

namespace clang {
namespace ento {
namespace mpi {

void MPIChecker::checkDoubleNonblocking(const CallEvent &PreCallEvent,
                                        CheckerContext &Ctx) const {
  if (!FuncClassifier->isNonBlockingType(PreCallEvent.getCalleeIdentifier())) {
    return;
  }
  const MemRegion *const MR =
      PreCallEvent.getArgSVal(PreCallEvent.getNumArgs() - 1).getAsRegion();
  if (!MR)
    return;
  const ElementRegion *const ER = dyn_cast<ElementRegion>(MR);

  // The region must be typed, in order to reason about it.
  if (!isa<TypedRegion>(MR) || (ER && !isa<TypedRegion>(ER->getSuperRegion())))
    return;

  ProgramStateRef State = Ctx.getState();
  const Request *const Req = State->get<RequestMap>(MR);

  // double nonblocking detected
  if (Req && Req->CurrentState == Request::State::Nonblocking) {
    ExplodedNode *ErrorNode = Ctx.generateNonFatalErrorNode();
    BReporter.reportDoubleNonblocking(PreCallEvent, *Req, MR, ErrorNode,
                                      Ctx.getBugReporter());
    Ctx.addTransition(ErrorNode->getState(), ErrorNode);
  }
  // no error
  else {
    State = State->set<RequestMap>(MR, Request::State::Nonblocking);
    Ctx.addTransition(State);
  }
}

void MPIChecker::checkDoubleClose(const CallEvent &PreCallEvent,
                                  CheckerContext &Ctx) const {
  // Get the CallEvent PreCallEvent(Path-sensitive callback) and check if there is Pointer to a
  // valid Ident Info returned by isMPI_File_close
  if (!FuncClassifier->isMPI_File_close(PreCallEvent.getCalleeIdentifier())) {
    return;
  }
  // load the file on position NumArgs - 1 into MemRegion
  const MemRegion *const MR =
      PreCallEvent.getArgSVal(PreCallEvent.getNumArgs() - 1).getAsRegion();
  if(!MR)
      return;
  const ElementRegion *const ER = dyn_cast<ElementRegion>(MR);
  // something wrong with the type?
  if(!isa<TypedRegion>(MR) || (ER && !isa<TypedRegion>(ER->getSuperRegion())))
      return;
  /* get actual Ctx (CheckerContext structure) as programstate
    this will have information for the begugging message and many more
    ProgramState holds complete information on a momentary state of the program under analysis 
    New State needs to be created, because the original ones are immutable*/
  ProgramStateRef State = Ctx.getState();
  const MPIFile *const Fh = State->get<MPIFileMap>(MR);

  // create ErrorNode
  // if FH and Fh is already stored in the MPIFileMap(MR)
  if(Fh && Fh->CurrentState == MPIFile::State::Close) {
    // Ctx.generateNonFatalErrorNode()
    // This node will not be a sink. That is, exploration will continue along this path.
    ExplodedNode *ErrorNode = Ctx.generateNonFatalErrorNode();
    BReporter.reportDoubleClose(PreCallEvent, *Fh, MR, ErrorNode,
                                Ctx.getBugReporter());
    Ctx.addTransition(ErrorNode->getState(), ErrorNode);
  }
  // no error keep on analyse
  else {
    // obtain a new state with a modified trait value
    // trait was defined in MPITypes_2.h
    // through set a new state is obtained, but with modified trait value
    State = State->set<MPIFileMap>(MR, MPIFile::State::Close);
    // add new modified State
    Ctx.addTransition(State);
  }
}
void MPIChecker::checkFileLeak(SymbolReaper &SymReaper,
                                           CheckerContext &Ctx) const {
  if (SymReaper.hasDeadSymbols())
    return;
  ProgramStateRef State = Ctx.getState();
  const auto &MPIFiles = State->get<MPIFileMap>();
  if(MPIFiles.isEmpty())
      return;

  static CheckerProgramPointTag Tag("MPI-Checker", "File Leak");
  ExplodedNode *ErrorNode{nullptr};

  auto FileMap = State->get<MPIFileMap>();
  for (const auto &Fh : FileMap) {
    // first one is alive - this is the open
    if (!SymReaper.isLiveRegion(Fh.first))
      // second one is the not detected close
      if (Fh.second.CurrentState == MPIFile::State::Open) {
        if (!ErrorNode) {
          ErrorNode = Ctx.generateNonFatalErrorNode(State, &Tag);
          State = ErrorNode->getState();
        }
        BReporter.reportFileLeak(Fh.second, Fh.first, ErrorNode,
                                 Ctx.getBugReporter());
      }
    State = State->remove<MPIFileMap>(Fh.first);
    // Report not close detected after open
  }

}
void MPIChecker::checkFileOpen(const CallEvent &PreCallEvent,
                               CheckerContext &Ctx) const {
  // if the Func Call is File_Open
  if (!FuncClassifier->isMPI_File_open(PreCallEvent.getCalleeIdentifier())) {
    return;
  }
  const MemRegion *const MR =
      PreCallEvent.getArgSVal(PreCallEvent.getNumArgs() - 1).getAsRegion();
  if (!MR)
    return;
  const ElementRegion *const ER = dyn_cast<ElementRegion>(MR);
  // something wrong with the type?
  if (!isa<TypedRegion>(MR) || (ER && !isa<TypedRegion>(ER->getSuperRegion())))
    return;
  // capture the next transition where the State is open
  ProgramStateRef State = Ctx.getState();
  const MPIFile *const Fh = State->get<MPIFileMap>(MR);
  State = State->set<MPIFileMap>(MR, MPIFile::State::Open);
      printf("File is open\n");

  // just to check the functionality
  ExplodedNode *ErrorNode = Ctx.generateNonFatalErrorNode();
    BReporter.reportOpen(PreCallEvent, *Fh, MR, ErrorNode,
                                      Ctx.getBugReporter());
    Ctx.addTransition(ErrorNode->getState(), ErrorNode);
}

void MPIChecker::checkUnmatchedWaits(const CallEvent &PreCallEvent,
                                     CheckerContext &Ctx) const {
  if (!FuncClassifier->isWaitType(PreCallEvent.getCalleeIdentifier()))
    return;
  const MemRegion *const MR = topRegionUsedByWait(PreCallEvent);
  if (!MR)
    return;
  const ElementRegion *const ER = dyn_cast<ElementRegion>(MR);

  // The region must be typed, in order to reason about it.
  if (!isa<TypedRegion>(MR) || (ER && !isa<TypedRegion>(ER->getSuperRegion())))
    return;

  llvm::SmallVector<const MemRegion *, 2> ReqRegions;
  allRegionsUsedByWait(ReqRegions, MR, PreCallEvent, Ctx);
  if (ReqRegions.empty())
    return;

  ProgramStateRef State = Ctx.getState();
  static CheckerProgramPointTag Tag("MPI-Checker", "UnmatchedWait");
  ExplodedNode *ErrorNode{nullptr};

  // Check all request regions used by the wait function.
  for (const auto &ReqRegion : ReqRegions) {
    const Request *const Req = State->get<RequestMap>(ReqRegion);
    State = State->set<RequestMap>(ReqRegion, Request::State::Wait);
    if (!Req) {
      if (!ErrorNode) {
        ErrorNode = Ctx.generateNonFatalErrorNode(State, &Tag);
        State = ErrorNode->getState();
      }
      // A wait has no matching nonblocking call.
      BReporter.reportUnmatchedWait(PreCallEvent, ReqRegion, ErrorNode,
                                    Ctx.getBugReporter());
    }
  }

  if (!ErrorNode) {
    Ctx.addTransition(State);
  } else {
    Ctx.addTransition(State, ErrorNode);
  }
}

void MPIChecker::checkMissingWaits(SymbolReaper &SymReaper,
                                   CheckerContext &Ctx) const {
  if (!SymReaper.hasDeadSymbols())
    return;

  ProgramStateRef State = Ctx.getState();
  const auto &Requests = State->get<RequestMap>();
  if (Requests.isEmpty())
    return;

  static CheckerProgramPointTag Tag("MPI-Checker", "MissingWait");
  ExplodedNode *ErrorNode{nullptr};

  auto ReqMap = State->get<RequestMap>();
  for (const auto &Req : ReqMap) {
    if (!SymReaper.isLiveRegion(Req.first)) {
      if (Req.second.CurrentState == Request::State::Nonblocking) {

        if (!ErrorNode) {
          ErrorNode = Ctx.generateNonFatalErrorNode(State, &Tag);
          State = ErrorNode->getState();
        }
        BReporter.reportMissingWait(Req.second, Req.first, ErrorNode,
                                    Ctx.getBugReporter());
      }
      State = State->remove<RequestMap>(Req.first);
    }
  }

  // Transition to update the state regarding removed requests.
  if (!ErrorNode) {
    Ctx.addTransition(State);
  } else {
    Ctx.addTransition(State, ErrorNode);
  }
}

const MemRegion *MPIChecker::topRegionUsedByWait(const CallEvent &CE) const {

  if (FuncClassifier->isMPI_Wait(CE.getCalleeIdentifier())) {
    return CE.getArgSVal(0).getAsRegion();
  } else if (FuncClassifier->isMPI_Waitall(CE.getCalleeIdentifier())) {
    return CE.getArgSVal(1).getAsRegion();
  } else {
    return (const MemRegion *)nullptr;
  }
}

void MPIChecker::allRegionsUsedByWait(
    llvm::SmallVector<const MemRegion *, 2> &ReqRegions,
    const MemRegion *const MR, const CallEvent &CE, CheckerContext &Ctx) const {

  MemRegionManager *const RegionManager = MR->getMemRegionManager();

  if (FuncClassifier->isMPI_Waitall(CE.getCalleeIdentifier())) {
    const MemRegion *SuperRegion{nullptr};
    if (const ElementRegion *const ER = MR->getAs<ElementRegion>()) {
      SuperRegion = ER->getSuperRegion();
    }

    // A single request is passed to MPI_Waitall.
    if (!SuperRegion) {
      ReqRegions.push_back(MR);
      return;
    }

    const auto &Size = Ctx.getStoreManager().getSizeInElements(
        Ctx.getState(), SuperRegion,
        CE.getArgExpr(1)->getType()->getPointeeType());
    const llvm::APSInt &ArrSize = Size.getAs<nonloc::ConcreteInt>()->getValue();

    for (size_t i = 0; i < ArrSize; ++i) {
      const NonLoc Idx = Ctx.getSValBuilder().makeArrayIndex(i);

      const ElementRegion *const ER = RegionManager->getElementRegion(
          CE.getArgExpr(1)->getType()->getPointeeType(), Idx, SuperRegion,
          Ctx.getASTContext());

      ReqRegions.push_back(ER->getAs<MemRegion>());
    }
  } else if (FuncClassifier->isMPI_Wait(CE.getCalleeIdentifier())) {
    ReqRegions.push_back(MR);
  }
}

} // end of namespace: mpi
} // end of namespace: ento
} // end of namespace: clang

// Registers the checker for static analysis.
void clang::ento::registerMPIChecker(CheckerManager &MGR) {
  MGR.registerChecker<clang::ento::mpi::MPIChecker>();
}
