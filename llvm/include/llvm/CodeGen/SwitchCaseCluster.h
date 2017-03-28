//=======-- SwitchCaseCluster.h - Form case clusters from SwitchInst --=======//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This implements routines for forming case clusters.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CODEGEN_SWITCHCASECLUSTER_H
#define LLVM_CODEGEN_SWITCHCASECLUSTER_H

#include "llvm/Target/TargetLowering.h"
#include <vector>

namespace llvm {

class SelectionDAGBuilder;

enum CaseClusterKind {
  /// A cluster of adjacent case labels with the same destination, or just one
  /// case.
  CC_Range,
  /// A cluster of cases suitable for jump table lowering.
  CC_JumpTable,
  /// A cluster of cases suitable for bit test lowering.
  CC_BitTests
};

typedef std::vector<unsigned> CaseVector;

/// A cluster of case labels.
class CaseCluster {
public:
  CaseClusterKind Kind;
  const ConstantInt *Low, *High;
  CaseVector Cases;

  // Return a read-only case iterator indexed by I in this cluster.
  SwitchInst::ConstCaseIt getCase(const SwitchInst *SI, unsigned I) const {
    assert(Cases.size() > 0 && I < Cases.size());
    return SwitchInst::ConstCaseIt(SI, Cases[I]);
  }

  /// Return the number of cases in this cluster.
  unsigned getNumerOfCases() const { return Cases.size(); }

  /// Return case value indexed by I in this cluster.
  const ConstantInt *getCaseValueAt(const SwitchInst *SI, unsigned I) const {
    return getCase(SI, I).getCaseValue();
  }

  /// Return successor for the case indexed by I in this cluster.
  const BasicBlock *getCaseSuccessorAt(const SwitchInst *SI, unsigned I) const {
    return getCase(SI, I).getCaseSuccessor();
  }

  /// Return a single successor for the case indexed by I in this cluster.
  const BasicBlock *getSingleSuccessor(const SwitchInst *SI) const {
    assert(Kind == CC_Range && "Expected to be used only by CC_Range clusters");
    const BasicBlock *BB = getCase(SI, 0).getCaseSuccessor();
#ifndef NDEBUG
    for (unsigned I = 1, E = Cases.size(); I != E; ++I)
      assert(getCaseSuccessorAt(SI, I) == BB && "Invalid case cluster");
#endif
    return BB;
  }

  static CaseCluster range(const ConstantInt *Low, const ConstantInt *High,
                           unsigned Index) {
    CaseCluster C;
    C.Kind = CC_Range;
    C.Low = Low;
    C.High = High;
    C.Cases.push_back(Index);
    return C;
  }

  static CaseCluster jumpTable(const ConstantInt *Low,
                               const ConstantInt *High) {
    CaseCluster C;
    C.Kind = CC_JumpTable;
    C.Low = Low;
    C.High = High;
    return C;
  }

  static CaseCluster bitTests(const ConstantInt *Low, const ConstantInt *High) {
    CaseCluster C;
    C.Kind = CC_BitTests;
    C.Low = Low;
    C.High = High;
    return C;
  }
};

typedef std::vector<CaseCluster> CaseClusterVector;
typedef CaseClusterVector::iterator CaseClusterIt;

class TargetLowering;

class SwitchCaseClusterFinder {
public:
  const DataLayout &DL;
  const TargetLowering &TLI;
  const CodeGenOpt::Level OptLevel;

  SwitchCaseClusterFinder(const DataLayout &DL, const TargetLowering &TLI,
                          const CodeGenOpt::Level OptLevel)
      : DL(DL), TLI(TLI), OptLevel(OptLevel) {}

  /// Check whether the range [Low,High] fits in a machine word.
  static bool rangeFitsInWord(const APInt &Low, const APInt &High,
                              const DataLayout &DL) {
    // FIXME: Using the pointer type doesn't seem ideal.
    uint64_t BW = DL.getPointerSizeInBits();
    uint64_t Range = (High - Low).getLimitedValue(UINT64_MAX - 1) + 1;
    return Range <= BW;
  }

  /// Calculate clusters for cases in SI and store them in Clusters.
  const BasicBlock *findClusters(const SwitchInst &SI,
                                 CaseClusterVector &Clusters);

private:
  /// Extract cases from the switch and build inital form of case clusters.
  /// Return a default block.
  const BasicBlock *formInitalCaseClusers(const SwitchInst &SI,
                                          CaseClusterVector &Clusters);

  /// Find clusters of cases suitable for jump table lowering.
  void findJumpTables(CaseClusterVector &Clusters, const SwitchInst *SI);

  /// Find clusters of cases suitable for bit test lowering.
  void findBitTestClusters(CaseClusterVector &Clusters, const SwitchInst *SI);

  // Replace an unreachable default with the most popular destination.
  const BasicBlock *replaceUnrechableDefault(const SwitchInst &SI,
                                             CaseClusterVector &Clusters);

  /// Check whether these clusters are suitable for lowering with bit tests
  /// based on the number of destinations, comparison metric, and range.
  bool isSuitableForBitTests(unsigned NumDests, unsigned NumCmps,
                             const APInt &Low, const APInt &High);

  /// Return true if it can build a bit test cluster from Clusters[First..Last].
  // bool canBuildJumpTable(const CaseClusterVector &Clusters, unsigned First,
  bool canBuildJumpTable(const CaseClusterVector &Clusters, unsigned First,
                         unsigned Last, const SwitchInst *SI,
                         CaseCluster &JTCluster);

  /// Returns true if it can build a jump table cluster from
  /// Clusters[First..Last].
  bool canBuildBitTest(CaseClusterVector &Clusters, unsigned First,
                       unsigned Last, const SwitchInst *SI,
                       CaseCluster &BTCluster);

  /// Check whether a range of clusters is dense enough for a jump table.
  bool isDense(const CaseClusterVector &Clusters,
               const SmallVectorImpl<unsigned> &TotalCases, unsigned First,
               unsigned Last, unsigned MinDensity) const;

  /// Sort Clusters and merge adjacent cases.
  void sortAndRangeify(const SwitchInst *SI, CaseClusterVector &Clusters);

  /// Build a jump table cluster from Clusters[First..Last].
  void buildJumpTable(const CaseClusterVector &Clusters, unsigned First,
                      unsigned Last, const SwitchInst *SI,
                      CaseCluster &JTCluster);

  /// Build a bit test cluster from Clusters[First..Last].
  void buildBitTests(CaseClusterVector &Clusters, unsigned First, unsigned Last,
                     const SwitchInst *SI, CaseCluster &BTCluster);
};
} // end namespace llvm
#endif
