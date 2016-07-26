//=------- VecClone.cpp - Vector function to loop transform -*- C++ -*-------=//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
// ===--------------------------------------------------------------------=== //
///
/// \file
/// This pass inserts the body of a vector function inside a vector length
/// trip count scalar loop for functions that are declared SIMD. The pass
/// follows the Intel vector ABI requirements for name mangling encodings.
///
// ===--------------------------------------------------------------------=== //

// This pass is flexible enough to deal with two forms of LLVM IR, namely the
// difference between when Mem2Reg has been run and when Mem2Reg has not run.
// i.e., the pass needs to be able to make the loop transformation at all
// opt levels.
//
// When Mem2Reg has run:
//
// define i32 @foo(i32 %i, i32 %x) #0 {
// entry:
//   %add = add nsw i32 %x, %i
//   ret i32 %add
// }
//
// When Mem2Reg has not run:
//
// define i32 @foo(i32 %i, i32 %x) #0 {
// entry:
// %i.addr = alloca i32, align 4
// %x.addr = alloca i32, align 4
// store i32 %i, i32* %i.addr, align 4
// store i32 %x, i32* %x.addr, align 4
// %0 = load i32, i32* %x.addr, align 4
// %1 = load i32, i32* %i.addr, align 4
// %add = add nsw i32 %0, %1
//  ret i32 %add
// }
//
// When Mem2Reg has not been run (i.e., parameters have not been registerized),
// we end up with an alloca, store to alloca, and load from alloca sequence.
// When parameters have already been registerized, users of the parameter use
// the parameter directly and not through a load of a new SSA temp. For either
// case, this pass will expand the vector parameters/return to vector types,
// alloca new space for them on the stack, and do an initial store to the
// alloca. Linear and uniform parameters will be used directly, instead of
// through a load instruction.

#include "llvm/Analysis/VectorUtils.h"
#include "llvm/Analysis/VectorVariant.h"
#include "llvm/Transforms/Utils/VecClone.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/InitializePasses.h"
#include "llvm/PassRegistry.h"
#include <map>
#include <set>

#define SV_NAME "vec-clone"
#define DEBUG_TYPE "VecClone"

using namespace llvm;

VecClone::VecClone() : ModulePass(ID) { }

void VecClone::getAnalysisUsage(AnalysisUsage &AU) const {
  // Placeholder for any new pass dependencies. For now, none are needed.
}

void VecClone::insertInstruction(Instruction *Inst, BasicBlock *BB)
{
  // This function inserts instructions in a way that groups like instructions
  // together for debuggability/readability purposes. This was designed to make
  // the entry basic block easier to read since this pass creates/modifies
  // alloca, store, and bitcast instructions for each vector parameter and
  // return. Thus, this function ensures all allocas are grouped together, all
  // stores are grouped together, and so on. If the type of instruction passed
  // in does not exist in the basic block, then it is added to the end of the
  // basic block, just before the terminator instruction.

  BasicBlock::reverse_iterator BBIt = BB->rbegin();
  BasicBlock::reverse_iterator BBEnd = BB->rend();
  BasicBlock::iterator AnchorInstIt = BB->end();
  AnchorInstIt--;
  Instruction *Anchor = &*AnchorInstIt;

  for (; BBIt != BBEnd; ++BBIt) {
    if (Inst->getOpcode() == (&*BBIt)->getOpcode()) {
      Anchor = &*BBIt;
      break;
    }
  }

  if (isa<BranchInst>(Anchor)) {
    Inst->insertBefore(Anchor);
  } else {
    Inst->insertAfter(Anchor);
  }
}

bool VecClone::hasComplexType(Function *F)
{
  Function::ArgumentListType &ArgList = F->getArgumentList();
  Function::ArgumentListType::iterator ArgListIt = ArgList.begin();
  Function::ArgumentListType::iterator ArgListEnd = ArgList.end();

  for (; ArgListIt != ArgListEnd; ++ArgListIt) {
    // Complex types for parameters/return come in as vector.
    if (ArgListIt->getType()->isVectorTy()) {
      return true;
    }
  }

  return false;
}

Function* VecClone::cloneFunction(Function &F, VectorVariant &V)
{

  DEBUG(dbgs() << "Cloning Function: " << F.getName() << "\n");
  DEBUG(F.dump());

  FunctionType* OrigFunctionType = F.getFunctionType();
  Type *ReturnType = F.getReturnType();
  Type *CharacteristicType = calcCharacteristicType(F, V);

  // Expand return type to vector.
  if (!ReturnType->isVoidTy())
    ReturnType = VectorType::get(ReturnType, V.getVlen());

  std::vector<VectorKind> ParmKinds = V.getParameters();
  SmallVector<Type*, 4> ParmTypes;
  FunctionType::param_iterator ParmIt = OrigFunctionType->param_begin();
  FunctionType::param_iterator ParmEnd = OrigFunctionType->param_end();
  std::vector<VectorKind>::iterator VKIt = ParmKinds.begin();
  for (; ParmIt != ParmEnd; ++ParmIt, ++VKIt) {
    if (VKIt->isVector())
      ParmTypes.push_back(VectorType::get((*ParmIt)->getScalarType(),
                                          V.getVlen()));
    else
      ParmTypes.push_back(*ParmIt);
  }

  if (V.isMasked()) {
    VectorType *MaskType = VectorType::get(CharacteristicType, V.getVlen());
    ParmTypes.push_back(MaskType);
  }

  FunctionType* CloneFuncType = FunctionType::get(ReturnType, ParmTypes,
                                                  false);

  std::string VariantName = V.generateFunctionName(F.getName());
  Function* Clone = Function::Create(CloneFuncType, F.getLinkage(),
                                     VariantName, F.getParent());

  // Remove vector variant attributes from the original function. They are
  // not needed for the cloned function and it prevents any attempts at
  // trying to clone the function again in case the pass is called more than
  // once.
  AttrBuilder AB;
  for (auto Attr : getVectorVariantAttributes(F)) {
    AB.addAttribute(Attr);
  }
  AttributeSet AttrsToRemove = AttributeSet::get(F.getContext(),
                                                 AttributeSet::FunctionIndex,
                                                 AB);

  F.removeAttributes(AttributeSet::FunctionIndex, AttrsToRemove);

  // Copy all the attributes from the scalar function to its vector version
  // except for the vector variant attributes.
  Clone->copyAttributesFrom(&F);

  // Remove incompatible argument attributes (applied to the scalar argument,
  // does not apply to its vector counterpart).
  Function::arg_iterator ArgIt = Clone->arg_begin();
  Function::arg_iterator ArgEnd = Clone->arg_end();
  for (uint64_t Idx = 1; ArgIt != ArgEnd; ++ArgIt, ++Idx) {
    Type* ArgType = (*ArgIt).getType();
    AB = AttributeFuncs::typeIncompatible(ArgType);
    AttributeSet AS = AttributeSet::get(Clone->getContext(), Idx, AB);
    (*ArgIt).removeAttr(AS);
  }

  ValueToValueMapTy Vmap;
  ArgIt = F.arg_begin();
  ArgEnd = F.arg_end();
  Function::arg_iterator NewArgIt = Clone->arg_begin();
  for (; ArgIt != ArgEnd; ++ArgIt, ++NewArgIt) {
    NewArgIt->setName(ArgIt->getName());
    Vmap[&*ArgIt] = &*NewArgIt;
  }

  if (V.isMasked()) {
    Argument &MaskArg = *NewArgIt;
    MaskArg.setName("mask");
  }

  SmallVector<ReturnInst*, 8> Returns;
  CloneFunctionInto(Clone, &F, Vmap, true, Returns);

  DEBUG(dbgs() << "After Cloning and Parameter/Return Expansion\n");
  DEBUG(Clone->dump());

  return Clone;
}

