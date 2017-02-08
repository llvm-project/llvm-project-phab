//===-------------- VecClone.h - Class definition -*- C++ -*---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
// ===--------------------------------------------------------------------=== //
///
/// \file
/// This file defines the VecClone pass class.
///
// ===--------------------------------------------------------------------=== //

#include "llvm/Analysis/VectorVariant.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"

#ifndef LLVM_TRANSFORMS_UTILS_VECCLONE_H
#define LLVM_TRANSFORMS_UTILS_VECCLONE_H

namespace llvm {

class ModulePass;

/// \brief Represents the mapping of a vector parameter to its corresponding
/// vector to scalar type cast instruction. This done so that the scalar loop
/// inserted by this pass contains instructions that are in scalar form so that
/// the loop can later be vectorized.
struct ParmRef {
  // Represents the parameter in one of two forms:
  // 1) A vector alloca instruction if the parameter has not been registerized.
  // 2) The parameter as the Value* passed in via the function call.
  Value *VectorParm;

  // Represents the vector parameter cast from a vector type to scalar type.
  Instruction *VectorParmCast;
};

class VecClone : public ModulePass {

  private:

    /// \brief Contains the names of the declared vector function variants
    typedef std::vector<std::string> DeclaredVariants;

    /// \brief Contains a mapping of a function to its vector function variants
    typedef std::map<Function*, DeclaredVariants> FunctionVariants;

    /// \brief Get all function attributes that specify a vector variant.
    std::vector<Attribute> getVectorVariantAttributes(Function& F);

    /// \brief Determine the characteristic type of the vector function as
    /// specified according to the vector function ABI.
    Type* calcCharacteristicType(Function& F, VectorVariant& Variant);

    /// \brief Get all functions marked for vectorization in module and their
    /// list of variants.
    void getFunctionsToVectorize(
      Module &M, std::map<Function*, std::vector<StringRef> > &FuncVars);

    /// \brief Returns a floating point or integer constant depending on Ty.
    template <typename T>
    Constant* getConstantValue(Type *Ty, LLVMContext &Context, T Val);

    /// \brief Return true if the function has a complex type for the return
    /// or parameters.
    bool hasComplexType(Function *F);

    /// \brief Make a copy of the function if it is marked as SIMD.
    Function* cloneFunction(Function &F, VectorVariant &V);

    /// \brief Take the entry basic block for the function as split off a second
    /// basic block that will form the loop entry.
    BasicBlock* splitEntryIntoLoop(Function *Clone, VectorVariant &V,
                                   BasicBlock *EntryBlock);

    /// \brief Take the loop entry basic block and split off a second basic
    /// block into a new return basic block.
    BasicBlock* splitLoopIntoReturn(Function *Clone, BasicBlock *LoopBlock);

    /// \brief Generate a basic block to test the loop exit condition.
    BasicBlock* createLoopExit(Function *Clone, BasicBlock *ReturnBlock);

    /// \brief Update the predecessors of the return basic block.
    void updateReturnPredecessors(Function *Clone, BasicBlock *LoopExitBlock,
                                  BasicBlock *ReturnBlock);

    /// \brief Create the backedge from the loop exit basic block to the loop
    /// entry block.
    PHINode* createPhiAndBackedgeForLoop(Function *Clone,
                                         BasicBlock *EntryBlock,
                                         BasicBlock *LoopBlock,
                                         BasicBlock *LoopExitBlock,
                                         BasicBlock *ReturnBlock,
                                         int VL);

    /// \brief Generate vector alloca instructions for vector parameters and
    /// change the parameter types to vector types. Expand the return value of
    /// the function to a vector type. This function returns the instruction
    /// corresponding to the expanded return and the instruction corresponding
    /// to the mask.
    Instruction* expandVectorParametersAndReturn(
        Function *Clone,
        VectorVariant &V,
        Instruction **Mask,
        BasicBlock *EntryBlock,
        BasicBlock *LoopBlock,
        BasicBlock *ReturnBlock,
        SmallVectorImpl<ParmRef*>& ParmMap);

    /// \brief Expand the function parameters to vector types. This function
    /// returns the instruction corresponding to the mask.
    Instruction* expandVectorParameters(
        Function *Clone,
        VectorVariant &V,
        BasicBlock *EntryBlock,
        SmallVectorImpl<ParmRef*>& ParmMap);

