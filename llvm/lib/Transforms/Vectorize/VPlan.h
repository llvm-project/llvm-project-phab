//===- VPlan.h - Represent A Vectorizer Plan ------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the declarations of the Vectorization Plan base classes:
// 1. VPBasicBlock and VPRegionBlock that inherit from a common pure virtual
//    VPBlockBase, together implementing a Hierarchical CFG;
// 2. Specializations of GraphTraits that allow VPBlockBase graphs to be treated
//    as proper graphs for generic algorithms;
// 3. Pure virtual VPRecipeBase and its pure virtual sub-classes
//    VPConditionBitRecipeBase and VPOneByOneRecipeBase that
//    represent base classes for recipes contained within VPBasicBlocks;
// 4. The VPlan class holding a candidate for vectorization;
// 5. The VPlanUtils class providing methods for building plans;
// 6. The VPlanPrinter class providing a way to print a plan in dot format.
// These are documented in docs/VectorizationPlan.rst.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TRANSFORMS_VECTORIZE_VPLAN_H
#define LLVM_TRANSFORMS_VECTORIZE_VPLAN_H

#include "llvm/ADT/GraphTraits.h"
#include "llvm/ADT/ilist.h"
#include "llvm/ADT/ilist_node.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"

// The (re)use of existing LoopVectorize classes is subject to future VPlan
// refactoring.
namespace {
class InnerLoopVectorizer;
class LoopVectorizationLegality;
class LoopVectorizationCostModel;
}

namespace llvm {

class VPBasicBlock;

/// VPRecipeBase is a base class describing one or more instructions that will
/// appear consecutively in the vectorized version, based on Instructions from
/// the given IR. These Instructions are referred to as the "Ingredients" of
/// the Recipe. A Recipe specifies how its ingredients are to be vectorized:
/// e.g., copy or reuse them as uniform, scalarize or vectorize them according
/// to an enclosing loop dimension, vectorize them according to internal SLP
/// dimension.
///
/// **Design principle:** in order to reason about how to vectorize an
/// Instruction or how much it would cost, one has to consult the VPRecipe
/// holding it.
///
/// **Design principle:** when a sequence of instructions conveys additional
/// information as a group, we use a VPRecipe to encapsulate them and attach
/// this information to the VPRecipe. For instance a VPRecipe can model an
/// interleave group of loads or stores with additional information for
/// calculating their cost and for performing IR code generation, as a group.
///
/// **Design principle:** a VPRecipe should reuse existing containers of its
/// ingredients, i.e., iterators of basic blocks, to be lightweight. A new
/// containter should be opened on-demand, e.g., to avoid excessive recipes
/// each holding an interval of ingredients.
class VPRecipeBase : public ilist_node_with_parent<VPRecipeBase, VPBasicBlock> {
  friend class VPlanUtils;
  friend class VPBasicBlock;

private:
  const unsigned char VRID; /// Subclass identifier (for isa/dyn_cast)

  /// Each VPRecipe is contained in a single VPBasicBlock.
  class VPBasicBlock *Parent;

  /// Record which Instructions would require generating their complementing
  /// form as well, providing a vector-to-scalar or scalar-to-vector conversion.
  SmallPtrSet<Instruction *, 1> AlsoPackOrUnpack;

public:
  /// An enumeration for keeping track of the concrete subclass of VPRecipeBase
  /// that is actually instantiated. Values of this enumeration are kept in the
  /// VPRecipe classes VRID field. They are used for concrete type
  /// identification.
  typedef enum {
    VPVectorizeOneByOneSC,
    VPScalarizeOneByOneSC,
    VPWidenIntInductionSC,
    VPBuildScalarStepsSC,
    VPInterleaveSC,
    VPExtractMaskBitSC,
    VPMergeScalarizeBranchSC,
  } VPRecipeTy;

  VPRecipeBase(const unsigned char SC) : VRID(SC), Parent(nullptr) {}

  virtual ~VPRecipeBase() {}

  /// \return an ID for the concrete type of this object.
  /// This is used to implement the classof checks. This should not be used
  /// for any other purpose, as the values may change as LLVM evolves.
  unsigned getVPRecipeID() const { return VRID; }

  /// \return the VPBasicBlock which this VPRecipe belongs to.
  class VPBasicBlock *getParent() {
    return Parent;
  }

  /// The method which generates the new IR instructions that correspond to
  /// this VPRecipe in the vectorized version, thereby "executing" the VPlan.
  virtual void vectorize(struct VPTransformState &State) = 0;

  /// Each recipe prints itself.
  virtual void print(raw_ostream &O) const = 0;