bool VecClone::isVectorOrLinearParamStore(
    Function *Clone,
    std::vector<VectorKind> &ParmKinds,
    Instruction *Inst)
{
  if (StoreInst *Store = dyn_cast<StoreInst>(Inst)) {
    Value *Op0 = Store->getOperand(0);
    Function::ArgumentListType &ArgList = Clone->getArgumentList();
    Function::ArgumentListType::iterator ArgListIt = ArgList.begin();
    Function::ArgumentListType::iterator ArgListEnd = ArgList.end();

    for (; ArgListIt != ArgListEnd; ++ArgListIt) {
      unsigned ParmIdx = ArgListIt->getArgNo();
      if (&*ArgListIt == Op0 &&
          (ParmKinds[ParmIdx].isVector() || ParmKinds[ParmIdx].isLinear())) {
        return true;
      }
    }
  }

  return false;
}

BasicBlock* VecClone::splitEntryIntoLoop(Function *Clone, VectorVariant &V,
                                         BasicBlock *EntryBlock)
{

  // EntryInsts contains all instructions that need to stay in the entry basic
  // block. These instructions include allocas and stores involving vector and
  // linear parameters to alloca. Linear parameter stores to alloca are kept in
  // the entry block because there will be a load from this alloca in the loop
  // for which we will apply the stride. Instructions involving uniform
  // parameter stores to alloca should be sunk into the loop to maintain
  // uniform behavior. All instructions involving private variables are also
  // sunk into the loop.

  SmallVector<Instruction*, 4> EntryInsts;
  std::vector<VectorKind> ParmKinds = V.getParameters();
  BasicBlock::iterator BBIt = EntryBlock->begin();
  BasicBlock::iterator BBEnd = EntryBlock->end();

  for (; BBIt != BBEnd; ++BBIt) {
    if (isa<AllocaInst>(BBIt) ||
        isVectorOrLinearParamStore(Clone, ParmKinds, &*BBIt)) {
      // If this is a store of a vector parameter, keep it in the entry block
      // because it will be modified with the vector alloca reference. Since the
      // parameter has already been expanded, this becomes a vector store (i.e.,
      // packing instruction) that we do not want to appear in the scalar loop.
      // It is correct to leave linear parameter stores in the entry or move
      // them to the scalar loop, but leaving them in the entry block prevents
      // an additional store inside the loop. Uniform parameter stores must be
      // moved to the loop body to behave as uniform. Consider the following:
      //
      // __declspec(vector(uniform(x)))
      // int foo(int a, int x) {
      //   x++;
      //   return (a + x);
      // }
      //
      // Assume x = 1 for the call to foo. This implies x = 2 for the vector
      // add. e.g., a[0:VL-1] + <2, 2, 2, 2>. If the initial store of x to the
      // stack is done in the entry block outside of the loop, then x will be
      // incremented by one each time within the loop because the increment of
      // x will reside in the loop. Therefore, if the store of x is sunk into
      // the loop, the initial value of 1 will always be stored to a temp
      // before the increment, resulting in the value of 2 always being computed
      // in the scalar loop.
      EntryInsts.push_back(&*BBIt);
    }
  }

  BasicBlock *LoopBlock = EntryBlock->splitBasicBlock(EntryBlock->begin(),
                                                      "simd.loop");

  for (auto *Inst : EntryInsts) {
    Inst->removeFromParent();
    Inst->insertBefore(EntryBlock->getTerminator());
  }

  DEBUG(dbgs() << "After Entry Block Split\n");
  DEBUG(Clone->dump());

  return LoopBlock;
}

BasicBlock* VecClone::splitLoopIntoReturn(Function *Clone,
                                          BasicBlock *LoopBlock)
{

  // Determine the basic block with the return. For simple cases, the 'ret'
  // instruction will be part of the entry block. In this case, separate the
  // 'ret' into a new basic block because we don't want this as part of the
  // loop. For more complex cases, the 'ret' and corresponding instructions
  // (i.e., load from auto variable) will already be in a separate basic block,
  // so no need to split here.

  Instruction *SplitPt = LoopBlock->getTerminator();

  if (ReturnInst *Return = dyn_cast<ReturnInst>(SplitPt)) {

    // If the return is from a preceeding load, make sure the load is also put
    // in the return block. This is the old scalar load that will end up getting
    // replaced with the vector return and will get cleaned up later.

    // Make sure this is not a void function before getting the return
    // operand.
    if (!Clone->getReturnType()->isVoidTy()) {
      Value *RetOp = Return->getOperand(0);
      Value::use_iterator UseIt = RetOp->use_begin();
      Value::use_iterator UseEnd = RetOp->use_end();

      for (; UseIt != UseEnd; ++UseIt) {
        LoadInst *RetLoad = dyn_cast<LoadInst>(*UseIt);
        if (RetLoad) {
          SplitPt = RetLoad;
        }
      }
    }
  }

  Function::iterator ReturnBlockIt = Clone->end();
  BasicBlock *ReturnBlock;
  if (dyn_cast<LoadInst>(SplitPt) || dyn_cast<ReturnInst>(SplitPt)) {
    ReturnBlock = LoopBlock->splitBasicBlock(SplitPt, "return");
  } else {
    ReturnBlockIt = Clone->end();
    ReturnBlockIt--;
    ReturnBlock = &*ReturnBlockIt;
  }

  return ReturnBlock;
}

void VecClone::updateReturnPredecessors(Function *Clone,
                                        BasicBlock *LoopExitBlock,
                                        BasicBlock *ReturnBlock)
{ 
  // Update the branches of the ReturnBlock predecessors to point back to
  // LoopBlock if the index is less than VL.

  // First, collect the basic blocks to be updated since we don't want to update
  // the CFG while iterating through it.
  SmallVector<BranchInst*, 4> BranchesToUpdate;
  Function::iterator FI = Clone->begin();
  Function::iterator FE = Clone->end();
  for (; FI != FE; ++FI) {

    BasicBlock::iterator BBI = FI->begin();
    BasicBlock::iterator BBE = FI->end();

    for (; BBI != BBE; ++BBI) {

      BranchInst *Branch = dyn_cast<BranchInst>(BBI);

      if (Branch) {
        unsigned NumSucc = Branch->getNumSuccessors();

        for (unsigned I = 0; I < NumSucc; ++I) {
          if (Branch->getSuccessor(I) == ReturnBlock) {
            BranchesToUpdate.push_back(Branch);
          }
        }
      }
    }
  }

  // Now, do the actual update. The code below handles both conditional and
  // unconditional branches because we loop through all successors of the
  // branch to see if any of them point to the ReturnBlock.
  for (unsigned I = 0; I < BranchesToUpdate.size(); ++I) {
    unsigned int NumOps = BranchesToUpdate[I]->getNumSuccessors();
    for (unsigned Idx = 0; Idx < NumOps; ++Idx) {
      BasicBlock *Successor = BranchesToUpdate[I]->getSuccessor(Idx);
      if (Successor == ReturnBlock) {
        BranchesToUpdate[I]->setOperand(Idx, LoopExitBlock);
      }
    }
  }
}

BasicBlock* VecClone::createLoopExit(Function *Clone, BasicBlock *ReturnBlock)
{
  BasicBlock *LoopExitBlock = BasicBlock::Create(Clone->getContext(),
                                                 "simd.loop.exit",
                                                 Clone, ReturnBlock);

  updateReturnPredecessors(Clone, LoopExitBlock, ReturnBlock);
  return LoopExitBlock;
}

