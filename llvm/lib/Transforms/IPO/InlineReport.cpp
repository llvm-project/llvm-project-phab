//===- InlineReport.cpp - Inline report ------------ ---------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the mechanics of the inlining report.
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/IPO/InlineReport.h"
#include "llvm/Transforms/IPO/InlinerPass.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;
using namespace InlineReportTypes;

///
/// \brief Inlining report level option
///
/// Specified with -inline-report=N
///   N is a bit mask with the following interpretation of the bits
///    0: No inlining report
///    1: Simple inlining report
///    2: Add inlining reasons (to be implemented) 
///    4: Put the inlining reasons on the same line as the call sites
///    8: Print the line and column info for each call site if available
///   16: Print the file for each call site
///   32: Print linkage info for each function and call site
///
cl::opt<unsigned>
InlineReportLevel("inline-report", cl::Hidden, cl::init(0),
  cl::Optional, cl::desc("Print inline report"));

//
// The functions below implement the printing of the inlining report 
//

//
// Member functions for class InlineReportCallSite
//

InlineReportCallSite::~InlineReportCallSite(void) {
  while (!Children.empty()) {
    InlineReportCallSite* cs = Children.back();
    Children.pop_back();
    delete cs;
  }
}

InlineReportCallSite* InlineReportCallSite::copyBase(
  const InlineReportCallSite& Base, Instruction* NI)
{
  InlineReportCallSite* NewCS = new InlineReportCallSite(Base.IRCallee, 
    Base.IsInlined, Base.M, nullptr, NI); 
  NewCS->IsInlined = Base.IsInlined; 
  NewCS->InlineCost = Base.InlineCost; 
  NewCS->OuterInlineCost = Base.OuterInlineCost; 
  NewCS->InlineThreshold = Base.InlineThreshold; 
  NewCS->Line = Base.Line; 
  NewCS->Col = Base.Col; 
  NewCS->Children.clear(); 
  return NewCS; 
} 

InlineReportCallSite* InlineReportCallSite::cloneBase(
  const ValueToValueMapTy& IIMap) { 
  if (IsInlined) { 
    InlineReportCallSite* IRCSk = copyBase(*this, nullptr); 
    return IRCSk; 
  } 
  const Value* oldCall = this->getCall(); 
  if (oldCall == nullptr) { 
    return nullptr; 
  } 
  ValueToValueMapTy::const_iterator VMI = IIMap.find(oldCall);
  if (VMI == IIMap.end()) { 
    return nullptr; 
  } 
  WeakVH newCall = VMI->second; 
  Instruction* NI = cast<Instruction>(newCall);
  InlineReportCallSite* IRCSk = copyBase(*this, NI); 
  return IRCSk; 
} 

///
/// \brief Print 'indentCount' indentations 
///
static void printIndentCount(unsigned indentCount) {
  for (unsigned J = 0; J < indentCount; ++J) {
    llvm::errs() << "   "; 
  } 
} 

///
/// \brief Print the linkage info for a function 'F' as a single letter,
/// if the 'Level' specifies InlineReportOptions::Linkage. 
/// For an explanation of the meaning of these letters, see InlineReport.h. 
///
static void printFunctionLinkage(unsigned Level, InlineReportFunction* IRF)
{
  if (!(Level & InlineReportOptions::Linkage)) { 
    return;
  } 
  llvm::errs() << IRF->getLinkageChar() << " "; 
} 

///
/// \brief Print the callee name, and if non-zero, the line and column 
/// number of the call site 
///
void InlineReportCallSite::printCalleeNameModuleLineCol(unsigned Level) 
{
  if (getIRCallee() != nullptr) { 
    printFunctionLinkage(Level, getIRCallee()); 
    llvm::errs() << getIRCallee()->getName(); 
  } 
  if (Level & InlineReportOptions::File) { 
    llvm::errs() << " " << M->getModuleIdentifier();   
  }  
  if ((Level & InlineReportOptions::LineCol) && (Line != 0 || Col != 0)) { 
    llvm::errs() << " (" << Line << "," << Col << ")"; 
  } 
} 