  /// Add an instruction to the set of instructions for which a vector-to-
  /// scalar or scalar-to-vector conversion is needed, in addition to
  /// vectorizing or scalarizing the instruction itself, respectively.
  void addAlsoPackOrUnpack(Instruction *I) { AlsoPackOrUnpack.insert(I); }

  /// Indicates if a given instruction requires vector-to-scalar or scalar-to-
  /// vector conversion.
  bool willAlsoPackOrUnpack(Instruction *I) const {
    return AlsoPackOrUnpack.count(I);
  }
};

/// A VPConditionBitRecipeBase is a pure virtual VPRecipe which supports a
/// conditional branch. Concrete sub-classes of this recipe are in charge of
/// generating the instructions that compute the condition for this branch in
/// the vectorized version.
class VPConditionBitRecipeBase : public VPRecipeBase {
protected:
  /// The actual condition bit that was generated. Holds null until the
  /// value/instuctions are generated by the vectorize() method.
  Value *ConditionBit;

public:
  /// Construct a VPConditionBitRecipeBase, simply propating its concrete type.
  VPConditionBitRecipeBase(const unsigned char SC)
      : VPRecipeBase(SC), ConditionBit(nullptr) {}

  /// \return the actual bit that was generated, to be plugged into the IR
  /// conditional branch, or null if the code computing the actual bit has not
  /// been generated yet.
  Value *getConditionBit() { return ConditionBit; }

  virtual StringRef getName() const = 0;
};

/// VPOneByOneRecipeBase is a VPRecipeBase which handles each Instruction in its
/// ingredients independently, in order. The ingredients are either all
/// vectorized, or all scalarized.
/// A VPOneByOneRecipeBase is a virtual base recipe which can be materialized
/// by one of two sub-classes, namely VPVectorizeOneByOneRecipe or
/// VPScalarizeOneByOneRecipe for Vectorizing or Scalarizing all ingredients,
/// respectively.
/// The ingredients are held as a sub-sequence of original Instructions, which
/// reside in the same IR BasicBlock and in the same order. The Ingredients are
/// accessed by a pointer to the first and last Instruction.
class VPOneByOneRecipeBase : public VPRecipeBase {
  friend class VPlanUtilsLoopVectorizer;

public:
  /// Hold the ingredients by pointing to their original BasicBlock location.
  BasicBlock::iterator Begin;
  BasicBlock::iterator End;

protected:
  VPOneByOneRecipeBase() = delete;

  VPOneByOneRecipeBase(unsigned char SC, const BasicBlock::iterator B,
                       const BasicBlock::iterator E, class VPlan *Plan);

  /// Do the actual code generation for a single instruction.
  /// This function is to be implemented and specialized by the respective
  /// sub-class.
  virtual void transformIRInstruction(Instruction *I,
                                      struct VPTransformState &State) = 0;

public:
  ~VPOneByOneRecipeBase() {}

  /// Method to support type inquiry through isa, cast, and dyn_cast.
  static inline bool classof(const VPRecipeBase *V) {
    return V->getVPRecipeID() == VPRecipeBase::VPScalarizeOneByOneSC ||
           V->getVPRecipeID() == VPRecipeBase::VPVectorizeOneByOneSC;
  }

  bool isScalarizing() const {
    return getVPRecipeID() == VPRecipeBase::VPScalarizeOneByOneSC;
  }

  /// The method which generates all new IR instructions that correspond to
  /// this VPOneByOneRecipeBase in the vectorized version, thereby
  /// "executing" the VPlan.
  /// VPOneByOneRecipeBase may either scalarize or vectorize all Instructions.
  void vectorize(struct VPTransformState &State) override {
    for (auto It = Begin; It != End; ++It)
      transformIRInstruction(&*It, State);
  }

  const BasicBlock::iterator &begin() { return Begin; }

  const BasicBlock::iterator &end() { return End; }
};

/// Hold the indices of a specific scalar instruction. The VPIterationInstance
/// span the iterations of the original loop, that correspond to a single
/// iteration of the vectorized loop.
struct VPIterationInstance {
  unsigned Part;
  unsigned Lane;
};

// Forward declaration.
class BasicBlock;

/// Hold additional information passed down when "executing" a VPlan, that is
/// needed for generating IR. Also facilitates reuse of existing LV
/// functionality.
struct VPTransformState {

  VPTransformState(unsigned VF, unsigned UF, class LoopInfo *LI,
                   class DominatorTree *DT, IRBuilder<> &Builder,
                   InnerLoopVectorizer *ILV, LoopVectorizationLegality *Legal,
                   LoopVectorizationCostModel *Cost)
      : VF(VF), UF(UF), Instance(nullptr), LI(LI), DT(DT), Builder(Builder),
        ILV(ILV), Legal(Legal), Cost(Cost) {}