PHINode* VecClone::createPhiAndBackedgeForLoop(
    Function *Clone,
    BasicBlock *EntryBlock,
    BasicBlock *LoopBlock,
    BasicBlock *LoopExitBlock,
    BasicBlock *ReturnBlock,
    int VectorLength)
{
                                                        
  // Create the phi node for the top of the loop block and add the back
  // edge to the loop from the loop exit.

  PHINode *Phi = PHINode::Create(Type::getInt32Ty(Clone->getContext()), 2,
                                 "index", &*LoopBlock->getFirstInsertionPt());

  Constant *Inc = ConstantInt::get(Type::getInt32Ty(Clone->getContext()), 1);
  Constant *IndInit = ConstantInt::get(Type::getInt32Ty(Clone->getContext()),
                                       0);

  Instruction *Induction = BinaryOperator::CreateNUWAdd(Phi, Inc, "indvar",
                                                        LoopExitBlock);

  Constant *VL = ConstantInt::get(Type::getInt32Ty(Clone->getContext()),
                                  VectorLength);

  Instruction *VLCmp = new ICmpInst(*LoopExitBlock, CmpInst::ICMP_ULT,
                                    Induction, VL, "vl.cond");

  BranchInst::Create(LoopBlock, ReturnBlock, VLCmp, LoopExitBlock);

  Phi->addIncoming(IndInit, EntryBlock);
  Phi->addIncoming(Induction, LoopExitBlock);

  DEBUG(dbgs() << "After Loop Insertion\n");
  DEBUG(Clone->dump());

  return Phi;
}

Instruction* VecClone::expandVectorParameters(
    Function *Clone,
    VectorVariant &V,
    BasicBlock *EntryBlock,
    std::vector<ParmRef*>& VectorParmMap)
{
  // For vector parameters, expand the existing alloca to a vector. Then,
  // bitcast the vector and store this instruction in a map. The map is later
  // used to insert the new instructions and to replace the old scalar memory
  // references. If there are no parameters, then the function simply does not
  // perform any expansion since we iterate over the function's arg list.

  Instruction *Mask = nullptr;
  SmallVector<StoreInst*, 4> StoresToInsert;

  Function::ArgumentListType &CloneArgList = Clone->getArgumentList();
  Function::ArgumentListType::iterator ArgIt = CloneArgList.begin();
  Function::ArgumentListType::iterator ArgEnd = CloneArgList.end();

  for (; ArgIt != ArgEnd; ++ArgIt) {

    User::user_iterator UserIt = ArgIt->user_begin();
    User::user_iterator UserEnd = ArgIt->user_end();

    VectorType *VecType = dyn_cast<VectorType>(ArgIt->getType());

    if (VecType) {

      // Create a new vector alloca and bitcast to a pointer to the
      // element type. The following is an example of what the cast
      // should look like:
      //
      // %veccast = bitcast <2 x i32>* %vec_a.addr to i32*
      //
      // geps using the bitcast will appear in a scalar form instead
      // of casting to an array or using vector. For example,
      //
      // %vecgep1 = getelementptr i32, i32* %veccast, i32 %index
      //
      // instead of:
      //
      // getelementptr inbounds [4 x i32], [4 x i32]* %a, i32 0, i64 1
      //
      // We do this to put the geps in a more scalar form.

      Twine VarName = "vec." + ArgIt->getName();
      AllocaInst *VecAlloca = new AllocaInst(VecType, VarName);
      insertInstruction(VecAlloca, EntryBlock);
      PointerType *ElemTypePtr =
          PointerType::get(VecType->getElementType(),
                           VecAlloca->getType()->getAddressSpace());

      BitCastInst *VecParmCast = nullptr;
      if (ArgIt->getNumUses() == 0 && V.isMasked()) {
        Mask = new BitCastInst(VecAlloca, ElemTypePtr, "mask.cast");
      } else {
        Twine CastName = "vec." + ArgIt->getName() + ".cast";
        VecParmCast = new BitCastInst(VecAlloca, ElemTypePtr, CastName);
        insertInstruction(VecParmCast, EntryBlock);
      }

      for (; UserIt != UserEnd; ++UserIt) {

        StoreInst *StoreUser = dyn_cast<StoreInst>(*UserIt);
        AllocaInst *Alloca = NULL;
        ParmRef *PRef = new ParmRef();

        if (StoreUser) {

          // For non-mask parameters, find the initial store of the parameter
          // to an alloca instruction. Map this alloca to the vector bitcast
          // created above so that we can update the old scalar references.

          Alloca = dyn_cast<AllocaInst>(UserIt->getOperand(1));
          PRef->VectorParm = Alloca;
        } else {
          // Since Mem2Reg has run, there is no existing scalar store for
          // the parameter, but we must still pack (store) the expanded vector
          // parameter to a new vector alloca. This store is created here and
          // put in a container for later insertion. We cannot insert it here
          // since this will be a new user of the parameter and we are still
          // iterating over the original users of the parameter. This will
          // invalidate the iterator. We also map the parameter directly to the
          // vector bitcast so that we can later update any users of the
          // parameter.

          Value *ArgValue = dyn_cast<Value>(ArgIt);
          StoreInst *Store = new StoreInst(ArgValue, VecAlloca);
          StoresToInsert.push_back(Store);
          PRef->VectorParm = ArgValue;
        }

        PRef->VectorParmCast = VecParmCast;
        VectorParmMap.push_back(PRef);
      }
    }
  }

  // Insert any necessary vector parameter stores here. This is needed for when
  // there were no existing scalar stores that we can update to vector stores
  // for the parameter. This is needed when Mem2Reg has registerized parameters.
  // The stores are inserted after the allocas in the entry block.
  for (auto *Inst : StoresToInsert) {
    insertInstruction(Inst, EntryBlock);
  }

  return Mask;
}

Instruction* VecClone::createExpandedReturn(Function *Clone,
                                            BasicBlock *EntryBlock,
                                            VectorType *ReturnType)
{
  // Expand the return temp to a vector.

  Twine VarName = "vec.retval";
  VectorType *AllocaType = dyn_cast<VectorType>(Clone->getReturnType());

  AllocaInst *VecAlloca = new AllocaInst(AllocaType, VarName);
  insertInstruction(VecAlloca, EntryBlock);
  PointerType *ElemTypePtr =
      PointerType::get(ReturnType->getElementType(),
                       VecAlloca->getType()->getAddressSpace());

  BitCastInst *VecCast = new BitCastInst(VecAlloca, ElemTypePtr, "ret.cast");
  insertInstruction(VecCast, EntryBlock);

  return VecCast;
}

