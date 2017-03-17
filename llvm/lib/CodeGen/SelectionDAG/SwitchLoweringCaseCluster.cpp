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

#include "llvm/CodeGen/SwitchLoweringCaseCluster.h"
#include "SelectionDAGBuilder.h"
#include "llvm/Analysis/BranchProbabilityInfo.h"

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

bool SwitchLoweringCaseClusterBuilder::isDense(
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

bool SwitchLoweringCaseClusterBuilder::canBuildBitTest(
    CaseClusterVector &Clusters, unsigned First, unsigned Last,
    const SwitchInst *SI, CaseCluster &BTCluster) {
  assert(First <= Last);
  if (First == Last)
    return false;

  SmallPtrSet<const BasicBlock *, 4> Dests;
  unsigned NumCmps = 0;
  for (int64_t I = First; I <= Last; ++I) {
    assert(Clusters[I].Kind == CC_Range);
    Dests.insert(Clusters[I].BB);
    NumCmps += (Clusters[I].Low == Clusters[I].High) ? 1 : 2;
  }
  unsigned NumDests = Dests.size();

  APInt Low = Clusters[First].Low->getValue();
  APInt High = Clusters[Last].High->getValue();
  assert(Low.slt(High));

  return isSuitableForBitTests(NumDests, NumCmps, Low, High);
}

void SwitchLoweringCaseClusterBuilder::sortAndRangeify(
    CaseClusterVector &Clusters) {
#ifndef NDEBUG
  for (const CaseCluster &CC : Clusters)
    assert(CC.Low == CC.High && "Input clusters must be single-case");
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
    // MachineBasicBlock *Succ = CC.MBB;
    const BasicBlock *Succ = CC.BB;

    // if (DstIndex != 0 && Clusters[DstIndex - 1].MBB == Succ &&
    if (DstIndex != 0 && Clusters[DstIndex - 1].BB == Succ &&
        (CaseVal->getValue() - Clusters[DstIndex - 1].High->getValue()) == 1) {
      // If this case has the same successor and is a neighbour, merge it into
      // the previous cluster.
      Clusters[DstIndex - 1].High = CaseVal;
      Clusters[DstIndex - 1].Prob += CC.Prob;
    } else {
      std::memmove(&Clusters[DstIndex++], &Clusters[SrcIndex],
                   sizeof(Clusters[SrcIndex]));
    }
  }
  Clusters.resize(DstIndex);
}

const BasicBlock *SwitchLoweringCaseClusterBuilder::replaceUnrechableDefault(
    const SwitchInst &SI, CaseClusterVector &Clusters) {
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
        if (CC.BB != DefaultBB)
          New.push_back(CC);
      }
      Clusters = std::move(New);
    }
  }
  return DefaultBB;
}

void SwitchLoweringCaseClusterBuilder::formInitalCaseCluser(
    const SwitchInst &SI, CaseClusterVector &Clusters,
    BranchProbabilityInfo *BPI) {
  Clusters.reserve(SI.getNumCases());
  for (auto I : SI.cases()) {
    const BasicBlock *Succ = I.getCaseSuccessor();
    const ConstantInt *CaseVal = I.getCaseValue();
    BranchProbability Prob =
        BPI ? BPI->getEdgeProbability(SI.getParent(), I.getSuccessorIndex())
            : BranchProbability(1, SI.getNumCases() + 1);
    Clusters.push_back(CaseCluster::range(CaseVal, CaseVal, Succ, Prob));
  }
  // Cluster adjacent cases with the same destination. We do this at all
  // optimization levels because it's cheap to do and will make codegen faster
  // if there are many clusters.
  sortAndRangeify(Clusters);
}

bool SwitchLoweringCaseClusterBuilder::rangeFitsInWord(const APInt &Low,
                                                       const APInt &High) {
  // FIXME: Using the pointer type doesn't seem ideal.
  uint64_t BW = DL.getPointerSizeInBits();
  uint64_t Range = (High - Low).getLimitedValue(UINT64_MAX - 1) + 1;
  return Range <= BW;
}