///
/// \brief Print a representation of the inlining instance.
///
/// indentCount: The number of indentations to print
/// level: The level N from '-inline-report=N'
///
void InlineReportCallSite::print(unsigned IndentCount, unsigned Level) {
  printIndentCount(IndentCount);
  if (getIsInlined()) { 
    llvm::errs() << "-> INLINE: "; 
    printCalleeNameModuleLineCol(Level); 
  } 
  else { 
    llvm::errs() << "-> "; 
    printCalleeNameModuleLineCol(Level);
  } 
  llvm::errs() << "\n";
} 

//
// Member functions for class InlineReportFunction
//

InlineReportFunction::~InlineReportFunction(void) {
  while (!CallSites.empty()) {
    InlineReportCallSite* CS = CallSites.back();
    CallSites.pop_back();
    delete CS;
  }
}

void InlineReportFunction::setLinkageChar(Function *F) { 
  LinkageChar = (F->hasLocalLinkage() ? 'L' :
    (F->hasLinkOnceODRLinkage() ? 'O' :
    (F->hasAvailableExternallyLinkage() ? 'X' : 'A')));
} 

// 
// Member functions for class InlineReport
//

InlineReportFunction* InlineReport::addFunction(Function* F, Module* M) {
  if (Level == 0) { 
    return nullptr; 
  } 
  if (F == nullptr) { 
    return nullptr; 
  } 
  InlineReportFunctionMap::const_iterator MapIt = IRFunctionMap.find(F); 
  if (MapIt != IRFunctionMap.end()) { 
    InlineReportFunction* IRF = MapIt->second; 
    makeCurrent(M, F); 
    return IRF; 
  } 
  InlineReportFunction* IRF = new InlineReportFunction(F);
  IRFunctionMap.insert(std::make_pair(F, IRF)); 
  IRF->setName(F->getName().str()); 
  IRF->setIsDeclaration(F->isDeclaration()); 
  IRF->setLinkageChar(F); 
  addCallback(F); 
  return IRF; 
} 

InlineReportCallSite* InlineReport::addCallSite(Function* F, CallSite* CS, 
  Module* M) {
  if (Level == 0) { 
     return nullptr; 
  }
  if (F == nullptr) { 
     return nullptr; 
  } 
  Instruction *I = CS->getInstruction(); 
  DebugLoc DLoc = CS->getInstruction()->getDebugLoc();
  InlineReportFunctionMap::const_iterator MapIt = IRFunctionMap.find(F);
  assert(MapIt != IRFunctionMap.end()); 
  InlineReportFunction* IRF = MapIt->second;  
  Function* Callee = CS->getCalledFunction(); 
  InlineReportFunction* IRFC = nullptr; 
  if (Callee != nullptr) { 
    InlineReportFunctionMap::const_iterator MapItC 
      = IRFunctionMap.find(Callee);
    IRFC = MapItC == IRFunctionMap.end() 
        ? addFunction(Callee, M) : MapItC->second;
  } 
  InlineReportCallSite* IRCS = new InlineReportCallSite(IRFC, false, 
    M, &DLoc, I); 
  IRF->addCallSite(IRCS); 
  IRInstructionCallSiteMap.insert(std::make_pair(I, IRCS)); 
  addCallback(I); 
  return IRCS;
} 

InlineReportCallSite* InlineReport::addNewCallSite(Function* F, CallSite* CS, 
  Module* M) {
  if (Level == 0) { 
     return nullptr; 
  }
  InlineReportCallSite* IRCS = getCallSite(CS);
  if (IRCS != nullptr) 
    return IRCS; 
  return addCallSite(F, CS, M); 
} 

void InlineReport::setDead(Function* F) { 
  if (Level == 0) {
    return; 
  }
  InlineReportFunctionMap::const_iterator MapIt = IRFunctionMap.find(F);
  assert(MapIt != IRFunctionMap.end());
  InlineReportFunction* INR = MapIt->second; 
  INR->setDead(true); 
} 