Instruction* VecClone::expandReturn(Function *Clone, BasicBlock *EntryBlock,
                                    BasicBlock *LoopBlock,
                                    BasicBlock *ReturnBlock,
                                    std::vector<ParmRef*>& VectorParmMap)
{
  // Determine how the return is currently handled, since this will determine
  // if a new vector alloca is required for it. For simple functions, an alloca
  // may not have been created for the return value. The function may just
  // simply return a value defined by some operation that now exists within the
  // loop. If an alloca was generated already, then the return block will load
  // from it and then return. Thus, we look for a return resulting from a load
  // in the return block. If found, we have already expanded all alloca
  // instructions to vector types and the old scalar references have already
  // been replaced with them. In this case, we only need to pack the results
  // from the vector alloca into a temp and return the temp. If a vector alloca
  // was not generated for the return, we need to add one for it because we have
  // a scalar reference in the loop that needs to be replaced. After creating
  // the new vector alloca, replace the reference to it in the loop and then
  // pack the results into a temp and return it.
  //
  // Example 1: // alloca not generated in entry block
  //
  // loop:
  //   ... // some set of instructions
  //   %add1 = add nsw i32 %1, %2
  //   br label %loop.exit (loop exit contains br to return block)
  //
  // return:
  //   ret i32 %add1
  //
  //
  // Example 2:
  //
  // loop:
  //  ... // some set of instructions
  //   %vecgep1 = getelementptr <2 x i32>* %vec_ret, i32 0, i32 %index
  //   store i32 %add2, i32* %vecgep1
  //   br label %loop.exit (loop exit contains br to return block)
  //
  // return:
  //   %7 = load i32, i32*, %retval // the original scalar alloca 
  //   ret i32 %7
  //

  ReturnInst *FuncReturn = dyn_cast<ReturnInst>(ReturnBlock->getTerminator());
  assert(FuncReturn && "Expected ret instruction to terminate the return\
                        basic block");

  LoadInst *LoadFromAlloca = dyn_cast<LoadInst>(FuncReturn->getOperand(0));

  // We need to generate a vector alloca for the return vector.
  // Two cases exist, here:
  // 
  // 1) For simple functions, the return is a temp defined within the
  //    loop body and the temp is not loaded from an alloca, or the return is
   //   a constant. (obviously, also not loaded from an alloca)
  // 
  // 2) The return temp traces back to an alloca.
  // 
  // For both cases, generate a vector alloca so that we can later load from it
  // and return the vector temp from the function. The alloca is used to load
  // and store from so that the scalar loop contains load/store/gep
  // instructions. This enables AVR construction to remain straightforward.
  // E.g., we don't need to worry about figuring out how to represent
  // insert/extract when building AVR nodes. This keeps consistent with how ICC
  // is operating.
  // 
  // Additionally, for case 1 we must generate a gep and store after the
  // instruction that defines the original return temp, so that we can store
  // the result into the proper index of the return vector. For case 2, we must
  // go into the loop and replace the old scalar alloca reference with the one
  // just created as vector.

  Instruction *VecReturn = NULL;
  VectorType *ReturnType = dyn_cast<VectorType>(Clone->getReturnType());

  if (!LoadFromAlloca) {

    // Case 1

    VecReturn = createExpandedReturn(Clone, EntryBlock, ReturnType);
    Value *RetVal = FuncReturn->getReturnValue();
    Instruction *RetFromTemp = dyn_cast<Instruction>(RetVal);

    Instruction *InsertPt;
    Value *ValToStore;
    Instruction *Phi = &*LoopBlock->begin();

    if (RetFromTemp) {
      // If we're returning from an SSA temp, set the insert point to the
      // definition of the temp.
      InsertPt = RetFromTemp;
      ValToStore = RetFromTemp;
    } else {
      // If we're returning a constant, then set the insert point to the loop
      // phi. From here, a store to the vector using the constant is inserted.
      InsertPt = Phi;
      ValToStore = RetVal;
    }

    // Generate a gep from the bitcast of the vector alloca used for the return
    // vector.
    Twine GepName = VecReturn->getName() + ".gep";
    GetElementPtrInst *VecGep =
        GetElementPtrInst::Create(ReturnType->getElementType(),
                                  VecReturn, Phi, GepName);
    VecGep->insertAfter(InsertPt);

    // Store the constant or temp to the appropriate lane in the return vector.
    StoreInst *VecStore = new StoreInst(ValToStore, VecGep);
    VecStore->insertAfter(VecGep);

  } else {

    // Case 2

    AllocaInst *Alloca = dyn_cast<AllocaInst>(LoadFromAlloca->getOperand(0));
    bool AllocaFound = false;
    unsigned ParmIdx = 0;

    for (; ParmIdx < VectorParmMap.size(); ParmIdx++) {
      Value *ParmVal = VectorParmMap[ParmIdx]->VectorParm;
      if (ParmVal == Alloca)
        AllocaFound = true;
    }

    if (AllocaFound) {
      // There's already a vector alloca created for the return, which is the
      // same one used for the parameter. E.g., we're returning the updated
      // parameter.
      VecReturn = VectorParmMap[ParmIdx]->VectorParmCast;
    } else {
      // A new return vector is needed because we do not load the return value
      // from an alloca.
      VecReturn = createExpandedReturn(Clone, EntryBlock, ReturnType);
      ParmRef *PRef = new ParmRef();
      PRef->VectorParm = Alloca;
      PRef->VectorParmCast = VecReturn;
      VectorParmMap.push_back(PRef);
    }
  }

  return VecReturn;
}

Instruction* VecClone::expandVectorParametersAndReturn(
    Function *Clone,
    VectorVariant &V,
    Instruction **Mask,
    BasicBlock *EntryBlock,
    BasicBlock *LoopBlock,
    BasicBlock *ReturnBlock,
    std::vector<ParmRef*>& VectorParmMap)
{
  // If there are no parameters, then this function will do nothing and this
  // is the expected behavior.
  *Mask = expandVectorParameters(Clone, V, EntryBlock, VectorParmMap);

  // If the function returns void, then don't attempt to expand to vector.
  Instruction *ExpandedReturn = ReturnBlock->getTerminator();
  if (!Clone->getReturnType()->isVoidTy()) {
    ExpandedReturn = expandReturn(Clone, EntryBlock, LoopBlock, ReturnBlock,
                                  VectorParmMap);
  }

  // So, essentially what has been done to this point is the creation and
  // insertion of the vector alloca instructions. Now, we insert the bitcasts of
  // those instructions, which have been stored in the map. The insertion of the
  // vector bitcast to element type pointer is done at the end of the EntryBlock
  // to ensure that any initial stores of vector parameters have been done
  // before the cast.

  std::vector<ParmRef*>::iterator MapIt;
  for (auto MapIt : VectorParmMap) {
    Instruction *ExpandedCast = MapIt->VectorParmCast;
    if (!ExpandedCast->getParent()) {
      insertInstruction(ExpandedCast, EntryBlock);
    }
  }

  // Insert the mask parameter store to alloca and bitcast if this is a masked
  // variant.
  if (*Mask) {
    // Mask points to the bitcast of the alloca instruction to element type
    // pointer. Insert the bitcast after all of the other bitcasts for vector
    // parameters.
    insertInstruction(*Mask, EntryBlock);

    Value *MaskVector = (*Mask)->getOperand(0);

    // MaskParm points to the function's mask parameter.
    Function::ArgumentListType &ArgList = Clone->getArgumentList();
    Function::ArgumentListType::iterator MaskParm = ArgList.end();
    MaskParm--;

    // Find the last parameter store in the function entry block and insert the
    // the store of the mask parameter after it. We do this just to make the
    // LLVM IR easier to read. If there are no parameters, just insert the store
    // before the terminator. For safety, if we cannot find a store, then insert
    // this store after the last alloca. At this point, there will at least be
    // an alloca for either a parameter or return. This code just ensures that
    // the EntryBlock instructions are grouped by alloca, followed by store,
    // followed by bitcast for readability reasons.

    StoreInst *MaskStore = new StoreInst(&*MaskParm, MaskVector);
    insertInstruction(MaskStore, EntryBlock);
  }

  DEBUG(dbgs() << "After Parameter/Return Expansion\n");
  DEBUG(Clone->dump());

  return ExpandedReturn;
}

