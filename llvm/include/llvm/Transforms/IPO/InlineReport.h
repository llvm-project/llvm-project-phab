//===- InlineReport.h Implement inlining report ---------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines various classes needed to represent an inlining report.
//
//===----------------------------------------------------------------------===//
 
#ifndef LLVM_TRANSFORMS_IPO_INLINEREPORT_H
#define LLVM_TRANSFORMS_IPO_INLINEREPORT_H

#include "llvm/ADT/MapVector.h"
#include "llvm/Analysis/CallGraphReport.h"
#include "llvm/Analysis/CallGraphSCCPass.h"
#include "llvm/Analysis/InlineCost.h"
#include "llvm/Transforms/Utils/Cloning.h"

namespace llvm {

class InlineReportCallSite;

typedef std::vector<InlineReportCallSite*> InlineReportCallSiteVector;

namespace InlineReportTypes { 

typedef enum {
  Basic = 1,      // Print basic information like what was inlined 
  Reasons = 2,    // Add reasons for inlining or not inlining 
                  //   (to be implemented)
  SameLine = 4,   // Put the reasons and the call site on the same lime
  LineCol = 8,    // Print the line and column of the call sites
                  //   if we had appropriate source position information
  File = 16,      // Print the file of the call sites 
  Linkage = 32    // Print linkage info for routines and call sites:
                  //   L: local (F.hasLocalLinkage()) 
                  //   O: link once ODR (one definition rule) 
                  //     (F.hasLinkOnceODRLinkage()) 
                  //   X: available externally (and generally not emitted)
                  //     (F.hasAvailableExternallyLinkage()) 
                  //   A: alternate (something other than L, O, or X)
} InlineReportOptions; 

} 

class InlineReportFunction; 

///
/// \brief Represents a CallSite in the inlining report
///
class InlineReportCallSite {
public:

  // \brief Constructor for InlineReportCallSite
  // The source file is given by 'M'.  The line and column info by 'Dloc'
  explicit InlineReportCallSite(InlineReportFunction* IRCallee, bool IsInlined,
    Module* Module, DebugLoc* DLoc, Instruction* I) : IRCallee(IRCallee), 
    IsInlined(IsInlined), InlineCost(-1), OuterInlineCost (-1), 
    InlineThreshold (-1), Call (I), M (Module) {
    Line = DLoc && DLoc->get() ? DLoc->getLine() : 0;
    Col = DLoc && DLoc->get() ? DLoc->getCol() : 0;
    Children.clear();
  };

  ~InlineReportCallSite(void);
  InlineReportCallSite(const InlineReportCallSite&) = delete; 
  void operator=(const InlineReportCallSite&) = delete; 
 
  // \brief Return a clone of *this, but do not copy its children, and 
  // use the IIMap to get a new value for the 'Call'.
  InlineReportCallSite* cloneBase(const ValueToValueMapTy& IIMap);

  InlineReportFunction* getIRCallee() const { return IRCallee; }
  bool getIsInlined() const { return IsInlined; }
  void setIsInlined(bool Inlined) { IsInlined = Inlined; }

  /// \brief Return the vector of InlineReportCallSites which represent
  /// the calls made from the section of inlined code represented by
  /// this InlineReportCallSite.
  const InlineReportCallSiteVector& getChildren() { return Children; }

  /// \brief Inlining is inhibited if the inline cost is greater than
  /// the threshold.
  int getInlineCost() const { return InlineCost; }
  void setInlineCost(int Cost) { InlineCost = Cost; }

  /// \brief Since inlining is bottom up, always selecting the leaf-most
  /// call sites for inlining is not always best, as it may inhibit inlining
  /// further up the call tree.  Therefore, in addition to an inlining cost,
  /// the inliner computes an outer inlining cost as well.  Inlining is also
  /// inhibited if the outer inlining cost is greater than the inline 
  /// threshold.
  int getOuterInlineCost() const { return OuterInlineCost; }
  void setOuterInlineCost(int Cost) { OuterInlineCost = Cost; }

  int getInlineThreshold() const { return InlineThreshold; }
  void setInlineThreshold(int Threshold) { InlineThreshold = Threshold; }
  Instruction* getCall() const { return Call; }
  void setCall(Instruction* call) { Call = call; }
  void addChild(InlineReportCallSite* IRCS) {
    Children.push_back(IRCS);
  }

  /// \brief Print the info in the inlining instance for the inling report
  /// indenting 'indentCount' indentations, assuming an inlining report
  /// level of 'ReportLevel'.
  void print(unsigned IndentCount, unsigned ReportLevel);