    /// \brief Expand the function's return value to a vector type.
    Instruction* expandReturn(Function *Clone, BasicBlock *EntryBlock,
                             BasicBlock *LoopBlock, BasicBlock *ReturnBlock,
                             SmallVectorImpl<ParmRef*>& ParmMap);

    /// \brief Update the old parameter references to with the new vector
    /// references.
    void updateScalarMemRefsWithVector(
        Function *Clone,
        BasicBlock *EntryBlock,
        BasicBlock *ReturnBlock,
        PHINode *Phi,
        SmallVectorImpl<ParmRef*>& ParmMap);
        
    /// \brief Update the values of linear parameters by adding the stride
    /// before the use.
    void updateLinearReferences(Function *Clone, Function &F,
                                VectorVariant &V, PHINode *Phi);

    /// \brief Update the instructions in the return basic block to return a
    /// vector temp.
    void updateReturnBlockInstructions(Function *Clone, BasicBlock *ReturnBlock,
                                       Instruction *VecReturnAlloca);

    /// \brief Create a separate basic block to mark the begin and end of the
    /// SIMD loop formed from the vector function. Essentially, this function
    /// transfers the information from the SIMD function keywords and creates
    /// new loop pragmas so that parameter information can be transferred to
    /// the loop.
    void insertDirectiveIntrinsics(Module& M, Function *Clone, Function &F,
                                   VectorVariant &V,
                                   BasicBlock *EntryBlock, 
                                   BasicBlock *LoopExitBlock,
                                   BasicBlock *ReturnBlock);

    /// \brief Create the basic block indicating the begin of the SIMD loop.
    void insertBeginRegion(Module& M, Function *Clone, Function &F,
                           VectorVariant &V, BasicBlock *EntryBlock);

    /// \brief Create the basic block indicating the end of the SIMD loop.
    void insertEndRegion(Module& M, Function *Clone, BasicBlock *LoopExitBlock,
                         BasicBlock *ReturnBlock);

    /// \brief Create a new vector alloca instruction for the return vector and
    /// bitcast to the appropriate element type.
    Instruction* createExpandedReturn(Function *F, BasicBlock *BB,
                                      VectorType *ReturnType);

    /// \brief Return the position of the parameter in the function's parameter
    /// list.
    int getParmIndexInFunction(Function *F, Value *Parm);

    /// \brief Check to see if the function is simple enough that a loop does
    /// not need to be inserted into the function.
    bool isSimpleFunction(Function *Clone, VectorVariant &V,
                          ReturnInst *Return);

    /// \brief Inserts the if/else split and mask condition for masked SIMD
    /// functions.
    void insertSplitForMaskedVariant(Function *Clone, BasicBlock *LoopBlock,
                                     BasicBlock *LoopExitBlock,
                                     Instruction *Mask, PHINode *Phi);

    /// \brief Utility function to insert instructions with other instructions
    /// of the same kind.
    void insertInstruction(Instruction *Inst, BasicBlock *BB);

    /// \brief Utility function that generates instructions that calculate the
    /// stride for a linear parameter.
    Instruction* generateStrideForParameter(Function *Clone, Argument *Arg,
                                            Instruction *ParmUser, int Stride,
                                            PHINode *Phi);

    /// \brief Utility function that returns true if Inst is a store of a vector
    /// or linear parameter.
    bool isVectorOrLinearParamStore(Function *Clone,
                                    std::vector<VectorKind> &ParmKinds,
                                    Instruction *Inst);

    /// \brief Removes the original scalar alloca instructions that correspond
    /// to a vector parameter before widening.
    void removeScalarAllocasForVectorParams(
      SmallVectorImpl<ParmRef*> &VectorParmMap);

    /// \brief Adds metadata to the conditional branch of the simd loop latch to
    /// prevent loop unrolling.
    void disableLoopUnrolling(BasicBlock *Latch);

    /// \brief Check to see that the type of the gep used for a load instruction
    /// is compatible with the type needed as the result of the load. Basically,
    /// check the validity of the LLVM IR to make sure that proper pointer
    /// dereferencing is done.
    bool typesAreCompatibleForLoad(Type *GepType, Type *LoadType);

    bool runOnModule(Module &M) override;

  public:

    static char ID;
    VecClone();
    void print(raw_ostream &OS, const Module * = nullptr) const override;
    void getAnalysisUsage(AnalysisUsage &AU) const override;

}; // end pass class

ModulePass *createVecClonePass();

} // end llvm namespace

#endif // LLVM_TRANSFORMS_UTILS_VECCLONE_H