bool VecClone::typesAreCompatibleForLoad(Type *GepType, Type *LoadType)
{
  // GepType will always be a pointer since this refers to an alloca for a
  // vector.
  PointerType *GepPtrTy = dyn_cast<PointerType>(GepType);
  Type *LoadFromTy = GepPtrTy->getElementType();
  Type *LoadToTy = LoadType;

  // Dereferencing pointers in LLVM IR means that we have to have a load for
  // each level of indirection. This means that we load from a gep and the
  // resulting load value type is reduced by one level of indirection. For
  // example, we load from a gep of i32* to a temp that has an i32 type. We
  // cannot do multiple levels of dereferencing in a single load. For example,
  // we cannot load from a gep of i32** to an i32. This requires two loads.
  //
  // Legal Case: GepType = i32**, LoadFromTy = i32*,
  //             LoadType = i32*, LoadToTy = i32*
  // 
  // %vec.b.elem.2 = load i32*, i32** %vec.b.cast.gep1
  //
  // In this case, since both are pointers, types will be considered equal by
  // LLVM, so we must continue getting the element types of each pointer type
  // until one is no longer a pointer type. Then do an equality check.
  //
  // Legal Case: GepType = i32*, LoadFromTy = i32,
  //             LoadType = i32, LoadToTy = i32
  //
  // %vec.b.elem.2 = load i32, i32* %vec.b.cast.gep1
  //
  // Ready to compare as is
  //
  // Illegal Case: GepType = i32**, LoadFromTy = i32*
  //               LoadType = i32, LoadToTy = i32
  //
  // %vec.b.elem.2 = load i32, i32** %vec.b.cast.gep1
  //
  // This case arises due to differences in the LLVM IR at -O0 and >= -O1.
  // For >= -O1, Mem2Reg registerizes parameters and there are no alloca
  // instructions created for function parameters. At -O0, vector parameters
  // are expanded and we modify the existing alloca that was used for the scalar
  // parameter. When there is no alloca for vector parameters, we must create
  // one for them. Thus, we have introduced an additional level of indirection
  // for users of parameters at >= -O1. This can become a problem for load
  // instructions and results in this illegal case. This function helps to
  // check that we are not attempting to do an extra level of indirection
  // within the load instructions for elements of vector parameters in the
  // simd loop. If an illegal case is encountered, an additional load is
  // inserted to account for the extra level of indirection and any users are
  // updated accordingly.

  while (LoadFromTy->getTypeID() == Type::PointerTyID &&
         LoadToTy->getTypeID()   == Type::PointerTyID) {

    PointerType *FromPtrTy = cast<PointerType>(LoadFromTy);
    PointerType *ToPtrTy = cast<PointerType>(LoadToTy);

    LoadFromTy = FromPtrTy->getElementType();
    LoadToTy = ToPtrTy->getElementType();
  }

  if (LoadFromTy->getTypeID() == LoadToTy->getTypeID()) {
    return true;
  }

  return false;
}

void VecClone::updateScalarMemRefsWithVector(
    Function *Clone,
    Function &F,
    BasicBlock *EntryBlock,
    BasicBlock *ReturnBlock,
    PHINode *Phi,
    std::vector<ParmRef*>& VectorParmMap)
{
  // This function replaces the old scalar uses of a parameter with a reference
  // to the new vector one. A gep is inserted using the vector bitcast created
  // in the entry block and any uses of the parameter are replaced with this
  // gep. The only users that will not be updated are those in the entry block
  // that do the initial store to the vector alloca of the parameter.

  std::vector<ParmRef*>::iterator VectorParmMapIt;

  for (auto VectorParmMapIt : VectorParmMap) {

    SmallVector<Instruction*, 4> InstsToUpdate;
    Value *Parm = VectorParmMapIt->VectorParm;
    Instruction *Cast = VectorParmMapIt->VectorParmCast;

    for (User *U : Parm->users()) {
      InstsToUpdate.push_back(dyn_cast<Instruction>(U));
    }

    for (unsigned I = 0; I < InstsToUpdate.size(); ++I) {

      Instruction *User = InstsToUpdate[I];
      if (!(dyn_cast<StoreInst>(User) && User->getParent() == EntryBlock)) {

        BitCastInst *BitCast = dyn_cast<BitCastInst>(Cast);
        PointerType *BitCastType = dyn_cast<PointerType>(BitCast->getType());
        Type *PointeeType = BitCastType->getElementType();

        Twine GepName = BitCast->getName() + ".gep";
        GetElementPtrInst *VecGep =
            GetElementPtrInst::Create(PointeeType, BitCast, Phi, GepName,
                                      User);

        unsigned NumOps = User->getNumOperands();
        for (unsigned I = 0; I < NumOps; ++I) {
          if (User->getOperand(I) == Parm) {

            bool TypesAreCompatible = false;

            if (isa<LoadInst>(User)) {
              TypesAreCompatible =
                  typesAreCompatibleForLoad(VecGep->getType(), User->getType());
            }

            if ((isa<LoadInst>(User) && TypesAreCompatible) ||
                isa<StoreInst>(User)) {
              // If the user is a load/store and the dereferencing is legal,
              // then just modify the load/store operand to use the gep.
              User->setOperand(I, VecGep);
            } else {
              // Otherwise, we need to load the value from the gep first before
              // using it. This effectively loads the particular element from
              // the vector parameter.
              Twine LoadName = "vec." + Parm->getName() + ".elem";
              LoadInst *ParmElemLoad = new LoadInst(VecGep, LoadName); 
              ParmElemLoad->insertAfter(VecGep);
              User->setOperand(I, ParmElemLoad);
            }
          }
        }
      } else {
        // The user is the parameter store to alloca in the entry block. Replace
        // the old scalar alloca with the new vector one.
        AllocaInst *VecAlloca = dyn_cast<AllocaInst>(Cast->getOperand(0));
        User->setOperand(1, VecAlloca);
      }
    }
  }

  DEBUG(dbgs() << "After Alloca Replacement\n");
  DEBUG(Clone->dump());
}

Instruction* VecClone::generateStrideForParameter(
    Function *Clone,
    Argument *Arg,
    Instruction *ParmUser,
    int Stride,
    PHINode *Phi)
{
  // Value returned as the last instruction needed to update the users of the
  // old parameter reference.
  Instruction *StrideInst = nullptr;

  Constant *StrideConst =
      ConstantInt::get(Type::getInt32Ty(Clone->getContext()), Stride);

  Instruction *Mul = BinaryOperator::CreateMul(StrideConst, Phi, "stride.mul");

  // Insert the stride related instructions after the user if the instruction
  // involves a redefinition of the parameter. For example, a load from the
  // parameter's associated alloca or a cast. For these situations, we want to
  // apply the stride to this SSA temp. For other instructions, e.g., add, the
  // instruction computing the stride must be inserted before the user.

  if (!isa<UnaryInstruction>(ParmUser)) {
    Mul->insertBefore(ParmUser);
  } else {
    Mul->insertAfter(ParmUser);
  }

  if (Arg->getType()->isPointerTy()) {

    // Linear updates to pointer parameters involves an address calculation, so
    // use gep. To properly update linear pointers we only need to multiply the
    // loop index and stride since gep is indexed starting at 0 from the base
    // address passed to the vector function.
    PointerType *ParmPtrType = dyn_cast<PointerType>(Arg->getType());

    // The base address used for linear gep computations.
    Value *BaseAddr = nullptr;
    StringRef RefName;

    if (LoadInst *ParmLoad = dyn_cast<LoadInst>(ParmUser)) {
      // We are loading from the alloca of the pointer parameter (no Mem2Reg)
      // i.e., loading a pointer to an SSA temp.
      BaseAddr = ParmUser;
      RefName = ParmLoad->getOperand(0)->getName();
    } else {
      // The user is using the pointer parameter directly.
      BaseAddr = Arg;
      RefName = BaseAddr->getName();
    }

    // Mul is always generated as i32 since it is calculated using the i32 loop
    // phi that is inserted by this pass. No cast on Mul is necessary because
    // gep can use a base address of one type with an index of another type.
    Twine GepName = RefName + ".gep";
    GetElementPtrInst *LinearParmGep =
        GetElementPtrInst::Create(ParmPtrType->getElementType(),
                                  BaseAddr, Mul, GepName);

    LinearParmGep->insertAfter(Mul);
    StrideInst = LinearParmGep;
  } else {
    // For linear values, a mul/add sequence is needed to generate the correct
    // value. i.e., val = linear_var * stride + loop_index;
    //
    // Also, Mul above is generated as i32 because the phi type is always i32.
    // However, ParmUser may be another integer type, so we must convert i32 to
    // i8/i16/i64 when the user is not i32.

    if (ParmUser->getType()->getIntegerBitWidth() !=
        Mul->getType()->getIntegerBitWidth()) {

      Instruction *MulConv =
              CastInst::CreateIntegerCast(Mul, ParmUser->getType(), true,
                                          "stride.cast");
      MulConv->insertAfter(Mul);
      Mul = MulConv;
    }

    // Generate the instruction that computes the stride.
    BinaryOperator *Add;
    StringRef TempName = "stride.add";
    if (isa<UnaryInstruction>(ParmUser)) {
      // The user of the parameter is an instruction that results in a
      // redefinition of it. e.g., a load from an alloca (no Mem2Reg) or a cast
      // instruction. In either case, the stride needs to be applied to this
      // temp.
      Add = BinaryOperator::CreateAdd(ParmUser, Mul, TempName);
    } else {
      // Otherwise, the user is an instruction that does not redefine the temp,
      // such as an add instruction. For these cases, the stride must be
      // computed before the user and the reference to the parameter must be
      // replaced with this instruction.
      Add = BinaryOperator::CreateAdd(Arg, Mul, TempName);
    }

    Add->insertAfter(Mul);
    StrideInst = Add;
  }

  return StrideInst;
}