bool SwitchLoweringCaseClusterBuilder::isSuitableForBitTests(
    unsigned NumDests, unsigned NumCmps, const APInt &Low, const APInt &High) {
  // FIXME: I don't think NumCmps is the correct metric: a single case and a
  // range of cases both require only one branch to lower. Just looking at the
  // number of clusters and destinations should be enough to decide whether to
  // build bit tests.

  // To lower a range with bit tests, the range must fit the bitwidth of a
  // machine word.
  if (!rangeFitsInWord(Low, High))
    return false;

  // Decide whether it's profitable to lower this range with bit tests. Each
  // destination requires a bit test and branch, and there is an overall range
  // check branch. For a small number of clusters, separate comparisons might be
  // cheaper, and for many destinations, splitting the range might be better.
  return (NumDests == 1 && NumCmps >= 3) || (NumDests == 2 && NumCmps >= 5) ||
         (NumDests == 3 && NumCmps >= 6);
}

bool SwitchLoweringCaseClusterBuilder::canBuildJumpTable(
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
    JTProbs.insert(Clusters[I].BB);
  }
  unsigned NumDests = JTProbs.size();
  return !(isSuitableForBitTests(NumDests, NumCmps,
                                 Clusters[First].Low->getValue(),
                                 Clusters[Last].High->getValue()));
}

void SwitchLoweringCaseClusterBuilder::findJumpTables(
    CaseClusterVector &Clusters, const SwitchInst *SI,
    const BasicBlock *DefaultBB) {
#ifndef NDEBUG
  // Clusters must be non-empty, sorted, and only contain Range clusters.
  assert(!Clusters.empty());
  for (CaseCluster &C : Clusters)
    assert(C.Kind == CC_Range);
  for (unsigned i = 1, e = Clusters.size(); i < e; ++i)
    assert(Clusters[i - 1].High->getValue().slt(Clusters[i].Low->getValue()));
#endif

  if (!areJTsAllowed(TLI, SI))
    return;

  const bool OptForSize = DefaultBB->getParent()->optForSize();

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
    const APInt &Hi = Clusters[i].High->getValue();
    const APInt &Lo = Clusters[i].Low->getValue();
    TotalCases[i] = (Hi - Lo).getLimitedValue() + 1;
    if (i != 0)
      TotalCases[i] += TotalCases[i - 1];
  }

  const unsigned MinDensity =
      OptForSize ? OptsizeJumpTableDensity : JumpTableDensity;

  // Cheap case: the whole range may be suitable for jump table.
  unsigned JumpTableSize =
      (Clusters[N - 1].High->getValue() - Clusters[0].Low->getValue())
          .getLimitedValue(UINT_MAX - 1) + 1;
  if (JumpTableSize <= MaxJumpTableSize &&
      isDense(Clusters, TotalCases, 0, N - 1, MinDensity)) {
    CaseCluster JTCluster;
    if (canBuildJumpTable(Clusters, 0, N - 1, SI, JTCluster)) {
      buildJumpTable(Clusters, 0, N - 1, SI, DefaultBB, JTCluster);
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
      buildJumpTable(Clusters, First, Last, SI, DefaultBB, JTCluster);
      Clusters[DstIndex++] = JTCluster;
    } else {
      for (unsigned I = First; I <= Last; ++I)
        std::memmove(&Clusters[DstIndex++], &Clusters[I], sizeof(Clusters[I]));
    }
  }
  Clusters.resize(DstIndex);
}

void SwitchLoweringCaseClusterBuilder::findBitTestClusters(
    CaseClusterVector &Clusters, const SwitchInst *SI) {
// Partition Clusters into as few subsets as possible, where each subset has a
// range that fits in a machine word and has <= 3 unique destinations.

#ifndef NDEBUG
  // Clusters must be sorted and contain Range or JumpTable clusters.
  assert(!Clusters.empty());
  assert(Clusters[0].Kind == CC_Range || Clusters[0].Kind == CC_JumpTable);
  for (const CaseCluster &C : Clusters)
    assert(C.Kind == CC_Range || C.Kind == CC_JumpTable);
  for (unsigned i = 1; i < Clusters.size(); ++i)
    assert(Clusters[i - 1].High->getValue().slt(Clusters[i].Low->getValue()));
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
                           Clusters[j].High->getValue()))
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
        Dests.insert(Clusters[k].BB);
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
      std::memmove(&Clusters[DstIndex], &Clusters[First],
                   sizeof(Clusters[0]) * NumClusters);
      DstIndex += NumClusters;
    }
  }
  Clusters.resize(DstIndex);
}

