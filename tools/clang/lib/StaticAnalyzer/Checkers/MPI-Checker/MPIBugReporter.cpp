//===-- MPIBugReporter.cpp - bug reporter -----------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file defines prefabricated reports which are emitted in
/// case of MPI related bugs, detected by path-sensitive analysis.
///
//===----------------------------------------------------------------------===//

#include "MPIBugReporter.h"
#include "MPIChecker.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CallEvent.h"

namespace clang {
namespace ento {
namespace mpi {

void MPIBugReporter::reportDoubleNonblocking(
    const CallEvent &MPICallEvent, const ento::mpi::Request &Req,
    const MemRegion *const RequestRegion,
    const ExplodedNode *const ExplNode,
    BugReporter &BReporter) const {

  std::string ErrorText;
  ErrorText = "Double nonblocking on request " +
              RequestRegion->getDescriptiveName() + ". ";

  auto Report = llvm::make_unique<BugReport>(*DoubleNonblockingBugType,
                                             ErrorText, ExplNode);

  Report->addRange(MPICallEvent.getSourceRange());
  SourceRange Range = RequestRegion->sourceRange();

  if (Range.isValid())
    Report->addRange(Range);

  Report->addVisitor(llvm::make_unique<RequestNodeVisitor>(
      RequestRegion, "Request is previously used by nonblocking call here. "));
  Report->markInteresting(RequestRegion);

  BReporter.emitReport(std::move(Report));
}

void MPIBugReporter::reportDoubleOpen(const CallEvent &MPICallEvent,
                                const ento::mpi::MPIFile &Fh,
                                const MemRegion *const MPIFileRegion,
                                const ExplodedNode *const ExplNode,
                                BugReporter &BReporter) const {
  std::string ErrorText;
  ErrorText = "Double open on file " +
              MPIFileRegion->getDescriptiveName() + ". ";
  auto Report =
      llvm::make_unique<BugReport>(*DoubleOpenBugType, ErrorText, ExplNode);
  Report->addRange(MPICallEvent.getSourceRange());
  SourceRange Range = MPIFileRegion->sourceRange();

  if (Range.isValid())
    Report->addRange(Range);

  Report->addVisitor(llvm::make_unique<MPIFileNodeVisitor>(
      MPIFileRegion, "File is previously opened here. "));
  Report->markInteresting(MPIFileRegion);
  BReporter.emitReport(std::move(Report));
}

void MPIBugReporter::reportDoubleClose(const CallEvent &MPICallEvent,
                                       const ento::mpi::MPIFile &Fh,
                                       const MemRegion *const MPIFileRegion,
                                       const ExplodedNode *const ExplNode,
                                       BugReporter &BReporter) const {
  std::string ErrorText;
  ErrorText =
      "Double close on file " + MPIFileRegion->getDescriptiveName() + ". ";

  auto Report =
      llvm::make_unique<BugReport>(*DoubleCloseBugType, ErrorText, ExplNode);

  Report->addRange(MPICallEvent.getSourceRange());
  SourceRange Range = MPIFileRegion->sourceRange();

  if (Range.isValid())
    Report->addRange(Range);

  Report->addVisitor(llvm::make_unique<MPIFileNodeVisitor>(
      MPIFileRegion, "File is previously closed here. "));
  Report->markInteresting(MPIFileRegion);

  BReporter.emitReport(std::move(Report));
}
void MPIBugReporter::reportFileLeak(const ento::mpi::MPIFile &Fh,
                                    const MemRegion *const MPIFileRegion,
                                    const ExplodedNode *const ExplNode,
                                    BugReporter &BReporter) const {
  std::string ErrorText;
  ErrorText = "File  " + MPIFileRegion->getDescriptiveName() +
              "has no matching close. ";

  auto Report =
      llvm::make_unique<BugReport>(*FileLeakBugType, ErrorText, ExplNode);

  SourceRange Range = MPIFileRegion->sourceRange();
  if (Range.isValid())
    Report->addRange(Range);

  Report->addVisitor(llvm::make_unique<MPIFileNodeVisitor>(
      MPIFileRegion, "File was previously opened here. "));
  Report->markInteresting(MPIFileRegion);

  BReporter.emitReport(std::move(Report));
}

void MPIBugReporter::reportMissingWait(
    const ento::mpi::Request &Req, const MemRegion *const RequestRegion,
    const ExplodedNode *const ExplNode,
    BugReporter &BReporter) const {
  std::string ErrorText{"Request " + RequestRegion->getDescriptiveName() +
                        " has no matching wait. "};

  auto Report =
      llvm::make_unique<BugReport>(*MissingWaitBugType, ErrorText, ExplNode);

  SourceRange Range = RequestRegion->sourceRange();
  if (Range.isValid())
    Report->addRange(Range);
  Report->addVisitor(llvm::make_unique<RequestNodeVisitor>(
      RequestRegion, "Request is previously used by nonblocking call here. "));
  Report->markInteresting(RequestRegion);

  BReporter.emitReport(std::move(Report));
}

void MPIBugReporter::reportUnmatchedWait(
    const CallEvent &CE, const clang::ento::MemRegion *const RequestRegion,
    const ExplodedNode *const ExplNode,
    BugReporter &BReporter) const {
  std::string ErrorText{"Request " + RequestRegion->getDescriptiveName() +
                        " has no matching nonblocking call. "};

  auto Report =
      llvm::make_unique<BugReport>(*UnmatchedWaitBugType, ErrorText, ExplNode);

  Report->addRange(CE.getSourceRange());
  SourceRange Range = RequestRegion->sourceRange();
  if (Range.isValid())
    Report->addRange(Range);

  BReporter.emitReport(std::move(Report));
}

std::shared_ptr<PathDiagnosticPiece>
MPIBugReporter::RequestNodeVisitor::VisitNode(const ExplodedNode *N,
                                              const ExplodedNode *PrevN,
                                              BugReporterContext &BRC,
                                              BugReport &BR) {

  if (IsNodeFound)
    return nullptr;

  const Request *const Req = N->getState()->get<RequestMap>(RequestRegion);
  const Request *const PrevReq =
      PrevN->getState()->get<RequestMap>(RequestRegion);

  // Check if request was previously unused or in a different state.
  if ((Req && !PrevReq) || (Req->CurrentState != PrevReq->CurrentState)) {
    IsNodeFound = true;

    ProgramPoint P = PrevN->getLocation();
    PathDiagnosticLocation L =
        PathDiagnosticLocation::create(P, BRC.getSourceManager());

    return std::make_shared<PathDiagnosticEventPiece>(L, ErrorText);
  }

  return nullptr;
}

std::shared_ptr<PathDiagnosticPiece>
MPIBugReporter::MPIFileNodeVisitor::VisitNode(const ExplodedNode *N,
                                              const ExplodedNode *PrevN,
                                              BugReporterContext &BRC,
                                              BugReport &BR) {

  if (IsNodeFound)
    return nullptr;

  const MPIFile *const Fh = N->getState()->get<MPIFileMap>(MPIFileRegion);
  const MPIFile *const PrevFh =
      PrevN->getState()->get<MPIFileMap>(MPIFileRegion);

  // check if the file was previously unused
  if ((Fh && !PrevFh) || (Fh->CurrentState != PrevFh->CurrentState)) {
    IsNodeFound = true;

    ProgramPoint P = PrevN->getLocation();
    PathDiagnosticLocation L =
        PathDiagnosticLocation::create(P, BRC.getSourceManager());

    return std::make_shared<PathDiagnosticEventPiece>(L, ErrorText);
  }

  return nullptr;
}

} // end of namespace: mpi
} // end of namespace: ento
} // end of namespace: clang