void InlineReport::cloneChildren(
  const InlineReportCallSiteVector& OldCallSiteVector, 
  InlineReportCallSite* NewCallSite, ValueToValueMapTy& IIMap)
{
  assert(NewCallSite->getChildren().empty()); 
  for (unsigned I = 0, E = OldCallSiteVector.size(); I < E; ++I) { 
    //
    // Copy the old InlineReportCallSite and add it to the children of the 
    // cloned InlineReportCallSite.
    InlineReportCallSite* IRCSj = OldCallSiteVector[I]; 
    InlineReportCallSite* IRCSk = IRCSj->cloneBase(IIMap); 
    if (IRCSk == nullptr) { 
      continue; 
    } 
    NewCallSite->addChild(IRCSk); 
    //
    // We keep track of the new calls that are added added to the inline 
    // report in case they themselves will be inlined. 
    if (IRCSk->getCall() != nullptr) { 
      IRInstructionCallSiteMap.insert(std::make_pair(IRCSk->getCall(), IRCSk)); 
      addCallback(IRCSk->getCall()); 
    } 
    //
    // Recursively copy the InlineReportCallSites for the children. 
    if (IRCSj->getIsInlined()) { 
      cloneChildren(IRCSj->getChildren(), IRCSk, IIMap); 
    } 
  } 
} 

void InlineReport::inlineCallSite(Instruction* NI, InlineReportCallSite* IRCS, 
  Module* M, Function* Callee, InlineFunctionInfo& InlineInfo) { 
  if (Level == 0) { 
    return; 
  } 
  //
  // Get the inline report for the routine being inlined.  We are going 
  // to make a clone of it.
  InlineReportFunctionMap::const_iterator MapItF = IRFunctionMap.find(Callee);
  InlineReportFunction* INR = addFunction(Callee, M);
  //
  // Ensure that the report is up to date since the last call to 
  // Inliner::runOnSCC
  makeCurrent(M, Callee); 
  //
  // Create InlineReportCallSites "new calls" which appear in the inlined 
  // code.  Also, create a mapping from the "original calls" which appeared 
  // in the routine that was inlined, to the "new calls". When we clone the 
  // inline report for the routine being inlined, we need to replace the 
  // original calls with the new calls in the cloned inline report.  
  // We use 'IIMap' to do that mapping.  
  ValueToValueMapTy IIMap; 
  SmallVector<const Value*, 8>& OriginalCalls = InlineInfo.OriginalCalls; 
  SmallVector<WeakVH, 8>& NewCalls = InlineInfo.InlinedCalls; 
  for (unsigned I = 0, E = OriginalCalls.size(); I < E; ++I) { 
    IIMap.insert(std::make_pair(OriginalCalls[I], NewCalls[I])); 
  } 
  //
  // Clone the inline report INR and attach it to the inlined call site IRCS.
  // Use IIMap to map the original calls to the new calls in the cloned 
  // inline report. 
  cloneChildren(INR->getCallSites(), IRCS, IIMap); 
  // Indicate that the call has been inlined in the inline report 
  IRCS->setIsInlined(true);
  //
  // Remove the inlined instruction from the IRInstructionCallSiteMap
  InlineReportInstructionCallSiteMap::const_iterator MapIt;
  MapIt = IRInstructionCallSiteMap.find(NI);
  assert(MapIt != IRInstructionCallSiteMap.end());
  IRInstructionCallSiteMap.erase(MapIt);
  IRCS->setCall(nullptr); 
} 

void InlineReport::printOptionValues(void) const {
  llvm::errs() << "Option Values:\n"; 
  llvm::errs() << "  inline-threshold: " 
    << llvm::getDefaultInlineThreshold() << "\n";    
  llvm::errs() << "\n"; 
} 