  /// Record the selected vectorization and unroll factors of the single loop
  /// being vectorized.
  unsigned VF;
  unsigned UF;

  /// Hold the indices to generate a specific scalar instruction. Null indicates
  /// that all instances are to be generated, using either scalar or vector
  /// instructions.
  VPIterationInstance *Instance;

  /// Hold state information used when constructing the CFG of the vectorized
  /// Loop, traversing the VPBasicBlocks and generating corresponding IR
  /// BasicBlocks.
  struct CFGState {
    /// The previous VPBasicBlock visited. In the beginning set to null.
    VPBasicBlock *PrevVPBB;
    /// The previous IR BasicBlock created or reused. In the beginning set to
    /// the new header BasicBlock.
    BasicBlock *PrevBB;
    /// The last IR BasicBlock of the loop body. Set to the new latch
    /// BasicBlock, used for placing the newly created BasicBlocks.
    BasicBlock *LastBB;
    /// A mapping of each VPBasicBlock to the corresponding BasicBlock. In case
    /// of replication, maps the BasicBlock of the last replica created.
    SmallDenseMap<class VPBasicBlock *, class BasicBlock *> VPBB2IRBB;

    CFGState() : PrevVPBB(nullptr), PrevBB(nullptr), LastBB(nullptr) {}
  } CFG;

  /// Hold pointer to LoopInfo to register new basic blocks in the loop.
  class LoopInfo *LI;

  /// Hold pointer to Dominator Tree to register new basic blocks in the loop.
  class DominatorTree *DT;

  /// Hold a reference to the IRBuilder used to generate IR code.
  IRBuilder<> &Builder;

  /// Hold a pointer to InnerLoopVectorizer to reuse its IR generation methods.
  class InnerLoopVectorizer *ILV;

  /// Hold a pointer to LoopVectorizationLegality
  class LoopVectorizationLegality *Legal;

  /// Hold a pointer to LoopVectorizationCostModel to access its
  /// IsUniformAfterVectorization method.
  LoopVectorizationCostModel *Cost;
};

/// VPBlockBase is the building block of the Hierarchical CFG. A VPBlockBase
/// can be either a VPBasicBlock or a VPRegionBlock.
///
/// The Hierarchical CFG is a control-flow graph whose nodes are basic-blocks
/// or Hierarchical CFG's. The Hierarchical CFG data structure we use is similar
/// to the Tile Tree [1], where cross-Tile edges are lifted to connect Tiles
/// instead of the original basic-blocks as in Sharir [2], promoting the Tile
/// encapsulation. We use the terms Region and Block rather than Tile [1] to
/// avoid confusion with loop tiling.
///
/// [1] "Register Allocation via Hierarchical Graph Coloring", David Callahan
/// and Brian Koblenz, PLDI 1991
///
/// [2] "Structural analysis: A new approach to flow analysis in optimizing
/// compilers", M. Sharir, Journal of Computer Languages, Jan. 1980
///
/// Note that in contrast to the IR BasicBlock, a VPBlockBase models its
/// control-flow edges with successor and predecessor VPBlockBase directly,
/// rather than through a Terminator branch or through predecessor branches that
/// Use the VPBlockBase.
class VPBlockBase {
  friend class VPlanUtils;

private:
  const unsigned char VBID; /// Subclass identifier (for isa/dyn_cast).

  std::string Name;

  /// The immediate VPRegionBlock which this VPBlockBase belongs to, or null if
  /// it is a topmost VPBlockBase.
  class VPRegionBlock *Parent;

  /// List of predecessor blocks.
  SmallVector<VPBlockBase *, 1> Predecessors;

  /// List of successor blocks.
  SmallVector<VPBlockBase *, 1> Successors;

  /// Successor selector, null for zero or single successor blocks.
  VPConditionBitRecipeBase *ConditionBitRecipe;

  /// Add \p Successor as the last successor to this block.
  void appendSuccessor(VPBlockBase *Successor) {
    assert(Successor && "Cannot add nullptr successor!");
    Successors.push_back(Successor);
  }

  /// Add \p Predecessor as the last predecessor to this block.
  void appendPredecessor(VPBlockBase *Predecessor) {
    assert(Predecessor && "Cannot add nullptr predecessor!");
    Predecessors.push_back(Predecessor);
  }

  /// Remove \p Predecessor from the predecessors of this block.
  void removePredecessor(VPBlockBase *Predecessor) {
    auto Pos = std::find(Predecessors.begin(), Predecessors.end(), Predecessor);
    assert(Pos && "Predecessor does not exist");
    Predecessors.erase(Pos);
  }

