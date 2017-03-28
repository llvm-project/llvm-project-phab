//===-- SwitchLoweringCaseCluster.cpp   -----------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This implements routines for forming case clusters for SwitchInst.
//
//===----------------------------------------------------------------------===//

#include "llvm/CodeGen/SwitchCaseCluster.h"
#include "llvm/Support/Debug.h"
#include <algorithm>
using namespace llvm;

/// Minimum jump table density for normal functions.
static cl::opt<unsigned>
    JumpTableDensity("jump-table-density", cl::init(10), cl::Hidden,
                     cl::desc("Minimum density for building a jump table in "
                              "a normal function"));

/// Minimum jump table density for -Os or -Oz functions.
static cl::opt<unsigned> OptsizeJumpTableDensity(
    "optsize-jump-table-density", cl::init(40), cl::Hidden,
    cl::desc("Minimum density for building a jump table in "
             "an optsize function"));

bool SwitchCaseClusterFinder::isDense(
    const CaseClusterVector &Clusters,
    const SmallVectorImpl<unsigned> &TotalCases, unsigned First, unsigned Last,
    unsigned Density) const {
  assert(Last >= First);
  assert(TotalCases[Last] >= TotalCases[First]);

  const APInt &LowCase = Clusters[First].Low->getValue();
  const APInt &HighCase = Clusters[Last].High->getValue();
  assert(LowCase.getBitWidth() == HighCase.getBitWidth());

  // FIXME: A range of consecutive cases has 100% density, but only requires one
  // comparison to lower. We should discriminate against such consecutive ranges
  // in jump tables.

  uint64_t Diff = (HighCase - LowCase).getLimitedValue((UINT64_MAX - 1) / 100);
  uint64_t Range = Diff + 1;

  uint64_t NumCases =
      TotalCases[Last] - (First == 0 ? 0 : TotalCases[First - 1]);

  assert(NumCases < UINT64_MAX / 100);
  assert(Range >= NumCases);

  return NumCases * 100 >= Range * Density;
}

static inline bool areJTsAllowed(const TargetLowering &TLI,
                                 const SwitchInst *SI) {
  const Function *Fn = SI->getParent()->getParent();
  if (Fn->getFnAttribute("no-jump-tables").getValueAsString() == "true")
    return false;

  return TLI.isOperationLegalOrCustom(ISD::BR_JT, MVT::Other) ||
         TLI.isOperationLegalOrCustom(ISD::BRIND, MVT::Other);
}

bool SwitchCaseClusterFinder::canBuildBitTest(CaseClusterVector &Clusters,
                                              unsigned First, unsigned Last,
                                              const SwitchInst *SI,
                                              CaseCluster &BTCluster) {
  assert(First <= Last);
  if (First == Last)
    return false;

  SmallPtrSet<const BasicBlock *, 4> Dests;
  unsigned NumCmps = 0;
  for (int64_t I = First; I <= Last; ++I) {
    assert(Clusters[I].Kind == CC_Range);
    Dests.insert(Clusters[I].getSingleSuccessor(SI));
    NumCmps += (Clusters[I].Low == Clusters[I].High) ? 1 : 2;
  }
  unsigned NumDests = Dests.size();

  APInt Low = Clusters[First].Low->getValue();
  APInt High = Clusters[Last].High->getValue();
  assert(Low.slt(High));

  return isSuitableForBitTests(NumDests, NumCmps, Low, High);
}

void SwitchCaseClusterFinder::sortAndRangeify(const SwitchInst *SI,
                                              CaseClusterVector &Clusters) {
#ifndef NDEBUG
  for (const CaseCluster &CC : Clusters)
    assert(CC.Low == CC.High && CC.Cases.size() == 1 &&
           "Input clusters must be single-case");
#endif
  std::sort(Clusters.begin(), Clusters.end(),
            [](const CaseCluster &a, const CaseCluster &b) {
              return a.Low->getValue().slt(b.Low->getValue());
            });
  // Merge adjacent clusters with the same destination.
  const unsigned N = Clusters.size();
  unsigned DstIndex = 0;
  for (unsigned SrcIndex = 0; SrcIndex < N; ++SrcIndex) {
    CaseCluster &CC = Clusters[SrcIndex];
    const ConstantInt *CaseVal = CC.Low;
    const BasicBlock *Succ = CC.getSingleSuccessor(SI);
    if (DstIndex != 0 &&
        Clusters[DstIndex - 1].getSingleSuccessor(SI) == Succ &&
        (CaseVal->getValue() - Clusters[DstIndex - 1].High->getValue()) == 1) {
      // If this case has the same successor and is a neighbour, merge it into
      // the previous cluster.
      Clusters[DstIndex - 1].High = CaseVal;
      Clusters[DstIndex - 1].Cases.push_back(Clusters[SrcIndex].Cases.back());
    } else
      Clusters[DstIndex++] = Clusters[SrcIndex];
  }
  Clusters.resize(DstIndex);
}