///
/// \brief Print the callsites in the 'Vector' 
///
/// indentCount: The number of indentations to print
/// level: The level N from '-inline-report=N'
///
static void printInlineReportCallSiteVector(
  const InlineReportCallSiteVector& Vector,
  unsigned IndentCount, unsigned Level) {
  for (unsigned I = 0, E = Vector.size(); I < E; ++I) {
    Vector[I]->print(IndentCount, Level); 
    printInlineReportCallSiteVector(Vector[I]->getChildren(), IndentCount + 1,
      Level);
  } 
} 

void InlineReportFunction::print(unsigned Level) const { 
  if (Level == 0) { 
    return;
  } 
  printInlineReportCallSiteVector(CallSites, 1, Level); 
} 

void InlineReport::print(void) const {
  if (Level == 0) { 
    return;
  } 
  llvm::errs() << "---- Begin Inlining Report ----\n"; 
  printOptionValues();  
  for (unsigned I = 0, E = IRDeadFunctionVector.size(); I < E; ++I) { 
    InlineReportFunction* IRF = IRDeadFunctionVector[I];
    llvm::errs() << "DEAD STATIC FUNC: ";
    printFunctionLinkage(Level, IRF);
    llvm::errs() << IRF->getName() << "\n\n";
  } 
  InlineReportFunctionMap::const_iterator Mit, E; 
  for (Mit = IRFunctionMap.begin(), E = IRFunctionMap.end(); Mit != E; ++Mit) { 
    Function* F = Mit->first;
    // Update the linkage info one last time before printing, 
    // as it may have changed.
    InlineReportFunction* IRF = Mit->second;
    IRF->setLinkageChar(F); 
    if (!IRF->getIsDeclaration()) {
      llvm::errs() << "COMPILE FUNC: ";
      printFunctionLinkage(Level, IRF); 
      llvm::errs() << IRF->getName() << "\n";
      InlineReportFunction* IRF = Mit->second;
      IRF->print(Level); 
      llvm::errs() << "\n"; 
    } 
  } 
  llvm::errs() << "---- End Inlining Report ------\n"; 
}

void InlineReportCallSite::loadCallsToMap(std::map<Instruction*, bool>& LMap) {
  Instruction* NI = getCall();
  if (NI != nullptr) { 
    LMap.insert(std::make_pair(NI, true)); 
  } 
  for (unsigned I = 0, E = Children.size(); I < E; ++I) { 
    Children[I]->loadCallsToMap(LMap); 
  } 
} 

#ifndef NDEBUG
bool InlineReport::validateFunction(Function* F) { 
  llvm::errs() << "Validating " << F->getName() << "\n"; 
  bool ReturnValue = true;
  InlineReportFunctionMap::const_iterator MapIt;
  MapIt = IRFunctionMap.find(F); 
  if (MapIt == IRFunctionMap.end()) { 
    return false;
  } 
  InlineReportFunction* IRF = MapIt->second; 
  IRF->print(Level);
  std::map<Instruction*, bool> OriginalCalls; 
  const InlineReportCallSiteVector& Vec = IRF->getCallSites(); 
  for (unsigned I = 0, E = Vec.size(); I < E; ++I) { 
    Vec[I]->loadCallsToMap(OriginalCalls); 
  } 
  for (Function::iterator BB = F->begin(), E = F->end(); BB != E; ++BB) { 
    for (BasicBlock::iterator I = BB->begin(), E = BB->end(); I != E; ++I) { 
      CallSite CS(cast<Value>(I));
      if (!CS) { 
        continue; 
      } 
      Instruction* NI = CS.getInstruction(); 
      std::map<Instruction*, bool>::const_iterator MapIt;
      MapIt = OriginalCalls.find(NI); 
      if (MapIt == OriginalCalls.end()) { 
        ReturnValue = false; 
        llvm::errs() << "Cannot find " << NI << "\n";
        NI->dump(); 
      } 
    } 
  } 
  llvm::errs() << "Done Validating " << F->getName() << "\n";
  return ReturnValue;
} 