  /// Remove \p Successor from the successors of this block.
  void removeSuccessor(VPBlockBase *Successor) {
    auto Pos = std::find(Successors.begin(), Successors.end(), Successor);
    assert(Pos && "Successor does not exist");
    Successors.erase(Pos);
  }

protected:
  VPBlockBase(const unsigned char SC, const std::string &N)
      : VBID(SC), Name(N), Parent(nullptr), ConditionBitRecipe(nullptr) {}

public:
  /// An enumeration for keeping track of the concrete subclass of VPBlockBase
  /// that is actually instantiated. Values of this enumeration are kept in the
  /// VPBlockBase classes VBID field. They are used for concrete type
  /// identification.
  typedef enum { VPBasicBlockSC, VPRegionBlockSC } VPBlockTy;

  virtual ~VPBlockBase() {}

  const std::string &getName() const { return Name; }

  /// \return an ID for the concrete type of this object.
  /// This is used to implement the classof checks. This should not be used
  /// for any other purpose, as the values may change as LLVM evolves.
  unsigned getVPBlockID() const { return VBID; }

  const class VPRegionBlock *getParent() const { return Parent; }

  /// \return the VPBasicBlock that is the entry of this VPBlockBase,
  /// recursively, if the latter is a VPRegionBlock. Otherwise, if this
  /// VPBlockBase is a VPBasicBlock, it is returned.
  const class VPBasicBlock *getEntryBasicBlock() const;

  /// \return the VPBasicBlock that is the exit of this VPBlockBase,
  /// recursively, if the latter is a VPRegionBlock. Otherwise, if this
  /// VPBlockBase is a VPBasicBlock, it is returned.
  const class VPBasicBlock *getExitBasicBlock() const;
  class VPBasicBlock *getExitBasicBlock();

  const SmallVectorImpl<VPBlockBase *> &getSuccessors() const {
    return Successors;
  }

  const SmallVectorImpl<VPBlockBase *> &getPredecessors() const {
    return Predecessors;
  }

  SmallVectorImpl<VPBlockBase *> &getSuccessors() { return Successors; }

  SmallVectorImpl<VPBlockBase *> &getPredecessors() { return Predecessors; }

  /// \return the successor of this VPBlockBase if it has a single successor.
  /// Otherwise return a null pointer.
  VPBlockBase *getSingleSuccessor() const {
    return (Successors.size() == 1 ? *Successors.begin() : nullptr);
  }

  /// \return the predecessor of this VPBlockBase if it has a single
  /// predecessor. Otherwise return a null pointer.
  VPBlockBase *getSinglePredecessor() const {
    return (Predecessors.size() == 1 ? *Predecessors.begin() : nullptr);
  }

  /// Returns the closest ancestor starting from "this", which has successors.
  /// Returns the root ancestor if all ancestors have no successors.
  VPBlockBase *getAncestorWithSuccessors();

  /// Returns the closest ancestor starting from "this", which has predecessors.
  /// Returns the root ancestor if all ancestors have no predecessors.
  VPBlockBase *getAncestorWithPredecessors();

  /// \return the successors either attached directly to this VPBlockBase or, if
  /// this VPBlockBase is the exit block of a VPRegionBlock and has no
  /// successors of its own, search recursively for the first enclosing
  /// VPRegionBlock that has successors and return them. If no such
  /// VPRegionBlock exists, return the (empty) successors of the topmost
  /// VPBlockBase reached.
  const SmallVectorImpl<VPBlockBase *> &getHierarchicalSuccessors() {
    return getAncestorWithSuccessors()->getSuccessors();
  }

  /// \return the hierarchical successor of this VPBlockBase if it has a single
  /// hierarchical successor. Otherwise return a null pointer.
  VPBlockBase *getSingleHierarchicalSuccessor() {
    return getAncestorWithSuccessors()->getSingleSuccessor();
  }

  /// \return the predecessors either attached directly to this VPBlockBase or,
  /// if this VPBlockBase is the entry block of a VPRegionBlock and has no
  /// predecessors of its own, search recursively for the first enclosing
  /// VPRegionBlock that has predecessors and return them. If no such
  /// VPRegionBlock exists, return the (empty) predecessors of the topmost
  /// VPBlockBase reached.
  const SmallVectorImpl<VPBlockBase *> &getHierarchicalPredecessors() {
    return getAncestorWithPredecessors()->getPredecessors();
  }

  /// \return the hierarchical predecessor of this VPBlockBase if it has a
  /// single hierarchical predecessor. Otherwise return a null pointer.
  VPBlockBase *getSingleHierarchicalPredecessor() {
    return getAncestorWithPredecessors()->getSinglePredecessor();
  }