const BasicBlock *
SwitchCaseClusterFinder::replaceUnrechableDefault(const SwitchInst &SI,
                                                  CaseClusterVector &Clusters) {
  const BasicBlock *DefaultBB = SI.getDefaultDest();
  if (OptLevel != CodeGenOpt::None) {
    // FIXME: Exploit unreachable default more aggressively.
    bool UnreachableDefault =
        isa<UnreachableInst>(SI.getDefaultDest()->getFirstNonPHIOrDbg());
    if (UnreachableDefault && !Clusters.empty()) {
      DenseMap<const BasicBlock *, unsigned> Popularity;
      unsigned MaxPop = 0;
      const BasicBlock *MaxBB = nullptr;
      for (auto I : SI.cases()) {
        const BasicBlock *BB = I.getCaseSuccessor();
        if (++Popularity[BB] > MaxPop) {
          MaxPop = Popularity[BB];
          MaxBB = BB;
        }
      }
      // Set new default.
      assert(MaxPop > 0 && MaxBB);
      DefaultBB = MaxBB;

      // Remove cases that were pointing to the destination that is now the
      // default.
      CaseClusterVector New;
      New.reserve(Clusters.size());
      for (CaseCluster &CC : Clusters) {
        if (CC.getSingleSuccessor(&SI) != DefaultBB)
          New.push_back(CC);
      }
      Clusters = std::move(New);
    }
  }
  return DefaultBB;
}

const BasicBlock *
SwitchCaseClusterFinder::formInitalCaseClusers(const SwitchInst &SI,
                                               CaseClusterVector &Clusters) {
  Clusters.reserve(SI.getNumCases());
  for (auto I : SI.cases()) {
    const ConstantInt *CaseVal = I.getCaseValue();
    Clusters.push_back(CaseCluster::range(CaseVal, CaseVal, I.getCaseIndex()));
  }
  // Cluster adjacent cases with the same destination. We do this at all
  // optimization levels because it's cheap to do and will make codegen faster
  // if there are many clusters.
  sortAndRangeify(&SI, Clusters);
  return replaceUnrechableDefault(SI, Clusters);
}

bool SwitchCaseClusterFinder::isSuitableForBitTests(unsigned NumDests,
                                                    unsigned NumCmps,
                                                    const APInt &Low,
                                                    const APInt &High) {
  // FIXME: I don't think NumCmps is the correct metric: a single case and a
  // range of cases both require only one branch to lower. Just looking at the
  // number of clusters and destinations should be enough to decide whether to
  // build bit tests.

  // To lower a range with bit tests, the range must fit the bitwidth of a
  // machine word.
  if (!rangeFitsInWord(Low, High, DL))
    return false;

  // Decide whether it's profitable to lower this range with bit tests. Each
  // destination requires a bit test and branch, and there is an overall range
  // check branch. For a small number of clusters, separate comparisons might be
  // cheaper, and for many destinations, splitting the range might be better.
  return (NumDests == 1 && NumCmps >= 3) || (NumDests == 2 && NumCmps >= 5) ||
         (NumDests == 3 && NumCmps >= 6);
}

bool SwitchCaseClusterFinder::canBuildJumpTable(
    const CaseClusterVector &Clusters, unsigned First, unsigned Last,
    const SwitchInst *SI, CaseCluster &JTCluster) {
  assert(First <= Last);
  unsigned NumCmps = 0;
  SmallPtrSet<const BasicBlock *, 4> JTProbs;
  for (unsigned I = First; I <= Last; ++I) {
    assert(Clusters[I].Kind == CC_Range);
    const APInt &Low = Clusters[I].Low->getValue();
    const APInt &High = Clusters[I].High->getValue();
    NumCmps += (Low == High) ? 1 : 2;
    JTProbs.insert(Clusters[I].getSingleSuccessor(SI));
  }
  return !(isSuitableForBitTests(JTProbs.size(), NumCmps,
                                 Clusters[First].Low->getValue(),
                                 Clusters[Last].High->getValue()));
}

