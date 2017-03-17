//===-- SwitchLoweringCaseCluster.h - Form case clusters from SwitchInst --===//
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

#ifndef LLVM_CODEGEN_SWITCHLOWERINGCASECLUSTER_H
#define LLVM_CODEGEN_SWITCHLOWERINGCASECLUSTER_H

#include "llvm/CodeGen/FunctionLoweringInfo.h"
#include "llvm/CodeGen/MachineJumpTableInfo.h"
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

/// A cluster of case labels.
struct CaseCluster {
  CaseClusterKind Kind;
  const ConstantInt *Low, *High;
  const BasicBlock *BB;
  union {
    MachineBasicBlock *MBB;
    unsigned JTCasesIndex;
    unsigned BTCasesIndex;
  };
  BranchProbability Prob;

  static CaseCluster range(const ConstantInt *Low, const ConstantInt *High,
                           const BasicBlock *BB, BranchProbability Prob) {
    CaseCluster C;
    C.Kind = CC_Range;
    C.Low = Low;
    C.High = High;
    C.BB = BB;
    C.Prob = Prob;
    return C;
  }

  static CaseCluster jumpTable(const ConstantInt *Low, const ConstantInt *High,
                               unsigned JTCasesIndex, BranchProbability Prob) {
    CaseCluster C;
    C.Kind = CC_JumpTable;
    C.Low = Low;
    C.High = High;
    C.JTCasesIndex = JTCasesIndex;
    C.Prob = Prob;
    return C;
  }

  static CaseCluster bitTests(const ConstantInt *Low, const ConstantInt *High,
                              unsigned BTCasesIndex, BranchProbability Prob) {
    CaseCluster C;
    C.Kind = CC_BitTests;
    C.Low = Low;
    C.High = High;
    C.BTCasesIndex = BTCasesIndex;
    C.Prob = Prob;
    return C;
  }
};

typedef std::vector<CaseCluster> CaseClusterVector;
typedef CaseClusterVector::iterator CaseClusterIt;

struct CaseBits {
  uint64_t Mask;
  MachineBasicBlock *BB;
  unsigned Bits;
  BranchProbability ExtraProb;

  CaseBits(uint64_t mask, MachineBasicBlock *bb, unsigned bits,
           BranchProbability Prob)
      : Mask(mask), BB(bb), Bits(bits), ExtraProb(Prob) {}

  CaseBits() : Mask(0), BB(nullptr), Bits(0) {}
};

typedef std::vector<CaseBits> CaseBitsVector;

struct JumpTableCase {
  JumpTableCase(unsigned R, unsigned J, MachineBasicBlock *M,
                MachineBasicBlock *D)
      : Reg(R), JTI(J), MBB(M), Default(D) {

    assert(MBB && "how MBB is null in JumpTable");
  }

  /// Reg - the virtual register containing the index of the jump table entry
  /// to jump to.
  unsigned Reg;
  /// JTI - the JumpTableIndex for this jump table in the function.
  unsigned JTI;
  /// MBB - the MBB into which to emit the code for the indirect jump.
  MachineBasicBlock *MBB;
  /// Default - the MBB of the default bb, which is a successor of the range
  /// check MBB.  This is when updating PHI nodes in successors.
  MachineBasicBlock *Default;
};
struct JumpTableHeader {
  JumpTableHeader(APInt F, APInt L, const Value *SV, MachineBasicBlock *H,
                  bool E = false)
      : First(std::move(F)), Last(std::move(L)), SValue(SV), HeaderBB(H),
        Emitted(E) {}
  APInt First;
  APInt Last;
  const Value *SValue;
  MachineBasicBlock *HeaderBB;
  bool Emitted;
};
typedef std::pair<JumpTableHeader, JumpTableCase> JumpTableBlock;

struct BitTestCase {
  BitTestCase(uint64_t M, MachineBasicBlock *T, MachineBasicBlock *Tr,
              BranchProbability Prob)
      : Mask(M), ThisBB(T), TargetBB(Tr), ExtraProb(Prob) {}
  uint64_t Mask;
  MachineBasicBlock *ThisBB;
  MachineBasicBlock *TargetBB;
  BranchProbability ExtraProb;
};

typedef SmallVector<BitTestCase, 3> BitTestInfo;

struct BitTestBlock {
  BitTestBlock(APInt F, APInt R, const Value *SV, unsigned Rg, MVT RgVT, bool E,
               bool CR, MachineBasicBlock *P, MachineBasicBlock *D,
               BitTestInfo C, BranchProbability Pr)
      : First(std::move(F)), Range(std::move(R)), SValue(SV), Reg(Rg),
        RegVT(RgVT), Emitted(E), ContiguousRange(CR), Parent(P), Default(D),
        Cases(std::move(C)), Prob(Pr) {}
  APInt First;
  APInt Range;
  const Value *SValue;
  unsigned Reg;
  MVT RegVT;
  bool Emitted;
  bool ContiguousRange;
  MachineBasicBlock *Parent;
  MachineBasicBlock *Default;
  BitTestInfo Cases;
  BranchProbability Prob;
  BranchProbability DefaultProb;
};