  /// If a VPBlockBase has two successors, this is the Recipe that will generate
  /// the condition bit selecting the successor, and feeding the terminating
  /// conditional branch. Otherwise this is null.
  VPConditionBitRecipeBase *getConditionBitRecipe() {
    return ConditionBitRecipe;
  }

  const VPConditionBitRecipeBase *getConditionBitRecipe() const {
    return ConditionBitRecipe;
  }

  void setConditionBitRecipe(VPConditionBitRecipeBase *R) {
    ConditionBitRecipe = R;
  }

  /// The method which generates all new IR instructions that correspond to
  /// this VPBlockBase in the vectorized version, thereby "executing" the VPlan.
  virtual void vectorize(struct VPTransformState *State) = 0;

  /// Delete all blocks reachable from a given VPBlockBase, inclusive.
  static void deleteCFG(VPBlockBase *Entry);
};

/// VPBasicBlock serves as the leaf of the Hierarchical CFG. It represents a
/// sequence of instructions that will appear consecutively in a basic block
/// of the vectorized version. The VPBasicBlock takes care of the control-flow
/// relations with other VPBasicBlock's and Regions. It holds a sequence of zero
/// or more VPRecipe's that take care of representing the instructions.
/// A VPBasicBlock that holds no VPRecipe's represents no instructions; this
/// may happen, e.g., to support disjoint Regions and to ensure Regions have a
/// single exit, possibly an empty one.
///
/// Note that in contrast to the IR BasicBlock, a VPBasicBlock models its
/// control-flow edges with successor and predecessor VPBlockBase directly,
/// rather than through a Terminator branch or through predecessor branches that
/// "use" the VPBasicBlock.
class VPBasicBlock : public VPBlockBase {
  friend class VPlanUtils;

public:
  typedef iplist<VPRecipeBase> RecipeListTy;

private:
  /// The list of VPRecipes, held in order of instructions to generate.
  RecipeListTy Recipes;

public:
  /// Instruction iterators...
  typedef RecipeListTy::iterator iterator;
  typedef RecipeListTy::const_iterator const_iterator;
  typedef RecipeListTy::reverse_iterator reverse_iterator;
  typedef RecipeListTy::const_reverse_iterator const_reverse_iterator;

  //===--------------------------------------------------------------------===//
  /// Recipe iterator methods
  ///
  inline iterator begin() { return Recipes.begin(); }
  inline const_iterator begin() const { return Recipes.begin(); }
  inline iterator end() { return Recipes.end(); }
  inline const_iterator end() const { return Recipes.end(); }

  inline reverse_iterator rbegin() { return Recipes.rbegin(); }
  inline const_reverse_iterator rbegin() const { return Recipes.rbegin(); }
  inline reverse_iterator rend() { return Recipes.rend(); }
  inline const_reverse_iterator rend() const { return Recipes.rend(); }

  inline size_t size() const { return Recipes.size(); }
  inline bool empty() const { return Recipes.empty(); }
  inline const VPRecipeBase &front() const { return Recipes.front(); }
  inline VPRecipeBase &front() { return Recipes.front(); }
  inline const VPRecipeBase &back() const { return Recipes.back(); }
  inline VPRecipeBase &back() { return Recipes.back(); }

  /// Return the underlying instruction list container.
  ///
  /// Currently you need to access the underlying instruction list container
  /// directly if you want to modify it.
  const RecipeListTy &getInstList() const { return Recipes; }
  RecipeListTy &getInstList() { return Recipes; }

  /// Returns a pointer to a member of the instruction list.
  static RecipeListTy VPBasicBlock::*getSublistAccess(VPRecipeBase *) {
    return &VPBasicBlock::Recipes;
  }

  VPBasicBlock(const std::string &Name) : VPBlockBase(VPBasicBlockSC, Name) {}

  ~VPBasicBlock() { Recipes.clear(); }

  /// Method to support type inquiry through isa, cast, and dyn_cast.
  static inline bool classof(const VPBlockBase *V) {
    return V->getVPBlockID() == VPBlockBase::VPBasicBlockSC;
  }

  /// Augment the existing recipes of a VPBasicBlock with an additional
  /// \p Recipe at a position given by an existing recipe \p Before. If
  /// \p Before is null, \p Recipe is appended as the last recipe.
  void addRecipe(VPRecipeBase *Recipe, VPRecipeBase *Before = nullptr) {
    Recipe->Parent = this;
    if (!Before) {
      Recipes.push_back(Recipe);
      return;
    }
    assert(Before->Parent == this &&
           "Insertion before point not in this basic block.");
    Recipes.insert(Before->getIterator(), Recipe);
  }

  void removeRecipe(VPRecipeBase *Recipe) {
    assert(Recipe->Parent == this &&
           "Recipe to remove not in this basic block.");
    Recipes.remove(Recipe);
    Recipe->Parent = nullptr;
  }