void VecClone::updateLinearReferences(Function *Clone, Function &F,
                                      VectorVariant &V, PHINode *Phi)
{
  // Add stride to parameters marked as linear. This is done by finding all
  // users of the scalar alloca associated with the parameter. The user should
  // be a load from this alloca to a temp. The stride is then added to this temp
  // and its uses are replaced with the new temp. Or, if Mem2Reg eliminates the
  // alloca/load, the parameter is used directly and this use is updated with
  // the stride.

  Function::ArgumentListType &ArgList = Clone->getArgumentList();
  Function::ArgumentListType::iterator ArgListIt = ArgList.begin();
  Function::ArgumentListType::iterator ArgListEnd = ArgList.end();
  std::vector<VectorKind> ParmKinds = V.getParameters();

  for (; ArgListIt != ArgListEnd; ++ArgListIt) {

    User::user_iterator ArgUserIt = ArgListIt->user_begin();
    User::user_iterator ArgUserEnd = ArgListIt->user_end();
    unsigned ParmIdx = ArgListIt->getArgNo();
    SmallVector<Instruction*, 4> LinearParmUsers;

    if (ParmKinds[ParmIdx].isLinear()) {

      int Stride = ParmKinds[ParmIdx].getStride();

      for (; ArgUserIt != ArgUserEnd; ++ArgUserIt) {

        // Collect all uses of the parameter so that they can later be used to
        // apply the stride.
        Instruction *ParmUser = dyn_cast<Instruction>(*ArgUserIt);
        if (StoreInst *ParmStore = dyn_cast<StoreInst>(ParmUser)) {

          // This code traces the store of the parameter to its associated
          // alloca. Then, we look for a load from that alloca to a temp. This
          // is the value we need to add the stride to. This is for when
          // Mem2Reg has not been run.
          AllocaInst *Alloca = dyn_cast<AllocaInst>(ArgUserIt->getOperand(1));

          if (Alloca) {
            for (auto *AU : Alloca->users()) {

              LoadInst *ParmLoad = dyn_cast<LoadInst>(AU);

              if (ParmLoad) {
                // The parameter is being loaded from an alloca to a new SSA
                // temp. We must replace the users of this load with an
                // instruction that adds the result of this load with the
                // stride.
                LinearParmUsers.push_back(ParmLoad);
              }
            }
          } else {
            // Mem2Reg has run, so the parameter is directly referenced in the
            // store instruction.
            LinearParmUsers.push_back(ParmStore);
          }
        } else {
          // Mem2Reg has registerized the parameters, so users of it will use
          // it directly, and not through a load of the parameter.
          LinearParmUsers.push_back(ParmUser);
        }

        for (unsigned I = 0; I < LinearParmUsers.size(); I++) {
          // For each user of parameter:

          // We must deal with two cases here, based on whether Mem2Reg has been
          // run.
          //
          // Example:
          //
          // __declspec(vector(linear(i:1),uniform(x),vectorlength(4)))
          // extern int foo(int i, int x) {
          //   return (x + i);
          // }
          //
          // 1) We are loading the parameter from an alloca and the SSA temp as
          //    as a result of the load is what we need to add the stride to.
          //    Then, any users of that temp must be replaced. The only load
          //    instructions put in the collection above are guaranteed to be
          //    associated with the parameter's alloca. Thus, we only need to
          //    check to see if a load is in the map to know what to do.
          //
          // Before Linear Update:
          //
          // simd.loop:                     ; preds = %simd.loop.exit, %entry
          //   %index = phi i32 [ 0, %entry ], [ %indvar, %simd.loop.exit ]
          //   store i32 %x, i32* %x.addr, align 4
          //   %0 = load i32, i32* %x.addr, align 4
          //   %1 = load i32, i32* %i.addr, align 4 <--- %i
          //   %add = add nsw i32 %0, %1            <--- replace %1 with stride
          //   %ret.cast.gep = getelementptr i32, i32* %ret.cast, i32 %index
          //   store i32 %add, i32* %ret.cast.gep
          //   br label %simd.loop.exit
          //
          // After Linear Update:
          //
          // simd.loop:                     ; preds = %simd.loop.exit, %entry
          //   %index = phi i32 [ 0, %entry ], [ %indvar, %simd.loop.exit ]
          //   store i32 %x, i32* %x.addr, align 4
          //   %0 = load i32, i32* %x.addr, align 4
          //   %1 = load i32, i32* %i.addr, align 4
          //   %stride.mul = mul i32 1, %index
          //   %stride.add = add i32 %1, %stride.mul <--- stride
          //   %add = add nsw i32 %0, %stride.add    <--- new %i with stride
          //   %ret.cast.gep = getelementptr i32, i32* %ret.cast, i32 %index
          //   store i32 %add, i32* %ret.cast.gep
          //   br label %simd.loop.exit
          //
          // 2) The user uses the parameter directly, and so we must apply the
          //    stride directly to the parameter. Any users of the parameter
          //    must then be updated.
          //
          // Before Linear Update:
          //
          // simd.loop:                     ; preds = %simd.loop.exit, %entry
          //   %index = phi i32 [ 0, %entry ], [ %indvar, %simd.loop.exit ]
          //   %add = add nsw i32 %x, %i <-- direct usage of %i
          //   %ret.cast.gep = getelementptr i32, i32* %ret.cast, i32 %index
          //   store i32 %add, i32* %ret.cast.gep
          //   br label %simd.loop.exit
          //
          // After Linear Update:
          //
          // simd.loop:                     ; preds = %simd.loop.exit, %entry
          //   %index = phi i32 [ 0, %entry ], [ %indvar, %simd.loop.exit ]
          //   %stride.mul = mul i32 1, %index
          //   %stride.add = add i32 %i, %stride.mul <--- stride
          //   %add = add nsw i32 %x, %stride.add    <--- new %i with stride
          //   %ret.cast.gep = getelementptr i32, i32* %ret.cast, i32 %index
          //   store i32 %add, i32* %ret.cast.gep
          //   br label %simd.loop.exit

          Instruction *StrideInst =
              generateStrideForParameter(Clone, &*ArgListIt, LinearParmUsers[I],
                                         Stride, Phi);

          SmallVector<Instruction*, 4> InstsToUpdate;
          Value *ParmUser;

          if (isa<UnaryInstruction>(LinearParmUsers[I])) {
            // Case 1
            ParmUser = LinearParmUsers[I];
            User::user_iterator StrideUserIt = LinearParmUsers[I]->user_begin();
            User::user_iterator StrideUserEnd = LinearParmUsers[I]->user_end();

            // Find the users of the redefinition of the parameter so that we
            // can apply the stride to those instructions.
            for (; StrideUserIt != StrideUserEnd; ++StrideUserIt) {

              Instruction *StrideUser = dyn_cast<Instruction>(*StrideUserIt);
              if (StrideUser != StrideInst) {
                // We've already inserted the stride which is now also a user of
                // the parameter, so don't update that instruction. Otherwise,
                // we'll create a self reference. Hence, why we don't use
                // replaceAllUsesWith().
                InstsToUpdate.push_back(StrideUser);
              }
            }
          } else {
            // Case 2
            ParmUser = &*ArgListIt;
            InstsToUpdate.push_back(LinearParmUsers[I]);
          }

          // Replace the old references to the parameter with the instruction
          // that applies the stride.
          for (unsigned J = 0; J < InstsToUpdate.size(); ++J) {
            unsigned NumOps = InstsToUpdate[J]->getNumOperands();
            for (unsigned K = 0; K < NumOps; ++K) {
              if (InstsToUpdate[J]->getOperand(K) == ParmUser) {
                InstsToUpdate[J]->setOperand(K, StrideInst);
              }
            }
          }
        }
      }
    }
  }

  DEBUG(dbgs() << "After Linear Updates\n");
  DEBUG(Clone->dump());
}

