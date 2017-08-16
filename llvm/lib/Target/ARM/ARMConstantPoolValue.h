//===-- ARMConstantPoolValue.h - ARM constantpool value ---------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the ARM specific constantpool value class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_ARM_ARMCONSTANTPOOLVALUE_H
#define LLVM_LIB_TARGET_ARM_ARMCONSTANTPOOLVALUE_H

#include "llvm/ADT/StringRef.h"
#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/Support/Casting.h"
#include <string>
#include <vector>

namespace llvm {

class BlockAddress;
class Constant;
class GlobalValue;
class GlobalVariable;
class LLVMContext;
class MachineBasicBlock;

namespace ARMCP {

enum ARMCPKind {
  CPValue,
  CPExtSymbol,
  CPBlockAddress,
  CPLSDA,
  CPMachineBasicBlock,
  CPPromotedGlobal,
  CPIndexAddress
};

enum ARMCPModifier {
  no_modifier, /// None
  TLSGD,       /// Thread Local Storage (General Dynamic Mode)
  GOT_PREL,    /// Global Offset Table, PC Relative
  GOTTPOFF,    /// Global Offset Table, Thread Pointer Offset
  TPOFF,       /// Thread Pointer Offset
  SECREL,      /// Section Relative (Windows TLS)
  SBREL,       /// Static Base Relative (RWPI)
};

} // end namespace ARMCP

/// ARMConstantPoolValue - ARM specific constantpool value. This is used to
/// represent PC-relative displacement between the address of the load
/// instruction and the constant being loaded, i.e. (&GV-(LPIC+8)).
class ARMConstantPoolValue : public MachineConstantPoolValue {
  unsigned LabelId;        // Label id of the load.
  ARMCP::ARMCPKind Kind;   // Kind of constant.
  unsigned char PCAdjust;  // Extra adjustment if constantpool is pc-relative.
                           // 8 for ARM, 4 for Thumb.
  ARMCP::ARMCPModifier Modifier;   // GV modifier i.e. (&GV(modifier)-(LPIC+8))
  bool AddCurrentAddress;

protected:
  ARMConstantPoolValue(Type *Ty, unsigned id, ARMCP::ARMCPKind Kind,
                       unsigned char PCAdj, ARMCP::ARMCPModifier Modifier,
                       bool AddCurrentAddress);

  ARMConstantPoolValue(LLVMContext &C, unsigned id, ARMCP::ARMCPKind Kind,
                       unsigned char PCAdj, ARMCP::ARMCPModifier Modifier,
                       bool AddCurrentAddress);

  template <typename Derived>
  int getExistingMachineCPValueImpl(MachineConstantPool *CP,
                                    unsigned Alignment) {
    unsigned AlignMask = Alignment - 1;
    const std::vector<MachineConstantPoolEntry> &Constants = CP->getConstants();
    for (unsigned i = 0, e = Constants.size(); i != e; ++i) {
      if (Constants[i].isMachineConstantPoolEntry() &&
          (Constants[i].getAlignment() & AlignMask) == 0) {
        ARMConstantPoolValue *CPV =
            (ARMConstantPoolValue *)Constants[i].Val.MachineCPVal;
        if (Derived *APC = dyn_cast<Derived>(CPV))
          if (cast<Derived>(this)->equals(APC))
            return i;
      }
    }

    return -1;
  }

public:
  ~ARMConstantPoolValue() override;

  ARMCP::ARMCPModifier getModifier() const { return Modifier; }
  StringRef getModifierText() const;
  bool hasModifier() const { return Modifier != ARMCP::no_modifier; }

  bool mustAddCurrentAddress() const { return AddCurrentAddress; }

  unsigned getLabelId() const { return LabelId; }
  unsigned char getPCAdjustment() const { return PCAdjust; }