void SwitchCaseClusterFinder::findJumpTables(CaseClusterVector &Clusters,
                                             const SwitchInst *SI) {

#ifndef NDEBUG
  // Clusters must be non-empty, sorted, and only contain Range clusters.
  assert(!Clusters.empty());
  for (CaseCluster &C : Clusters)
    assert(C.Kind == CC_Range);
  for (unsigned i = 1, e = Clusters.size(); i < e; ++i) {
    assert(Clusters[i - 1].High->getValue().slt(Clusters[i].Low->getValue()));
    // Case values in a cluster must be sorted.
    for (unsigned I = 1, E = Clusters[i].getNumerOfCases(); I != E; ++I) {
      const APInt &PreValue = Clusters[i].getCaseValueAt(SI, I - 1)->getValue();
      const APInt &CurValue = Clusters[i].getCaseValueAt(SI, I)->getValue();
      assert(PreValue.slt(CurValue));
    }
  }
#endif

  if (!areJTsAllowed(TLI, SI))
    return;

  const bool OptForSize = SI->getParent()->getParent()->optForSize();
  const int64_t N = Clusters.size();
  const unsigned MinJumpTableEntries = TLI.getMinimumJumpTableEntries();
  const unsigned SmallNumberOfEntries = MinJumpTableEntries / 2;
  const unsigned MaxJumpTableSize =
      OptForSize || TLI.getMaximumJumpTableSize() == 0
          ? UINT_MAX
          : TLI.getMaximumJumpTableSize();

  if (N < 2 || N < MinJumpTableEntries)
    return;

  // TotalCases[i]: Total nbr of cases in Clusters[0..i].
  SmallVector<unsigned, 8> TotalCases(N);
  for (unsigned i = 0; i < N; ++i) {
    TotalCases[i] = Clusters[i].getNumerOfCases();
    if (i != 0)
      TotalCases[i] += TotalCases[i - 1];
  }

  const unsigned MinDensity =
      OptForSize ? OptsizeJumpTableDensity : JumpTableDensity;

  // Cheap case: the whole range may be suitable for jump table.
  unsigned JumpTableSize =
      (Clusters[N - 1].High->getValue() - Clusters[0].Low->getValue())
          .getLimitedValue(UINT_MAX - 1) +
      1;

  if (JumpTableSize <= MaxJumpTableSize &&
      isDense(Clusters, TotalCases, 0, N - 1, MinDensity)) {
    CaseCluster JTCluster;
    if (canBuildJumpTable(Clusters, 0, N - 1, SI, JTCluster)) {
      buildJumpTable(Clusters, 0, N - 1, SI, JTCluster);
      Clusters[0] = JTCluster;
      Clusters.resize(1);
      return;
    }
  }

  // The algorithm below is not suitable for -O0.
  if (OptLevel == CodeGenOpt::None)
    return;

  // Split Clusters into minimum number of dense partitions. The algorithm uses
  // the same idea as Kannan & Proebsting "Correction to 'Producing Good Code
  // for the Case Statement'" (1994), but builds the MinPartitions array in
  // reverse order to make it easier to reconstruct the partitions in ascending
  // order. In the choice between two optimal partitionings, it picks the one
  // which yields more jump tables.

  // MinPartitions[i] is the minimum nbr of partitions of Clusters[i..N-1].
  SmallVector<unsigned, 8> MinPartitions(N);
  // LastElement[i] is the last element of the partition starting at i.
  SmallVector<unsigned, 8> LastElement(N);
  // PartitionsScore[i] is used to break ties when choosing between two
  // partitionings resulting in the same number of partitions.
  SmallVector<unsigned, 8> PartitionsScore(N);
  // For PartitionsScore, a small number of comparisons is considered as good as
  // a jump table and a single comparison is considered better than a jump
  // table.
  enum PartitionScores : unsigned {
    NoTable = 0,
    Table = 1,
    FewCases = 1,
    SingleCase = 2
  };

  // Base case: There is only one way to partition Clusters[N-1].
  MinPartitions[N - 1] = 1;
  LastElement[N - 1] = N - 1;
  PartitionsScore[N - 1] = PartitionScores::SingleCase;

  // Note: loop indexes are signed to avoid underflow.
  for (int64_t i = N - 2; i >= 0; i--) {
    // Find optimal partitioning of Clusters[i..N-1].
    // Baseline: Put Clusters[i] into a partition on its own.
    MinPartitions[i] = MinPartitions[i + 1] + 1;
    LastElement[i] = i;
    PartitionsScore[i] = PartitionsScore[i + 1] + PartitionScores::SingleCase;

    // Search for a solution that results in fewer partitions.
    for (int64_t j = N - 1; j > i; j--) {
      // Try building a partition from Clusters[i..j].
      JumpTableSize =
          (Clusters[j].High->getValue() - Clusters[i].Low->getValue())
              .getLimitedValue(UINT_MAX - 1) +
          1;
      if (JumpTableSize <= MaxJumpTableSize &&
          isDense(Clusters, TotalCases, i, j, MinDensity)) {
        unsigned NumPartitions = 1 + (j == N - 1 ? 0 : MinPartitions[j + 1]);
        unsigned Score = j == N - 1 ? 0 : PartitionsScore[j + 1];
        int64_t NumEntries = j - i + 1;

        if (NumEntries == 1)
          Score += PartitionScores::SingleCase;
        else if (NumEntries <= SmallNumberOfEntries)
          Score += PartitionScores::FewCases;
        else if (NumEntries >= MinJumpTableEntries)
          Score += PartitionScores::Table;

        // If this leads to fewer partitions, or to the same number of
        // partitions with better score, it is a better partitioning.
        if (NumPartitions < MinPartitions[i] ||
            (NumPartitions == MinPartitions[i] && Score > PartitionsScore[i])) {
          MinPartitions[i] = NumPartitions;
          LastElement[i] = j;
          PartitionsScore[i] = Score;
        }
      }
    }
  }

  // Iterate over the partitions, replacing some with jump tables in-place.
  unsigned DstIndex = 0;
  for (unsigned First = 0, Last; First < N; First = Last + 1) {
    Last = LastElement[First];
    assert(Last >= First);
    assert(DstIndex <= First);
    unsigned NumClusters = Last - First + 1;

    CaseCluster JTCluster;
    if (NumClusters >= MinJumpTableEntries &&
        canBuildJumpTable(Clusters, First, Last, SI, JTCluster)) {
      buildJumpTable(Clusters, First, Last, SI, JTCluster);
      Clusters[DstIndex++] = JTCluster;
    } else {
      for (unsigned I = First; I <= Last; ++I)
        Clusters[DstIndex++] = Clusters[I];
    }
  }
  Clusters.resize(DstIndex);
}