class SwitchLoweringCaseClusterBuilder {
public:
  const DataLayout &DL;
  const TargetLowering &TLI;
  const CodeGenOpt::Level OptLevel;

  SwitchLoweringCaseClusterBuilder(const DataLayout &DL,
                                   const TargetLowering &TLI,
                                   const CodeGenOpt::Level OptLevel)
      : DL(DL), TLI(TLI), OptLevel(OptLevel) {}

  virtual ~SwitchLoweringCaseClusterBuilder() {}

  /// Find clusters of cases suitable for jump table lowering.
  void findJumpTables(CaseClusterVector &Clusters, const SwitchInst *SI,
                      const BasicBlock *DefaultBB);

  /// Find clusters of cases suitable for bit test lowering.
  void findBitTestClusters(CaseClusterVector &Clusters, const SwitchInst *SI);

  /// Extract cases from the switch and build inital form of case clusters.
  void formInitalCaseCluser(const SwitchInst &SI, CaseClusterVector &Clusters,
                            BranchProbabilityInfo *BPI);

  // Replace an unreachable default with the most popular destination.
  const BasicBlock *replaceUnrechableDefault(const SwitchInst &SI,
                                             CaseClusterVector &Clusters);

  /// Check whether the range [Low,High] fits in a machine word.
  bool rangeFitsInWord(const APInt &Low, const APInt &High);

private:
  /// Check whether these clusters are suitable for lowering with bit tests
  /// based on the number of destinations, comparison metric, and range.
  bool isSuitableForBitTests(unsigned NumDests, unsigned NumCmps,
                             const APInt &Low, const APInt &High);

  /// Return true if it can build a bit test cluster from Clusters[First..Last].
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
  void sortAndRangeify(CaseClusterVector &Clusters);

  /// Build a jump table cluster from Clusters[First..Last].
  virtual void buildJumpTable(const CaseClusterVector &Clusters, unsigned First,
                              unsigned Last, const SwitchInst *SI,
                              const BasicBlock *DefaultBB,
                              CaseCluster &JTCluster);

  /// Build a bit test cluster from Clusters[First..Last].
  virtual void buildBitTests(CaseClusterVector &Clusters, unsigned First,
                             unsigned Last, const SwitchInst *SI,
                             CaseCluster &BTCluster);
};

class SwitchLoweringCaseClusterBuilderForDAG
    : public SwitchLoweringCaseClusterBuilder {
  FunctionLoweringInfo &FuncInfo;
  SelectionDAGBuilder *SDB;

public:
  SwitchLoweringCaseClusterBuilderForDAG(const DataLayout &DL,
                                         const TargetLowering &TLI,
                                         const CodeGenOpt::Level OptLevel,
                                         FunctionLoweringInfo &FuncInfo,
                                         SelectionDAGBuilder *SDB)
      : SwitchLoweringCaseClusterBuilder(DL, TLI, OptLevel), FuncInfo(FuncInfo),
        SDB(SDB) {}

  ~SwitchLoweringCaseClusterBuilderForDAG() {}

private:
  void buildJumpTable(const CaseClusterVector &Clusters, unsigned First,
                      unsigned Last, const SwitchInst *SI,
                      const BasicBlock *DefaultBB,
                      CaseCluster &JTCluster) override;

  void buildBitTests(CaseClusterVector &Clusters, unsigned First, unsigned Last,
                     const SwitchInst *SI, CaseCluster &BTCluster) override;
};

class SwitchLoweringCaseCluster {
  SwitchLoweringCaseClusterBuilder *ClusterBuilder;

public:
  SwitchLoweringCaseCluster(const DataLayout &DL, const TargetLowering &TLI,
                            const CodeGenOpt::Level OptLevel,
                            FunctionLoweringInfo &FuncInfo,
                            SelectionDAGBuilder *SDB) {
    ClusterBuilder = new SwitchLoweringCaseClusterBuilderForDAG(
        DL, TLI, OptLevel, FuncInfo, SDB);
  }

  SwitchLoweringCaseCluster(const DataLayout &DL, const TargetLowering &TLI,
                            const CodeGenOpt::Level OptLevel) {
    ClusterBuilder = new SwitchLoweringCaseClusterBuilder(DL, TLI, OptLevel);
  }

  ~SwitchLoweringCaseCluster() { delete ClusterBuilder; }

  const BasicBlock *findCaseClusters(const SwitchInst &SI,
                                     CaseClusterVector &Clusters,
                                     BranchProbabilityInfo *BPI);
};

} // end namespace llvm

#endif