  bool isGlobalValue() const { return Kind == ARMCP::CPValue; }
  bool isExtSymbol() const { return Kind == ARMCP::CPExtSymbol; }
  bool isBlockAddress() const { return Kind == ARMCP::CPBlockAddress; }
  bool isLSDA() const { return Kind == ARMCP::CPLSDA; }
  bool isMachineBasicBlock() const{ return Kind == ARMCP::CPMachineBasicBlock; }
  bool isPromotedGlobal() const{ return Kind == ARMCP::CPPromotedGlobal; }
  bool isIndexAddress() const { return Kind == ARMCP::CPIndexAddress; }

  int getExistingMachineCPValue(MachineConstantPool *CP,
                                unsigned Alignment) override;

  void addSelectionDAGCSEId(FoldingSetNodeID &ID) override;

  /// hasSameValue - Return true if this ARM constpool value can share the same
  /// constantpool entry as another ARM constpool value.
  virtual bool hasSameValue(ARMConstantPoolValue *ACPV);

  bool equals(const ARMConstantPoolValue *A) const {
    return this->LabelId == A->LabelId &&
      this->PCAdjust == A->PCAdjust &&
      this->Modifier == A->Modifier;
  }

  void print(raw_ostream &O) const override;
  void print(raw_ostream *O) const { if (O) print(*O); }
  void dump() const;
};

inline raw_ostream &operator<<(raw_ostream &O, const ARMConstantPoolValue &V) {
  V.print(O);
  return O;
}

/// ARMConstantPoolConstant - ARM-specific constant pool values for Constants,
/// Functions, and BlockAddresses.
class ARMConstantPoolConstant : public ARMConstantPoolValue {
  const Constant *CVal;         // Constant being loaded.
  const GlobalVariable *GVar = nullptr;

  ARMConstantPoolConstant(const Constant *C,
                          unsigned ID,
                          ARMCP::ARMCPKind Kind,
                          unsigned char PCAdj,
                          ARMCP::ARMCPModifier Modifier,
                          bool AddCurrentAddress);
  ARMConstantPoolConstant(Type *Ty, const Constant *C,
                          unsigned ID,
                          ARMCP::ARMCPKind Kind,
                          unsigned char PCAdj,
                          ARMCP::ARMCPModifier Modifier,
                          bool AddCurrentAddress);
  ARMConstantPoolConstant(const GlobalVariable *GV, const Constant *Init);

public:
  static ARMConstantPoolConstant *Create(const Constant *C, unsigned ID);
  static ARMConstantPoolConstant *Create(const GlobalValue *GV,
                                         ARMCP::ARMCPModifier Modifier);
  static ARMConstantPoolConstant *Create(const GlobalVariable *GV,
                                         const Constant *Initializer);
  static ARMConstantPoolConstant *Create(const Constant *C, unsigned ID,
                                         ARMCP::ARMCPKind Kind,
                                         unsigned char PCAdj);
  static ARMConstantPoolConstant *Create(const Constant *C, unsigned ID,
                                         ARMCP::ARMCPKind Kind,
                                         unsigned char PCAdj,
                                         ARMCP::ARMCPModifier Modifier,
                                         bool AddCurrentAddress);

  const GlobalValue *getGV() const;
  const BlockAddress *getBlockAddress() const;

  const GlobalVariable *getPromotedGlobal() const {
    return dyn_cast_or_null<GlobalVariable>(GVar);
  }

  const Constant *getPromotedGlobalInit() const {
    return CVal;
  }

  int getExistingMachineCPValue(MachineConstantPool *CP,
                                unsigned Alignment) override;

  /// hasSameValue - Return true if this ARM constpool value can share the same
  /// constantpool entry as another ARM constpool value.
  bool hasSameValue(ARMConstantPoolValue *ACPV) override;

  void addSelectionDAGCSEId(FoldingSetNodeID &ID) override;

  void print(raw_ostream &O) const override;

  static bool classof(const ARMConstantPoolValue *APV) {
    return APV->isGlobalValue() || APV->isBlockAddress() || APV->isLSDA() ||
           APV->isPromotedGlobal();
  }