void SwitchCaseClusterFinder::findBitTestClusters(CaseClusterVector &Clusters,
                                                  const SwitchInst *SI) {
// Partition Clusters into as few subsets as possible, where each subset has a
// range that fits in a machine word and has <= 3 unique destinations.
#ifndef NDEBUG
  // Clusters must be sorted and contain Range or JumpTable clusters.
  assert(!Clusters.empty());
  assert(Clusters[0].Kind == CC_Range || Clusters[0].Kind == CC_JumpTable);
  for (const CaseCluster &C : Clusters)
    assert(C.Kind == CC_Range || C.Kind == CC_JumpTable);

  for (unsigned i = 1; i < Clusters.size(); ++i) {
    assert(Clusters[i - 1].High->getValue().slt(Clusters[i].Low->getValue()));
    // Case values in a cluster must be sorted.
    for (unsigned I = 1, E = Clusters[i].getNumerOfCases(); I != E; ++I) {
      const APInt &PreValue = Clusters[i].getCaseValueAt(SI, I - 1)->getValue();
      const APInt &CurValue = Clusters[i].getCaseValueAt(SI, I)->getValue();
      assert(PreValue.slt(CurValue));
    }
  }
#endif

  // The algorithm below is not suitable for -O0.
  if (OptLevel == CodeGenOpt::None)
    return;

  // If target does not have legal shift left, do not emit bit tests at all.
  EVT PTy = TLI.getPointerTy(DL);
  if (!TLI.isOperationLegal(ISD::SHL, PTy))
    return;

  int BitWidth = PTy.getSizeInBits();
  const int64_t N = Clusters.size();

  // MinPartitions[i] is the minimum nbr of partitions of Clusters[i..N-1].
  SmallVector<unsigned, 8> MinPartitions(N);
  // LastElement[i] is the last element of the partition starting at i.
  SmallVector<unsigned, 8> LastElement(N);

  // FIXME: This might not be the best algorithm for finding bit test clusters.

  // Base case: There is only one way to partition Clusters[N-1].
  MinPartitions[N - 1] = 1;
  LastElement[N - 1] = N - 1;

  // Note: loop indexes are signed to avoid underflow.
  for (int64_t i = N - 2; i >= 0; --i) {
    // Find optimal partitioning of Clusters[i..N-1].
    // Baseline: Put Clusters[i] into a partition on its own.
    MinPartitions[i] = MinPartitions[i + 1] + 1;
    LastElement[i] = i;

    // Search for a solution that results in fewer partitions.
    // Note: the search is limited by BitWidth, reducing time complexity.
    for (int64_t j = std::min(N - 1, i + BitWidth - 1); j > i; --j) {
      // Try building a partition from Clusters[i..j].

      // Check the range.
      if (!rangeFitsInWord(Clusters[i].Low->getValue(),
                           Clusters[j].High->getValue(), DL))
        continue;

      // Check nbr of destinations and cluster types.
      // FIXME: This works, but doesn't seem very efficient.
      bool RangesOnly = true;
      SmallPtrSet<const BasicBlock *, 8> Dests;
      for (int64_t k = i; k <= j; k++) {
        if (Clusters[k].Kind != CC_Range) {
          RangesOnly = false;
          break;
        }
        Dests.insert(Clusters[k].getSingleSuccessor(SI));
      }
      if (!RangesOnly || Dests.size() > 3)
        break;

      // Check if it's a better partition.
      unsigned NumPartitions = 1 + (j == N - 1 ? 0 : MinPartitions[j + 1]);
      if (NumPartitions < MinPartitions[i]) {
        // Found a better partition.
        MinPartitions[i] = NumPartitions;
        LastElement[i] = j;
      }
    }
  }

  // Iterate over the partitions, replacing with bit-test clusters in-place.
  unsigned DstIndex = 0;
  for (unsigned First = 0, Last; First < N; First = Last + 1) {
    Last = LastElement[First];
    assert(First <= Last);
    assert(DstIndex <= First);

    CaseCluster BitTestCluster;
    if (canBuildBitTest(Clusters, First, Last, SI, BitTestCluster)) {
      buildBitTests(Clusters, First, Last, SI, BitTestCluster);
      Clusters[DstIndex++] = BitTestCluster;
    } else {
      size_t NumClusters = Last - First + 1;
      for (unsigned I = 0; I != NumClusters; ++I)
        Clusters[DstIndex++] = Clusters[First + I];
    }
  }
  Clusters.resize(DstIndex);
}