void VecClone::updateReturnBlockInstructions(
    Function *Clone,
    BasicBlock *ReturnBlock,
    Instruction *ExpandedReturn)
{
  // If the vector function returns void, then there is no need to do any
  // packing. The only instruction in the ReturnBlock is 'ret void', so
  // we can just leave this instruction and we're done.
  if (Clone->getReturnType()->isVoidTy()) return;

  // Collect all instructions in the return basic block. They will be removed.
  SmallVector<Instruction*, 4> InstToRemove;
  BasicBlock::iterator InstIt = ReturnBlock->begin();
  BasicBlock::iterator InstEnd = ReturnBlock->end();

  for (; InstIt != InstEnd; ++InstIt) {
    InstToRemove.push_back(&*InstIt);
  }

  // Remove all instructions from the return block. These will be replaced
  // with the instructions necessary to return a vector temp. The verifier
  // will complain if we remove the definitions of users first, so remove
  // instructions from the bottom up.
  for (int I = InstToRemove.size() - 1; I >= 0; I--) {
    InstToRemove[I]->eraseFromParent();
  }

  // Pack up the elements into a vector temp and return it. If the return
  // vector was bitcast to a pointer to the element type, we must bitcast to
  // vector before returning.
  Instruction *Return;
  if (dyn_cast<BitCastInst>(ExpandedReturn)) {
      // Operand 0 is the actual alloc reference in the bitcast.
      AllocaInst *Alloca = dyn_cast<AllocaInst>(ExpandedReturn->getOperand(0));
      PointerType *PtrVecType =
          PointerType::get(Clone->getReturnType(),
                           Alloca->getType()->getAddressSpace());
      Twine CastName = "vec." + ExpandedReturn->getName();
      BitCastInst *BitCast = new BitCastInst(ExpandedReturn, PtrVecType,
                                             CastName, ReturnBlock);
      Return = BitCast;
  } else {
      Return = ExpandedReturn;
  }

  LoadInst *VecReturn = new LoadInst(Return, "vec.ret", ReturnBlock);
  ReturnInst::Create(Clone->getContext(), VecReturn, ReturnBlock);

  DEBUG(dbgs() << "After Return Block Update\n");
  DEBUG(Clone->dump());
}

int VecClone::getParmIndexInFunction(Function *F, Value *Parm)
{
  Function::arg_iterator ArgIt = F->arg_begin();
  Function::arg_iterator ArgEnd = F->arg_end();
  for (unsigned Idx = 0; ArgIt != ArgEnd; ++ArgIt, ++Idx) {
    if (Parm == &*ArgIt) return Idx; 
  }

  return -1;
}

bool VecClone::isSimpleFunction(Function *Clone, VectorVariant &V,
                                ReturnInst *ReturnOnly)
{
  // For really simple functions, there is no need to go through the process
  // of inserting a loop.

  // Example:
  //
  // void foo(void) {
  //   return;
  // }
  // 
  // No need to insert a loop for this case since it's basically a no-op. Just
  // clone the function and return. It's possible that we could have some code
  // inside of a vector function that modifies global memory. Let that case go
  // through.
  if (ReturnOnly && Clone->getReturnType()->isVoidTy()) {
    return true;
  }

  return false;
}

void VecClone::insertSplitForMaskedVariant(Function *Clone,
                                           BasicBlock *LoopBlock,
                                           BasicBlock *LoopExitBlock,
                                           Instruction *Mask, PHINode *Phi)
{
  BasicBlock *LoopThenBlock =
      LoopBlock->splitBasicBlock(LoopBlock->getFirstNonPHI(),
                                 "simd.loop.then");

  BasicBlock *LoopElseBlock = BasicBlock::Create(Clone->getContext(),
                                                 "simd.loop.else",
                                                 Clone, LoopExitBlock);

  BranchInst::Create(LoopExitBlock, LoopElseBlock);

  BitCastInst *BitCast = dyn_cast<BitCastInst>(Mask);
  PointerType *BitCastType = dyn_cast<PointerType>(BitCast->getType());
  Type *PointeeType = BitCastType->getElementType();

  GetElementPtrInst *MaskGep =
      GetElementPtrInst::Create(PointeeType, Mask, Phi, "mask.gep",
                                LoopBlock->getTerminator());

  Twine LoadName = "mask.parm";
  LoadInst *MaskLoad = new LoadInst(MaskGep, LoadName,
                                    LoopBlock->getTerminator());

  Type *CompareTy = MaskLoad->getType();
  Instruction *MaskCmp;
  Constant* Zero;

  // Generate the compare instruction to see if the mask bit is on. In ICC, we
  // use the movemask intrinsic which takes both float/int mask registers and
  // converts to an integer scalar value, one bit representing each element.
  // AVR construction will be complicated if this intrinsic is introduced here,
  // so the current solution is to just generate either an integer or floating
  // point compare instruction for now. This may change anyway if we decide to
  // go to a vector of i1 values for the mask. I suppose this would be one
  // positive reason to use vector of i1.
  if (CompareTy->isIntegerTy()) {
    Zero = getConstantValue(CompareTy, Clone->getContext(), 0);
    MaskCmp = new ICmpInst(LoopBlock->getTerminator(), CmpInst::ICMP_NE,
                           MaskLoad, Zero, "mask.cond");
  } else if (CompareTy->isFloatingPointTy()) {
    Zero = getConstantValue(CompareTy, Clone->getContext(), 0.0);
    MaskCmp = new FCmpInst(LoopBlock->getTerminator(), CmpInst::FCMP_UNE,
                           MaskLoad, Zero, "mask.cond");
  } else {
    assert(0 && "Unsupported mask compare");
  }

  TerminatorInst *Term = LoopBlock->getTerminator();
  Term->eraseFromParent();
  BranchInst::Create(LoopThenBlock, LoopElseBlock, MaskCmp, LoopBlock);

  DEBUG(dbgs() << "After Split Insertion For Masked Variant\n");
  DEBUG(Clone->dump());
}