  bool equals(const ARMConstantPoolConstant *A) const {
    return CVal == A->CVal && ARMConstantPoolValue::equals(A);
  }
};

/// ARMConstantPoolSymbol - ARM-specific constantpool values for external
/// symbols.
class ARMConstantPoolSymbol : public ARMConstantPoolValue {
  const std::string S;          // ExtSymbol being loaded.

  ARMConstantPoolSymbol(LLVMContext &C, StringRef s, unsigned id,
                        unsigned char PCAdj, ARMCP::ARMCPModifier Modifier,
                        bool AddCurrentAddress);

public:
  static ARMConstantPoolSymbol *Create(LLVMContext &C, StringRef s, unsigned ID,
                                       unsigned char PCAdj);

  StringRef getSymbol() const { return S; }

  int getExistingMachineCPValue(MachineConstantPool *CP,
                                unsigned Alignment) override;

  void addSelectionDAGCSEId(FoldingSetNodeID &ID) override;

  /// hasSameValue - Return true if this ARM constpool value can share the same
  /// constantpool entry as another ARM constpool value.
  bool hasSameValue(ARMConstantPoolValue *ACPV) override;

  void print(raw_ostream &O) const override;

  static bool classof(const ARMConstantPoolValue *ACPV) {
    return ACPV->isExtSymbol();
  }

  bool equals(const ARMConstantPoolSymbol *A) const {
    return S == A->S && ARMConstantPoolValue::equals(A);
  }
};

/// ARMConstantPoolMBB - ARM-specific constantpool value of a machine basic
/// block.
class ARMConstantPoolMBB : public ARMConstantPoolValue {
  const MachineBasicBlock *MBB; // Machine basic block.

  ARMConstantPoolMBB(LLVMContext &C, const MachineBasicBlock *mbb, unsigned id,
                     unsigned char PCAdj, ARMCP::ARMCPModifier Modifier,
                     bool AddCurrentAddress);

public:
  static ARMConstantPoolMBB *Create(LLVMContext &C,
                                    const MachineBasicBlock *mbb,
                                    unsigned ID, unsigned char PCAdj);

  const MachineBasicBlock *getMBB() const { return MBB; }

  int getExistingMachineCPValue(MachineConstantPool *CP,
                                unsigned Alignment) override;

  void addSelectionDAGCSEId(FoldingSetNodeID &ID) override;

  /// hasSameValue - Return true if this ARM constpool value can share the same
  /// constantpool entry as another ARM constpool value.
  bool hasSameValue(ARMConstantPoolValue *ACPV) override;

  void print(raw_ostream &O) const override;

  static bool classof(const ARMConstantPoolValue *ACPV) {
    return ACPV->isMachineBasicBlock();
  }

  bool equals(const ARMConstantPoolMBB *A) const {
    return MBB == A->MBB && ARMConstantPoolValue::equals(A);
  }
};

/// ARMConstantPoolIndexAddress - ARM-specific constantpool value of an index to
/// another constant pool
class ARMConstantPoolIndexAddress : public ARMConstantPoolValue {
  int Index;

  ARMConstantPoolIndexAddress(LLVMContext &C, int Index, unsigned ID,
                              unsigned char PCAdj,
                              ARMCP::ARMCPModifier Modifier,
                              bool AddCurrentAddress);

public:
  static ARMConstantPoolIndexAddress *Create(LLVMContext &C, int Index,
                                             unsigned ID, unsigned char PCAdj);

  int getIndex() const { return Index; }

  int getExistingMachineCPValue(MachineConstantPool *CP,
                                unsigned Alignment) override;

  void addSelectionDAGCSEId(FoldingSetNodeID &ID) override;

  /// hasSameValue - Return true if this ARM constpool value can share the same
  /// constantpool entry as another ARM constpool value.
  bool hasSameValue(ARMConstantPoolValue *ACPV) override;

  void print(raw_ostream &O) const override;

  static bool classof(const ARMConstantPoolValue *ACPV) {
    return ACPV->isIndexAddress();
  }

  bool equals(const ARMConstantPoolIndexAddress *A) const;
};

} // end namespace llvm

#endif // LLVM_LIB_TARGET_ARM_ARMCONSTANTPOOLVALUE_H