void SwitchLoweringCaseClusterBuilder::buildJumpTable(
    const CaseClusterVector &Clusters, unsigned First, unsigned Last,
    const SwitchInst *SI, const BasicBlock *DefaultBB, CaseCluster &JTCluster) {
  assert(First <= Last);
  JTCluster = CaseCluster::jumpTable(Clusters[First].Low, Clusters[Last].High,
                                     0, BranchProbability::getZero());
}

void SwitchLoweringCaseClusterBuilder::buildBitTests(
    CaseClusterVector &Clusters, unsigned First, unsigned Last,
    const SwitchInst *SI, CaseCluster &BTCluster) {
  BTCluster = CaseCluster::bitTests(Clusters[First].Low, Clusters[Last].High, 0,
                                    BranchProbability::getZero());
}

void SwitchLoweringCaseClusterBuilderForDAG::buildJumpTable(
    const CaseClusterVector &Clusters, unsigned First, unsigned Last,
    const SwitchInst *SI, const BasicBlock *DefaultBB, CaseCluster &JTCluster) {
  assert(First <= Last);

  MachineBasicBlock *DefaultMBB = FuncInfo.MBBMap[DefaultBB];

  auto Prob = BranchProbability::getZero();
  std::vector<MachineBasicBlock *> Table;
  DenseMap<MachineBasicBlock *, BranchProbability> JTProbs;

  // Initialize probabilities in JTProbs.
  for (unsigned I = First; I <= Last; ++I)
    JTProbs[FuncInfo.MBBMap[Clusters[I].BB]] = BranchProbability::getZero();

  for (unsigned I = First; I <= Last; ++I) {
    assert(Clusters[I].Kind == CC_Range);
    Prob += Clusters[I].Prob;
    const APInt &Low = Clusters[I].Low->getValue();
    const APInt &High = Clusters[I].High->getValue();
    if (I != First) {
      // Fill the gap between this and the previous cluster.
      const APInt &PreviousHigh = Clusters[I - 1].High->getValue();
      assert(PreviousHigh.slt(Low));
      uint64_t Gap = (Low - PreviousHigh).getLimitedValue() - 1;
      for (uint64_t J = 0; J < Gap; J++)
        Table.push_back(DefaultMBB);
    }
    uint64_t ClusterSize = (High - Low).getLimitedValue() + 1;
    for (uint64_t J = 0; J < ClusterSize; ++J)
      Table.push_back(FuncInfo.MBBMap[Clusters[I].BB]);
    JTProbs[FuncInfo.MBBMap[Clusters[I].BB]] += Clusters[I].Prob;
  }

  // Create the MBB that will load from and jump through the table.
  // Note: We create it here, but it's not inserted into the function yet.
  MachineFunction *CurMF = FuncInfo.MF;
  MachineBasicBlock *JumpTableMBB =
      CurMF->CreateMachineBasicBlock(SI->getParent());

  // Add successors. Note: use table order for determinism.
  SmallPtrSet<MachineBasicBlock *, 8> Done;
  for (MachineBasicBlock *Succ : Table) {
    if (Done.count(Succ))
      continue;
    SDB->addSuccessorWithProb(JumpTableMBB, Succ, JTProbs[Succ]);
    Done.insert(Succ);
  }
  JumpTableMBB->normalizeSuccProbs();

  unsigned JTI = CurMF->getOrCreateJumpTableInfo(TLI.getJumpTableEncoding())
                     ->createJumpTableIndex(Table);

  // Set up the jump table info.
  JumpTableCase JT(-1U, JTI, JumpTableMBB, nullptr);
  JumpTableHeader JTH(Clusters[First].Low->getValue(),
                      Clusters[Last].High->getValue(), SI->getCondition(),
                      nullptr, false);
  SDB->JTCases.emplace_back(std::move(JTH), std::move(JT));

  JTCluster = CaseCluster::jumpTable(Clusters[First].Low, Clusters[Last].High,
                                     SDB->JTCases.size() - 1, Prob);
}

