//===- llvm/CodeGen/ReachingPhysDefs.h --- Reaching definitions -*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Reaching definitions of physical registers.
//
// This algorithm originated in the AArch64CollectLOHs pass. It was
// factored out so that targets could start to use a common class for
// reaching definitions. More specifically, SystemZ::ElimCompare.cpp
// was the initial reason for trying this. It is the hope and ambition
// this will evolve into a full fledged post-ra def/use analalyzer
// that suits the needs of all targets.

#ifndef LLVM_CODEGEN_REACHINGDEFS_H
#define LLVM_CODEGEN_REACHINGDEFS_H

#include "llvm/ADT/BitVector.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/MapVector.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineDominators.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstr.h"
using namespace llvm;

namespace llvm {

/// A set of MachineInstruction. Const is not used as it is allowed
/// for users of ReachingPhysDefs to transform instructions in maps.
struct SetOfMachineInstr : SetVector<MachineInstr *> {
  void dump();
};

/// Map a basic block to a set of instructions per register.
/// This is used to represent the exposed uses of a basic block
/// per register.
typedef MapVector<const MachineBasicBlock *,
                  std::unique_ptr<SetOfMachineInstr[]>>
  BlockToSetOfInstrsPerColor;

/// Map a basic block to an instruction per register.
/// This is used to represent the live-out definitions of a basic block
/// per register.
typedef MapVector<const MachineBasicBlock *,
                  std::unique_ptr<MachineInstr *[]>>
  BlockToInstrPerColor;

/// Map an instruction to a set of instructions. Used to represent the
/// mapping def to reachable uses or use to definitions.
typedef  MapVector<MachineInstr *, SetOfMachineInstr> InstrToInstrs;

/// Map a basic block to a BitVector.
/// This is used to record the kill registers per basic block.
typedef MapVector<const MachineBasicBlock *, BitVector> BlockToRegSet;

/// Map a register to a dense id.
typedef DenseMap<unsigned, unsigned> MapRegToId;

/// Map a dense id to a register. Used for debug purposes.
typedef SmallVector<unsigned, 32> MapIdToReg;

class ReachingPhysDefs {
protected:
  MachineFunction *MF;
  const TargetRegisterInfo *TRI;
  const TargetInstrInfo *TII;

  /// Name for this map, in debug outputs.
  std::string Name;

  /// If true, only a local analysis is done.
  bool BasicBlockScopeOnly;

  /// Dummy uses are recorded using this MI, during local analysis.
  MachineInstr *DummyOp;

  /// structures:
  /// For each basic block.
  /// Out: a set per color of definitions that reach the
  ///      out boundary of this block.
  /// In: Same as Out but for in boundary.
  /// Gen: generated color in this block (one operation per color).
  /// Kill: register set of killed color in this block.
  /// ReachableUses: a set per color of uses (operation) reachable
  ///                for "In" definitions.
  BlockToSetOfInstrsPerColor Out, In, ReachableUses;
  BlockToInstrPerColor Gen;
  BlockToRegSet Kill;

  /// Given a couple (MBB, reg) get the corresponding set of
  /// instructions from the given "sets".
  SetOfMachineInstr &getSet(BlockToSetOfInstrsPerColor &sets,
                            const MachineBasicBlock &MBB, unsigned reg);

  /// Given a couple (reg, MI) get the corresponding set of
  /// instructions from the the given "sets".
  SetOfMachineInstr &getUses(InstrToInstrs *sets, unsigned reg,
                             MachineInstr &MI);

  /// Same as getUses but does not modify the input map "sets".
  /// \return NULL if the couple (reg, MI) is not in sets.
  const SetOfMachineInstr *getUses(const InstrToInstrs *sets, unsigned reg,
                                   MachineInstr &MI);

  //// Virtual methods that make it possible to override common analysis
  //// results in a a specialized transformation context.

  /// Process the use operands of/MI.
  virtual void processMIUses(MachineInstr &MI);
  /// Given MO, return a used register if any.
  virtual unsigned processMIUseMO(MachineOperand &MO);
  /// Process MI clobbsers (regmask)
  virtual void processMIClobbers(MachineInstr &MI);
  /// Process the defs of MI.
  virtual void processMIDefs(MachineInstr &MI);

  /// Indicate if an MI that clobbers a reg should be inserted into
  /// maps.
  virtual bool rememberClobberingMI() { return false; }

  /// Indicate if dummy uses should be added to DummyOp in case only
  /// doing local analysis.
  virtual bool recordDummyUses() { return true; }

  /// Initialize the reaching definition algorithm.
  void initReachingDef();

  /// Reaching definition algorithm.
  void reachingDef();

  /// Reaching def core algorithm.
  void reachingDefAlgorithm();

  /// Build the inverse map from Use to Defs.
  virtual void reachedUsesToDefs();

  // Register numbers are not stored directly in maps but are mapped
  // to id's that start from 0.

  /// Current ID, to be assigned to next reg added.
  unsigned CurRegId;
  /// Map from reg to its id.
  MapRegToId RegToId;
  /// Map from id to reg.
  MapIdToReg IdToReg;

  ///// The resulting data structures

  /// A two dimensional map from [reg][DefMI] -> {Uses}
  InstrToInstrs *ColorOpToReachedUses;
  /// A two dimensional map from [reg][UseMI] -> {Defs}
  InstrToInstrs *ColorOpToReachingDefs;
  /// A map from [UseMI] -> {All DefMIs of any used reg of UseMI}
  InstrToInstrs UseToDefs;

public:
  ReachingPhysDefs(MachineFunction *MF_, const TargetRegisterInfo *TRI_,
                   bool BasicBlockScopeOnly,
                   const Twine &name = "Reaching defs");
  ~ReachingPhysDefs();

  /// Add reg to the set of registers to be handled by the algorithm.
  void addReg(unsigned reg);

  /// Add all target registers to be handled by the algorithm.
  void addAllTargetRegs();

  /// Return number of regs added to maps.
  unsigned nbRegs() { return RegToId.size(); }

  /// Return true if reg has already been added.
  bool isRegInMaps(unsigned reg) {
    return (RegToId.find(reg) != RegToId.end());
  }

  /// Do a reaching defs analyzis on a given set of phys regs. If a
  /// single reg is given, it is added.
  bool analyze(unsigned singleReg = 0);

  /// Some transformations rather leave things alone if a use of a reg
  /// have multiple defs. This method removes all defs with such a
  /// user from the ColorOpToReachedUses map, for the register
  /// involved.
  void pruneMultipleDefs();

#ifndef NDEBUG
  /// print the result of the reaching definition algorithm.
  void dump();
#endif

  /// Return the uses of MIs' definition of reg.
  SetOfMachineInstr *getUses(unsigned reg, MachineInstr &MI);
};

} // end llvm namespace.

#endif