  /// \brief Load the call represented by '*this' and all of its descendant
  /// calls into the map 'Lmap'.
  void loadCallsToMap(std::map<Instruction*, bool>& LMap);
 
private:
  InlineReportFunction* IRCallee;
  bool IsInlined;
  int InlineCost;
  int OuterInlineCost;
  int InlineThreshold;
  InlineReportCallSiteVector Children;
  Instruction* Call;
  ///
  /// \brief Used to get the file name when we print the report 
  Module* M;
  ///
  /// \brief The line and column number of the call site.  These are 0 if
  /// we are not compiling with -g or the lighter weight version
  /// -gline-tables-only
  unsigned Line;
  unsigned Col;
  void printCostAndThreshold(void);
  void printOuterCostAndThreshold(void);
  void printCalleeNameModuleLineCol(unsigned Level);
  // \brief Return a pointer to a copy of Base with an empty Children vector
  InlineReportCallSite* copyBase(const InlineReportCallSite& Base,
    Instruction* NI); 
};

///
/// \brief Represents a routine (compiled or dead) in the inlining report
///
class InlineReportFunction {
public:

  explicit InlineReportFunction(const Function* F) : IsDead(false),
    IsCurrent(false), IsDeclaration(false), LinkageChar(' ') {};
  ~InlineReportFunction(void); 
  InlineReportFunction(const InlineReportFunction&) = delete; 
  void operator=(const InlineReportFunction&) = delete; 

  /// \brief A vector of InlineReportCallSites representing the top-level
  /// call sites in a function (i.e. those which appear in the source code
  /// of the function).
  const InlineReportCallSiteVector& getCallSites() { return CallSites; }

  /// \brief Add an InlineReportCallSite to the list of top-level calls for
  /// this function.
  void addCallSite(InlineReportCallSite* IRCS) {
    CallSites.push_back(IRCS);
  }

  /// \brief Return true if the function has been dead code eliminated.
  bool getDead() const { return IsDead; }

  /// \brief Set whether the function is dead code eliminated.
  void setDead(bool Dead) { IsDead = Dead; }

  /// \brief Return true if the inline report for this routine reflects 
  /// the changes that have been made to the routine since the last call
  /// to Inliner::runOnSCC()
  bool getCurrent(void) const { return IsCurrent; } 

  /// \brief Set whether the inline report for the routine is current
  void setCurrent(bool Current) { IsCurrent = Current; } 

  bool getIsDeclaration(void) const { return IsDeclaration; } 

  void setIsDeclaration(bool Declaration) { IsDeclaration = Declaration; } 

  /// brief Get a single character indicating the linkage type 
  char getLinkageChar(void) { return LinkageChar; } 

  /// brief Set a single character indicating the linkage type 
  void setLinkageChar(Function *F); 

  std::string& getName() { return Name; } 

  void setName(std::string FunctionName) { Name = FunctionName; } 

  void print(unsigned Level) const; 

private:
  bool IsDead;
  bool IsCurrent; 
  bool IsDeclaration; 
  char LinkageChar; 
  std::string Name; 
  InlineReportCallSiteVector CallSites;
};

typedef MapVector<Function*, InlineReportFunction*> 
  InlineReportFunctionMap;
typedef std::vector<InlineReportFunction*> InlineReportFunctionVector; 
typedef std::map<Instruction*, InlineReportCallSite*>
  InlineReportInstructionCallSiteMap;

///
/// \brief The inlining report
///
class InlineReport : public CallGraphReport {
public:

  explicit InlineReport(unsigned MyLevel) :
    Level(MyLevel), ActiveInlineInstruction(nullptr) {};
  virtual ~InlineReport(void); 
  InlineReport(const InlineReport&) = delete; 
  void operator=(const InlineReport&) = delete; 

  // \brief Create an InlineReportFunction to represent F
  InlineReportFunction* addFunction(Function* F, Module* M);

  // \brief Create an InlineReportCallSite to represent CS
  InlineReportCallSite* addCallSite(Function* F, CallSite* CS, Module* M); 

  // \brief Create an InlineReportCallSite to represent CS, if one does 
  // not already exist
  InlineReportCallSite* addNewCallSite(Function* F, CallSite* CS, 
    Module* M); 

  // \brief Indicate that the Function is dead
  void setDead(Function *F);