void SwitchLoweringCaseClusterBuilderForDAG::buildBitTests(
    CaseClusterVector &Clusters, unsigned First, unsigned Last,
    const SwitchInst *SI, CaseCluster &BTCluster) {
  APInt Low = Clusters[First].Low->getValue();
  APInt High = Clusters[Last].High->getValue();
  assert(Low.slt(High));

  APInt LowBound;
  APInt CmpRange;

  const int BitWidth = TLI.getPointerTy(DL).getSizeInBits();

  assert(rangeFitsInWord(Low, High) && "Case range must fit in bit mask!");

  // Check if the clusters cover a contiguous range such that no value in the
  // range will jump to the default statement.
  bool ContiguousRange = true;
  for (int64_t I = First + 1; I <= Last; ++I) {
    if (Clusters[I].Low->getValue() != Clusters[I - 1].High->getValue() + 1) {
      ContiguousRange = false;
      break;
    }
  }

  if (Low.isStrictlyPositive() && High.slt(BitWidth)) {
    // Optimize the case where all the case values fit in a word without having
    // to subtract minValue. In this case, we can optimize away the subtraction.
    LowBound = APInt::getNullValue(Low.getBitWidth());
    CmpRange = High;
    ContiguousRange = false;
  } else {
    LowBound = Low;
    CmpRange = High - Low;
  }

  CaseBitsVector CBV;
  auto TotalProb = BranchProbability::getZero();
  for (unsigned i = First; i <= Last; ++i) {
    // Find the CaseBits for this destination.
    unsigned j;
    for (j = 0; j < CBV.size(); ++j)
      if (CBV[j].BB == FuncInfo.MBBMap[Clusters[i].BB])
        break;
    if (j == CBV.size())
      CBV.push_back(CaseBits(0, FuncInfo.MBBMap[Clusters[i].BB], 0,
                             BranchProbability::getZero()));
    CaseBits *CB = &CBV[j];

    // Update Mask, Bits and ExtraProb.
    uint64_t Lo = (Clusters[i].Low->getValue() - LowBound).getZExtValue();
    uint64_t Hi = (Clusters[i].High->getValue() - LowBound).getZExtValue();
    assert(Hi >= Lo && Hi < 64 && "Invalid bit case!");
    CB->Mask |= (-1ULL >> (63 - (Hi - Lo))) << Lo;
    CB->Bits += Hi - Lo + 1;
    CB->ExtraProb += Clusters[i].Prob;
    TotalProb += Clusters[i].Prob;
  }

  BitTestInfo BTI;
  std::sort(CBV.begin(), CBV.end(), [](const CaseBits &a, const CaseBits &b) {
    // Sort by probability first, number of bits second.
    if (a.ExtraProb != b.ExtraProb)
      return a.ExtraProb > b.ExtraProb;
    return a.Bits > b.Bits;
  });

  for (auto &CB : CBV) {
    MachineBasicBlock *BitTestBB =
        FuncInfo.MF->CreateMachineBasicBlock(SI->getParent());
    BTI.push_back(BitTestCase(CB.Mask, BitTestBB, CB.BB, CB.ExtraProb));
  }
  SDB->BitTestCases.emplace_back(std::move(LowBound), std::move(CmpRange),
                                 SI->getCondition(), -1U, MVT::Other, false,
                                 ContiguousRange, nullptr, nullptr,
                                 std::move(BTI), TotalProb);

  BTCluster = CaseCluster::bitTests(Clusters[First].Low, Clusters[Last].High,
                                    SDB->BitTestCases.size() - 1, TotalProb);
}

const BasicBlock *
SwitchLoweringCaseCluster::findCaseClusters(const SwitchInst &SI,
                                            CaseClusterVector &Clusters,
                                            BranchProbabilityInfo *BPI) {
  ClusterBuilder->formInitalCaseCluser(SI, Clusters, BPI);
  const BasicBlock *DefaultBB =
      ClusterBuilder->replaceUnrechableDefault(SI, Clusters);
  if (!Clusters.empty()) {
    ClusterBuilder->findJumpTables(Clusters, &SI, DefaultBB);
    ClusterBuilder->findBitTestClusters(Clusters, &SI);
  }
  return DefaultBB;
}