bool InlineReport::validate(void) { 
  bool GlobalRv = true; 
  InlineReportFunctionMap::const_iterator MI, ME; 
  llvm::errs() << "Start Validation Pass\n"; 
  for (MI = IRFunctionMap.begin(), ME = IRFunctionMap.end(); MI != ME; ++MI) { 
    Function* F = MI->first; 
    bool LocalRv = validateFunction(F); 
    llvm::errs() << "Validated " << F->getName();
    if (LocalRv) { 
      llvm::errs() << " passed\n"; 
    }
    else { 
      llvm::errs() << " failed\n"; 
    }  
    GlobalRv &= LocalRv;
  } 
  llvm::errs() << "End Validation Pass\n"; 
  return GlobalRv; 
}
#endif // NDEBUG

void InlineReport::makeCurrent(Module* M, Function* F) {
  InlineReportFunctionMap::const_iterator MapIt = IRFunctionMap.find(F); 
  assert(MapIt != IRFunctionMap.end());
  InlineReportFunction* IRF = MapIt->second; 
  if (IRF->getCurrent()) { 
    return;
  } 
  if (F->isDeclaration()) { 
    IRF->setCurrent(true); 
    return; 
  } 
  for (Function::iterator BB = F->begin(), E = F->end(); BB != E; ++BB) { 
    for (BasicBlock::iterator I = BB->begin(), E = BB->end(); I != E; ++I) { 
      CallSite CS(cast<Value>(I));
      if (!CS) { 
        continue; 
      } 
      Instruction* NI = CS.getInstruction(); 
      InlineReportInstructionCallSiteMap::const_iterator MapItICS; 
      MapItICS = IRInstructionCallSiteMap.find(NI); 
      if (MapItICS != IRInstructionCallSiteMap.end()) { 
        continue; 
      } 
      InlineReportCallSite* IRCS = addCallSite(F, &CS, M); 
      assert(IRCS != nullptr); 
    } 
  } 
  IRF->setCurrent(true); 
} 

void InlineReport::makeAllNotCurrent(void) {
   if (Level == 0) { 
     return; 
   } 
   InlineReportFunctionMap::const_iterator It, E; 
   for (It = IRFunctionMap.begin(), E = IRFunctionMap.end(); It != E; ++It) { 
     InlineReportFunction* IRF = It->second; 
     IRF->setCurrent(false); 
   } 
} 

void InlineReport::replaceFunctionWithFunction(Function* OldFunction, 
  Function* NewFunction) {
  InlineReportFunctionMap::const_iterator IrfIt; 
  if (OldFunction == NewFunction) { 
    return; 
  } 
  IrfIt = IRFunctionMap.find(OldFunction); 
  if (IrfIt == IRFunctionMap.end()) { 
    return; 
  } 
  InlineReportFunction* IRF = IrfIt->second; 
  int count = IRFunctionMap.erase(OldFunction); 
  assert(count == 1); 
  IRFunctionMap.insert(std::make_pair(NewFunction, IRF)); 
  IRF->setLinkageChar(NewFunction); 
  IRF->setName(NewFunction->getName()); 
} 

InlineReportCallSite* InlineReport::getCallSite(CallSite* CS) {
  if (Level == 0) {
    return nullptr; 
  }
  Instruction* NI = CS->getInstruction();
  InlineReportInstructionCallSiteMap::const_iterator
    MapItC = IRInstructionCallSiteMap.find(NI);
  if (MapItC == IRInstructionCallSiteMap.end()) { 
    return nullptr; 
  } 
  return MapItC->second;
} 

InlineReport::~InlineReport(void) {
  while (!IRCallbackVector.empty()) { 
    InlineReportCallback* IRCB = IRCallbackVector.back(); 
    IRCallbackVector.pop_back(); 
    delete IRCB; 
  } 
  InlineReportFunctionMap::const_iterator FI, FE; 
  for (FI = IRFunctionMap.begin(), FE = IRFunctionMap.end(); FI != FE; ++FI) {
    delete FI->second; 
  } 
}

void InlineReport::addCallback(Value* V) { 
  InlineReportCallback* IRCB = new InlineReportCallback(V, this); 
  IRCallbackVector.push_back(IRCB); 
}