void SwitchCaseClusterFinder::buildJumpTable(const CaseClusterVector &Clusters,
                                             unsigned First, unsigned Last,
                                             const SwitchInst *SI,
                                             CaseCluster &JTCluster) {
  assert(First <= Last);
  JTCluster = CaseCluster::jumpTable(Clusters[First].Low, Clusters[Last].High);

  for (unsigned I = First; I <= Last; ++I) {
    assert(Clusters[I].Kind == CC_Range);
    JTCluster.Cases.insert(JTCluster.Cases.end(), Clusters[I].Cases.begin(),
                           Clusters[I].Cases.end());
  }
}

void SwitchCaseClusterFinder::buildBitTests(CaseClusterVector &Clusters,
                                            unsigned First, unsigned Last,
                                            const SwitchInst *SI,
                                            CaseCluster &BTCluster) {
  BTCluster = CaseCluster::bitTests(Clusters[First].Low, Clusters[Last].High);

  for (unsigned I = First; I <= Last; ++I) {
    assert(Clusters[I].Kind == CC_Range);
    BTCluster.Cases.insert(BTCluster.Cases.end(), Clusters[I].Cases.begin(),
                           Clusters[I].Cases.end());
  }
}

const BasicBlock *
SwitchCaseClusterFinder::findClusters(const SwitchInst &SI,
                                      CaseClusterVector &Clusters) {
  const BasicBlock *DefaultBB = formInitalCaseClusers(SI, Clusters);
  if (!Clusters.empty()) {
    findJumpTables(Clusters, &SI);
    findBitTestClusters(Clusters, &SI);
  }
  return DefaultBB;
}