void VecClone::removeScalarAllocasForVectorParams(
    std::vector<ParmRef*> &VectorParmMap)
{
  std::vector<ParmRef*>::iterator VectorParmMapIt;

  for (auto VectorParmMapIt : VectorParmMap) {
    Value *Parm = VectorParmMapIt->VectorParm;
    if (AllocaInst *ScalarAlloca = dyn_cast<AllocaInst>(Parm)) {
      ScalarAlloca->eraseFromParent();
    }
  }
}

void VecClone::disableLoopUnrolling(BasicBlock *Latch)
{
  // Set disable unroll metadata on the conditional branch of the loop latch
  // for the simd loop. The following is an example of what the loop latch
  // and Metadata will look like. The !llvm.loop marks the beginning of the
  // loop Metadata and is always placed on the terminator of the loop latch.
  // (i.e., simd.loop.exit in this case). According to LLVM documentation, to
  // properly set the loop Metadata, the 1st operand of !16 must be a self-
  // reference to avoid some type of Metadata merging conflicts that have
  // apparently arisen in the past. This is part of LLVM history that I do not
  // know. Also, according to LLVM documentation, any Metadata nodes referring
  // to themselves are marked as distinct. As such, all Metadata corresponding
  // to a loop belongs to that loop alone and no sharing of Metadata can be
  // done across different loops.
  //
  // simd.loop.exit:        ; preds = %simd.loop, %if.else, %if.then
  //  %indvar = add nuw i32 %index, 1
  //  %vl.cond = icmp ult i32 %indvar, 2
  //  br i1 %vl.cond, label %simd.loop, label %simd.end.region, !llvm.loop !16
  //
  // !16 = distinct !{!16, !17}
  // !17 = !{!"llvm.loop.unroll.disable"}

  SmallVector<Metadata *, 4> MDs;

  // Reserve first location for self reference to the LoopID metadata node.
  MDs.push_back(nullptr);

  // Add unroll(disable) metadata to disable future unrolling.
  LLVMContext &Context = Latch->getContext();
  SmallVector<Metadata *, 1> DisableOperands;
  DisableOperands.push_back(MDString::get(Context, "llvm.loop.unroll.disable"));
  MDNode *DisableNode = MDNode::get(Context, DisableOperands);
  MDs.push_back(DisableNode);

  MDNode *NewLoopID = MDNode::get(Context, MDs);
  // Set operand 0 to refer to the loop id itself.
  NewLoopID->replaceOperandWith(0, NewLoopID);
  Latch->getTerminator()->setMetadata("llvm.loop", NewLoopID);
}

bool VecClone::runOnModule(Module &M) {

  DEBUG(dbgs() << "\nExecuting SIMD Function Cloning ...\n\n");

  FunctionVariants FunctionsToVectorize;
  getFunctionsToVectorize(M, FunctionsToVectorize);
  if (FunctionsToVectorize.empty()) {
    // No vector variants for this function exist.
    return false;
  }

  for (auto& pair : FunctionsToVectorize) {

    Function& F = *pair.first;
    DeclaredVariants &DeclaredVariants = pair.second;

    for (auto& DeclaredVariant : DeclaredVariants) {

      VectorVariant Variant(DeclaredVariant);

      // Clone the original function.
      DEBUG(dbgs() << "Before SIMD Function Cloning\n");
      DEBUG(F.dump());
      Function *Clone = cloneFunction(F, Variant);
      Function::iterator EntryBlock = Clone->begin();
      BasicBlock::iterator FirstInst = EntryBlock->begin();
      ReturnInst *ReturnOnly = dyn_cast<ReturnInst>(FirstInst);

      if (isSimpleFunction(Clone, Variant, ReturnOnly)) {
        continue;
      }

      BasicBlock *LoopBlock = splitEntryIntoLoop(Clone, Variant, &*EntryBlock);
      BasicBlock *ReturnBlock = splitLoopIntoReturn(Clone, LoopBlock);
      BasicBlock *LoopExitBlock = createLoopExit(Clone, ReturnBlock);
      PHINode *Phi = createPhiAndBackedgeForLoop(Clone, &*EntryBlock,
                                                 LoopBlock, LoopExitBlock,
                                                 ReturnBlock,
                                                 Variant.getVlen());

      // At this point, we've gathered some parameter information and have
      // restructured the function into an entry block, a set of blocks
      // forming the loop, a loop exit block, and a return block. Now,
      // we can go through and update instructions since we know what
      // is part of the loop.

      // VectorParmMap contains the mapping of the parameter to the bitcast
      // instruction that casts the vector alloca for vector parameters
      // to a scalar pointer for use in the simd loop. When parameters are
      // registerized, the Value* in the map correponds directly to the
      // function parameter. When parameters are not registerized, then the
      // Value* in the map is the original scalar alloca before expansion.
      // Later, users of the parameter, either directly or through the alloca,
      // are replaced with a gep using the bitcast of the vector alloca for the
      // parameter and the current loop induction variable value.
      //
      // IMPORTANT NOTE: std::vector was used here because later we emit LLVM
      // instructions using the members of ParmRef, and these instructions
      // should be ordered consistently for easier testability.

      std::vector<ParmRef*> VectorParmMap;

      // Create a new vector alloca instruction for all vector parameters and
      // return. For parameters, replace the initial store to the old alloca
      // with the vector one. Users of the old alloca within the loop will be
      // replaced with a gep using this address along with the proper loop
      // index.

      Instruction *Mask = NULL;
      Instruction *ExpandedReturn =
          expandVectorParametersAndReturn(Clone, Variant, &Mask, &*EntryBlock,
                                          LoopBlock, ReturnBlock,
                                          VectorParmMap);
      updateScalarMemRefsWithVector(Clone, F, &*EntryBlock, ReturnBlock, Phi,
                                    VectorParmMap);

      // Update any linear variables with the appropriate stride. This function
      // will insert a mul/add sequence before the use of the parameter. For
      // linear pointer parameters, the stride calculation is just a mul
      // instruction using the loop induction var and the stride value on the
      // parameter. This mul instruction is then used as the index of the gep
      // that will be inserted before the next use of the parameter. The
      // function also updates the users of the parameter with the new
      // calculation involving the stride.
      updateLinearReferences(Clone, F, Variant, Phi);

      // Remove the old scalar instructions associated with the return and
      // replace with packing instructions.
      updateReturnBlockInstructions(Clone, ReturnBlock, ExpandedReturn);

      // Remove the old scalar allocas associated with vector parameters since
      // these have now been replaced with vector ones.
      removeScalarAllocasForVectorParams(VectorParmMap);
      VectorParmMap.clear();

      // If this is the masked vector variant, insert the mask condition and
      // if/else blocks.
      if (Variant.isMasked()) {
        insertSplitForMaskedVariant(Clone, LoopBlock, LoopExitBlock, Mask, Phi);
      }

      DEBUG(dbgs() << "After SIMD Function Cloning\n");
      DEBUG(Clone->dump());

      // Disable unrolling from kicking in on the simd loop.
      disableLoopUnrolling(LoopExitBlock);

    } // End of function cloning for the variant
  } // End of function cloning for all variants

  return true; // LLVM IR has been modified
}

void VecClone::print(raw_ostream &OS, const Module *M) const {
  // TODO
}

ModulePass *llvm::createVecClonePass() {
  return new llvm::VecClone();
}

char VecClone::ID = 0;

static const char lv_name[] = "VecClone";
INITIALIZE_PASS_BEGIN(VecClone, SV_NAME, lv_name,
                      false /* modifies CFG */, false /* transform pass */)
INITIALIZE_PASS_END(VecClone, SV_NAME, lv_name,
                    false /* modififies CFG */, false /* transform pass */)