  /// The method which generates all new IR instructions that correspond to
  /// this VPBasicBlock in the vectorized version, thereby "executing" the
  /// VPlan.
  void vectorize(struct VPTransformState *State) override;

  /// Retrieve the list of VPRecipes that belong to this VPBasicBlock.
  const RecipeListTy &getRecipes() const { return Recipes; }

private:
  /// Create an IR BasicBlock to hold the instructions vectorized from this
  /// VPBasicBlock, and return it. Update the CFGState accordingly.
  BasicBlock *createEmptyBasicBlock(VPTransformState::CFGState &CFG);
};

/// VPRegionBlock represents a collection of VPBasicBlocks and VPRegionBlocks
/// which form a single-entry-single-exit subgraph of the CFG in the vectorized
/// code.
///
/// A VPRegionBlock may indicate that its contents are to be replicated several
/// times. This is designed to support predicated scalarization, in which a
/// scalar if-then code structure needs to be generated VF * UF times. Having
/// this replication indicator helps to keep a single VPlan for multiple
/// candidate VF's; the actual replication takes place only once the desired VF
/// and UF have been determined.
///
/// **Design principle:** when some additional information relates to an SESE
/// set of VPBlockBase, we use a VPRegionBlock to wrap them and attach the
/// information to it. For example, a VPRegionBlock can be used to indicate that
/// a scalarized SESE region is to be replicated, and that a vectorized SESE
/// region can retain its internal control-flow, independent of the control-flow
/// external to the region.
class VPRegionBlock : public VPBlockBase {
  friend class VPlanUtils;

private:
  /// Hold the Single Entry of the SESE region represented by the VPRegionBlock.
  VPBlockBase *Entry;

  /// Hold the Single Exit of the SESE region represented by the VPRegionBlock.
  VPBlockBase *Exit;

  /// A VPRegionBlock can represent either a single instance of its
  /// VPBlockBases, or multiple (VF * UF) replicated instances. The latter is
  /// used when the internal SESE region handles a single scalarized lane.
  bool IsReplicator;

public:
  VPRegionBlock(const std::string &Name)
      : VPBlockBase(VPRegionBlockSC, Name), Entry(nullptr), Exit(nullptr),
        IsReplicator(false) {}

  ~VPRegionBlock() {
    if (Entry)
      deleteCFG(Entry);
  }

  /// Method to support type inquiry through isa, cast, and dyn_cast.
  static inline bool classof(const VPBlockBase *V) {
    return V->getVPBlockID() == VPBlockBase::VPRegionBlockSC;
  }

  VPBlockBase *getEntry() { return Entry; }

  VPBlockBase *getExit() { return Exit; }

  const VPBlockBase *getEntry() const { return Entry; }

  const VPBlockBase *getExit() const { return Exit; }

  /// An indicator if the VPRegionBlock represents single or multiple instances.
  bool isReplicator() const { return IsReplicator; }

  void setReplicator(bool ToReplicate) { IsReplicator = ToReplicate; }

  /// The method which generates the new IR instructions that correspond to
  /// this VPRegionBlock in the vectorized version, thereby "executing" the
  /// VPlan.
  void vectorize(struct VPTransformState *State) override;
};

/// A VPlan represents a candidate for vectorization, encoding various decisions
/// taken to produce efficient vector code, including: which instructions are to
/// vectorized or scalarized, which branches are to appear in the vectorized
/// version. It models the control-flow of the candidate vectorized version
/// explicitly, and holds prescriptions for generating the code for this version
/// from a given IR code.
/// VPlan takes a "senario-based approach" to vectorization planning - different
/// scenarios, corresponding to making different decisions, can be modeled using
/// different VPlans.
/// The corresponding IR code is required to be SESE.
/// The vectorized version is represented using a Hierarchical CFG.
class VPlan {
  friend class VPlanUtils;
  friend class VPlanUtilsLoopVectorizer;

private:
  /// Hold the single entry to the Hierarchical CFG of the VPlan.
  VPBlockBase *Entry;

  /// The IR instructions which are to be transformed to fill the vectorized
  /// version are held as ingredients inside the VPRecipe's of the VPlan. Hold a
  /// reverse mapping to locate the VPRecipe an IR instruction belongs to. This
  /// serves optimizations that operate on the VPlan.
  DenseMap<Instruction *, VPRecipeBase *> Inst2Recipe;

public:
  VPlan() : Entry(nullptr) {}

  ~VPlan() {
    if (Entry)
      VPBlockBase::deleteCFG(Entry);
  }

  /// Generate the IR code for this VPlan.
  void vectorize(struct VPTransformState *State);

  VPBlockBase *getEntry() { return Entry; }
  const VPBlockBase *getEntry() const { return Entry; }