  /// \brief Indicate that CS has been inlined, and clone and attach the
  /// inlining report for the function being inlined to the
  /// InlineReportCallSite for CS.
  void inlineCallSite(Instruction* NI, InlineReportCallSite* IRCS, Module* M, 
    Function* Callee, InlineFunctionInfo& InlineInfo);

  /// \brief Print the inlining report at the given level.
  void print() const;

#ifndef NDEBUG
  /// \brief Run some simple consistency checking on 'F', e.g.
  /// (1) Check that F is in the inline report's function map 
  /// (2) Check that all of the call/invoke instructions in F's IR 
  ///       appear in the inline report for F 
  bool validateFunction(Function* F); 
  /// \brief Validate all of the functions in the IR function map 
  bool validate(void); 
#endif // NDEBUG

  /// \brief Ensure that the inline report for this routine reflects the 
  /// changes thatr have been made to that routine since the last call to 
  /// Inliner::runOnSCC() 
  void makeCurrent(Module* M,  Function* F); 

  /// \brief Indicate that the inline reports may need to be made current 
  /// with InlineReport::makeCurrent() before they are changed to indicate
  /// additional inlining.
  void makeAllNotCurrent(void); 

  void dumpFunctionMap(void);

  void replaceFunctionWithFunction(Function* OldFunction, 
    Function* NewFunction) override; 

  void addCallback(Value* V); 
  
  InlineReportCallSite* getCallSite(CallSite* CS); 

  Instruction* getActiveInlineInstruction(void) 
    { return ActiveInlineInstruction; } 
  void setActiveInlineInstruction(Instruction* AII) 
    { ActiveInlineInstruction = AII; } 

private:

  /// \brief The Level is specified by the option -inline-report=N.
  /// See llvm/lib/Transforms/IPO/Inliner.cpp for details on Level. 
  unsigned Level;

  // \brief The instruction for the call site currently being inlined 
  Instruction* ActiveInlineInstruction; 

  /// \brief A mapping from Functions to InlineReportFunctions
  InlineReportFunctionMap IRFunctionMap;

  /// \brief A mapping from Instructions to InlineReportCallSites
  InlineReportInstructionCallSiteMap IRInstructionCallSiteMap;

  /// \brief A vector of InlineReportFunctions of Functions that have
  /// been eliminated by dead static function elimination
  InlineReportFunctionVector IRDeadFunctionVector; 

  /// \brief Clone the vector of InlineReportCallSites for NewCallSite
  /// using the mapping of old calls to new calls IIMap
  void cloneChildren(const InlineReportCallSiteVector& OldCallSiteVector,
    InlineReportCallSite* NewCallSite, ValueToValueMapTy& IIMap);

  // \brief Print the inlining option values
  void printOptionValues(void) const;

  ///
  /// \brief CallbackVM for Instructions and Functions in the InlineReport
  ///
  class InlineReportCallback : public CallbackVH { 
    InlineReport* IR; 
    void deleted() override { 
      assert(IR != nullptr); 
      if (isa<Instruction>(getValPtr())) { 
        /// \brief Indicate in the inline report that the call site 
        /// corresponding to the Value has been deleted
        Instruction* I = cast<Instruction>(getValPtr()); 
        if (IR->getActiveInlineInstruction() != I) { 
          InlineReportInstructionCallSiteMap::const_iterator MapIt;  
          MapIt = IR->IRInstructionCallSiteMap.find(I);
          if (MapIt != IR->IRInstructionCallSiteMap.end()) {
            IR->IRInstructionCallSiteMap.erase(MapIt); 
          }
        }
      }  
      else if (isa<Function>(getValPtr())) {
        /// \brief Indicate in the inline report that the function
        /// corresponding to the Value has been deleted
        Function* F = cast<Function>(getValPtr()); 
        InlineReportFunctionMap::iterator MapIt; 
        MapIt = IR->IRFunctionMap.find(F);
        if (MapIt != IR->IRFunctionMap.end()) { 
          InlineReportFunction* IRF = MapIt->second; 
          IR->setDead(F); 
          IRF->setLinkageChar(F);
          IR->IRFunctionMap.erase(MapIt);
          IR->IRDeadFunctionVector.push_back(IRF); 
        } 
      } 
      setValPtr(nullptr); 
    };
  public: 
    InlineReportCallback(Value* V, InlineReport *CBIR) : 
      CallbackVH(V), IR(CBIR) {}; 
    virtual ~InlineReportCallback() {};
  };

  SmallVector<InlineReportCallback*,16> IRCallbackVector;

};

}

#endif 