  void setEntry(VPBlockBase *Block) { Entry = Block; }

  /// Retrieve the VPRecipe a given instruction \p Inst belongs to in the VPlan.
  /// Returns null if it belongs to no VPRecipe.
  VPRecipeBase *getRecipe(Instruction *Inst) {
    auto It = Inst2Recipe.find(Inst);
    if (It == Inst2Recipe.end())
      return nullptr;
    return It->second;
  }

  void setInst2Recipe(Instruction *I, VPRecipeBase *R) { Inst2Recipe[I] = R; }

  void resetInst2Recipe(Instruction *I) { Inst2Recipe.erase(I); }

  /// Retrieve the VPBasicBlock a given instruction \p Inst belongs to in the
  /// VPlan. Returns null if it belongs to no VPRecipe.
  VPBasicBlock *getBasicBlock(Instruction *Inst) {
    VPRecipeBase *Recipe = getRecipe(Inst);
    if (!Recipe)
      return nullptr;
    return Recipe->getParent();
  }

private:
  /// Add to the given dominator tree the header block and every new basic block
  /// that was created between it and the latch block, inclusive.
  void updateDominatorTree(class DominatorTree *DT, BasicBlock *LoopPreHeaderBB,
                           BasicBlock *LoopLatchBB);
};

/// The VPlanUtils class provides interfaces for the construction and
/// manipulation of a VPlan.
class VPlanUtils {
private:
  /// Unique ID generator.
  static unsigned NextOrdinal;

protected:
  VPlan *Plan;

  typedef iplist<VPRecipeBase> RecipeListTy;
  RecipeListTy *getRecipes(VPBasicBlock *Block) { return &Block->Recipes; }

public:
  VPlanUtils(VPlan *Plan) : Plan(Plan) {}

  ~VPlanUtils() {}

  /// Create a unique name for a new VPlan entity such as a VPBasicBlock or
  /// VPRegionBlock.
  std::string createUniqueName(const char *Prefix) {
    std::string S;
    raw_string_ostream RSO(S);
    RSO << Prefix << NextOrdinal++;
    return RSO.str();
  }

  /// Add a given \p Recipe as the last recipe of a given VPBasicBlock.
  void appendRecipeToBasicBlock(VPRecipeBase *Recipe, VPBasicBlock *ToVPBB) {
    assert(Recipe && "No recipe to append.");
    assert(!Recipe->Parent && "Recipe already in VPlan");
    ToVPBB->addRecipe(Recipe);
  }

  /// Create a new empty VPBasicBlock and return it.
  VPBasicBlock *createBasicBlock() {
    VPBasicBlock *BasicBlock = new VPBasicBlock(createUniqueName("BB"));
    return BasicBlock;
  }

  /// Create a new VPBasicBlock with a single \p Recipe and return it.
  VPBasicBlock *createBasicBlock(VPRecipeBase *Recipe) {
    VPBasicBlock *BasicBlock = new VPBasicBlock(createUniqueName("BB"));
    appendRecipeToBasicBlock(Recipe, BasicBlock);
    return BasicBlock;
  }

  /// Create a new, empty VPRegionBlock, with no blocks.
  VPRegionBlock *createRegion(bool IsReplicator) {
    VPRegionBlock *Region = new VPRegionBlock(createUniqueName("region"));
    setReplicator(Region, IsReplicator);
    return Region;
  }

  /// Set the entry VPBlockBase of a given VPRegionBlock to a given \p Block.
  /// Block is to have no predecessors.
  void setRegionEntry(VPRegionBlock *Region, VPBlockBase *Block) {
    assert(Block->Predecessors.empty() &&
           "Entry block cannot have predecessors.");
    Region->Entry = Block;
    Block->Parent = Region;
  }

  /// Set the exit VPBlockBase of a given VPRegionBlock to a given \p Block.
  /// Block is to have no successors.
  void setRegionExit(VPRegionBlock *Region, VPBlockBase *Block) {
    assert(Block->Successors.empty() && "Exit block cannot have successors.");
    Region->Exit = Block;
    Block->Parent = Region;
  }

  void setReplicator(VPRegionBlock *Region, bool ToReplicate) {
    Region->setReplicator(ToReplicate);
  }

  /// Sets a given VPBlockBase \p Successor as the single successor of another
  /// VPBlockBase \p Block. The parent of \p Block is copied to be the parent of
  /// \p Successor.
  void setSuccessor(VPBlockBase *Block, VPBlockBase *Successor) {
    assert(Block->getSuccessors().empty() && "Block successors already set.");
    Block->appendSuccessor(Successor);
    Successor->appendPredecessor(Block);
    Successor->Parent = Block->Parent;
  }

  /// Sets two given VPBlockBases \p IfTrue and \p IfFalse to be the two
  /// successors of another VPBlockBase \p Block. A given
  /// VPConditionBitRecipeBase provides the control selector. The parent of
  /// \p Block is copied to be the parent of \p IfTrue and \p IfFalse.
  void setTwoSuccessors(VPBlockBase *Block, VPConditionBitRecipeBase *R,
                        VPBlockBase *IfTrue, VPBlockBase *IfFalse) {
    assert(Block->getSuccessors().empty() && "Block successors already set.");
    Block->setConditionBitRecipe(R);
    Block->appendSuccessor(IfTrue);
    Block->appendSuccessor(IfFalse);
    IfTrue->appendPredecessor(Block);
    IfFalse->appendPredecessor(Block);
    IfTrue->Parent = Block->Parent;
    IfFalse->Parent = Block->Parent;
  }

  /// Given two VPBlockBases \p From and \p To, disconnect them from each other.
  void disconnectBlocks(VPBlockBase *From, VPBlockBase *To) {
    From->removeSuccessor(To);
    To->removePredecessor(From);
  }
};

/// VPlanPrinter prints a given VPlan to a given output stream. The printing is
/// indented and follows the dot format.
class VPlanPrinter {
private:
  raw_ostream &OS;
  const VPlan &Plan;
  unsigned Depth;
  unsigned TabLength = 2;
  std::string Indent;

  /// Handle indentation.
  void buildIndent() { Indent = std::string(Depth * TabLength, ' '); }
  void resetDepth() {
    Depth = 1;
    buildIndent();
  }
  void increaseDepth() {
    ++Depth;
    buildIndent();
  }
  void decreaseDepth() {
    --Depth;
    buildIndent();
  }

  /// Dump each element of VPlan.
  void dumpBlock(const VPBlockBase *Block);
  void dumpEdges(const VPBlockBase *Block);
  void dumpBasicBlock(const VPBasicBlock *BasicBlock);
  void dumpRegion(const VPRegionBlock *Region);

  const char *getNodePrefix(const VPBlockBase *Block);
  const std::string &getReplicatorString(const VPRegionBlock *Region);
  void drawEdge(const VPBlockBase *From, const VPBlockBase *To, bool Hidden,
                const Twine &Label);

public:
  VPlanPrinter(raw_ostream &O, const VPlan &P) : OS(O), Plan(P) {}
  void dump(const std::string &Title = "");
};

//===--------------------------------------------------------------------===//
// GraphTraits specializations for VPlan/VPRegionBlock Control-Flow Graphs  //
//===--------------------------------------------------------------------===//

// Provide specializations of GraphTraits to be able to treat a VPRegionBlock
// as a graph of VPBlockBases...

template <> struct GraphTraits<VPBlockBase *> {
  typedef VPBlockBase *NodeRef;
  typedef SmallVectorImpl<VPBlockBase *>::iterator ChildIteratorType;

  static NodeRef getEntryNode(NodeRef N) { return N; }

  static inline ChildIteratorType child_begin(NodeRef N) {
    return N->getSuccessors().begin();
  }

  static inline ChildIteratorType child_end(NodeRef N) {
    return N->getSuccessors().end();
  }
};

template <> struct GraphTraits<const VPBlockBase *> {
  typedef const VPBlockBase *NodeRef;
  typedef SmallVectorImpl<VPBlockBase *>::const_iterator ChildIteratorType;

  static NodeRef getEntryNode(NodeRef N) { return N; }

  static inline ChildIteratorType child_begin(NodeRef N) {
    return N->getSuccessors().begin();
  }

  static inline ChildIteratorType child_end(NodeRef N) {
    return N->getSuccessors().end();
  }
};

// Provide specializations of GraphTraits to be able to treat a VPRegionBlock as
// a graph of VPBasicBlocks... and to walk it in inverse order. Inverse order
// for a VPRegionBlock is considered to be when traversing the predecessor edges
// of a VPBlockBase instead of the successor edges.
//

template <> struct GraphTraits<Inverse<VPBlockBase *>> {
  typedef VPBlockBase *NodeRef;
  typedef SmallVectorImpl<VPBlockBase *>::iterator ChildIteratorType;

  static Inverse<VPBlockBase *> getEntryNode(Inverse<VPBlockBase *> B) {
    return B;
  }

  static inline ChildIteratorType child_begin(NodeRef N) {
    return N->getPredecessors().begin();
  }

  static inline ChildIteratorType child_end(NodeRef N) {
    return N->getPredecessors().end();
  }
};

} // namespace llvm

#endif // LLVM_TRANSFORMS_VECTORIZE_VPLAN_H
