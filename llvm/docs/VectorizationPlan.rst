+++++
VPlan
+++++

Goal of initial VPlan patch
+++++++++++++++++++++++++++
The design and implementation of VPlan follow our RFC [10]_ and presentation
[11]_. The initial patch is designed to:

- be a *lightweight* NFC patch;
- show key aspects of VPlan's Hierarchical CFG concept;
- demonstrate how VPlan can

  * capture *all* current vectorization decisions: which instructions are to
    
    + be vectorized "on their own", or
    + be part of an interleave group, or
    + be scalarized, and optionally have scalar instances moved down to other
      basic blocks and under a condition; and
    + be packed or unpacked (at the definition rather than at its uses) to
      provide both scalarized and vectorized forms; and

  * represent all control-flow *within loop body* of vectorized code version.

- Be a step towards

  * aligning Cost step with Transformation step,
  * representing entire code being transformed,
  * adding optmizations:

    + optimize conditional scalarization further,
    + retaining uniform control-flow,
    + vectorizing outerloops,
    + and more.

Out of scope for initial patch:

- changing how a loop is checked if it can be vectorized - "Legal";
- changing how a loop is checked if it should be vectorized - "Cost".


==================
Vectorization Plan
==================

.. contents::
   :local:

Overview
========
The Vectorization Plan is an explicit recipe for describing a vectorization
candidate. It serves for both estimating the cost reliably and for performing
the translation, and facilitates dealing with multiple vectorization candidates.

The overall structure consists of:

1. One LoopVectorizationPlanner for each attempt to vectorize a loop or a loop
   nest.

2. A LoopVectorizationPlanner can construct, optimize and discard one or more
   VPlans, providing different ways to vectorize the loop or the loop nest.

3. Once the best VPlan is determined, including the best vectorization factor
   and unroll factor, this VPlan drives the vector code generation using a
   VPTransformState object.

4. Each VPlan represents the loop or the loop nest using a hierarchical CFG.

5. At the bottom level of the hierarchical CFG are VPBasicBlocks.

6. Each VPBasicBlock consists of one or more VPRecipes to generate Instructions
   for it.

Motivation
----------
The vectorization transformation can be rather complicated, involving several
potential alternatives, especially for outer loops [1]_ but also possibly for
innermost loops. These alternatives may have significant performance impact,
both positive and negative. A cost model is therefore employed to identify the
best alternative, including the alternative of avoiding any transformation
altogether.

The process of vectorization traditionally involves three major steps: Legal,
Cost, and Transform. This is the general case in LLVM's LoopVectorizer:

1. Legal Step: check if loop can be legally vectorized; encode constraints and
   artifacts if so.
2. Cost Step: compute the relative cost of vectorizing it along possible
   vectorization and unroll factors (VF, UF).
3. Transform Step: vectorize the loop according to best VF and UF.

This design, which works only directly on the original LLVM-IR, has some
implications:

1. Cost Step tries to predict what the vectorized loop will look like and how
   much it will cost, independently of what the Transform Step will eventually
   do. It's hard to keep the two in sync.
2. Cost Step essentially considers a single vectorization candidate. Any
   alternatives are immediately evaluately and resolved.
3. Legal Step does more than check for vectorizability; e.g., it records
   auxiliary artifacts such as collectLoopUniforms() and InterleaveInfo.
4. Transform Step first populates the single basic block of the vectorized loop
   and later revisits scalarized instructions to predicate them one by one, as
   needed.

The Vectorization Plan is designed to explicitly model a vectorization
candidate to overcome the above constraints, which is especially important for
the vectorization of outer-loops. This affects the overall process by
essentially splitting the Transform Step into a Plan Step and a Code-Gen Step:

1. Legal Step: check if loop can be legally vectorized; encode contraints and
   artifacts if so. Initiate Vectorization Plan showing how the loop can be
   vectorized only after passing Legal, to save redundant construction.
2. Plan Step:

   a. Build initial Vectorization Plans following the constraints and
      decisions taken by Legal.
   b. Explore ways to optimize the vectorization plan, complying with
      all legal constraints, possibly constructing several plans following
      tentative vectorization decisions.
3. Cost Step: compute the relative cost of each plan. This step can be applied
   repeatedly by Plan Step 2.b.
4. Code-Gen Step: materialize the best plan. Note that only this step modifies
   the IR, as in the current Loop Vectorizer.

The Cost Step can also be split into an Early-Pruning Step(s) and a
"Cost-Gen" Step, where the former applies quick yet inaccurate estimates to
prune obviously-unpromising candidates, and the latter applies more accurate
estimates based on a full Plan.

One can compare with LLVM's existing SLP vectorizer, where TSLP [3]_ adds
Step 2.b.

As the scope of vectorization grows from innermost to outer loops, so do the
uncertainty and complexity of each step. One way to mitigate the shortcomings
of the Legal and Cost steps is to rely on programmers to indicate which loops
can and/or should be vectorized. This is implicit for certain loops in
data-parallel languages such as OpenCL [4]_, [5]_ and explicit in others such as
OpenMP [6]_. This design to extend the Loop Vectorizer to outer loops supports
and raises the importance of explicit vectorization beyond the current
capabilities of Clang and LLVM. Namely, from currently forcing the
vectorization of innermost loops according to prescribed width and/or
interleaving count, to supporting OpenMP's "#pragma omp simd" construct and
associated clauses, including vectorizing across function boundaries [2]_.

References
----------
.. [1] "Outer-loop vectorization: revisited for short SIMD architectures", Dorit
    Nuzman and Ayal Zaks, PACT 2008.

.. [2] "Proposal for function vectorization and loop vectorization with function
    calls", Xinmin Tian, [`cfe-dev
    <http://lists.llvm.org/pipermail/cfe-dev/2016-March/047732.html>`_].,
    March 2, 2016.
    See also `review <https://reviews.llvm.org/D22792>`_.

.. [3] "Throttling Automatic Vectorization: When Less is More", Vasileios
    Porpodas and Tim Jones, PACT 2015 and LLVM Developers' Meeting 2015.

.. [4] "Intel OpenCL SDK Vectorizer", Nadav Rotem, LLVM Developers' Meeting 2011.

.. [5] "Automatic SIMD Vectorization of SSA-based Control Flow Graphs", Ralf
    Karrenberg, Springer 2015. See also "Improving Performance of OpenCL on
    CPUs", LLVM Developers' Meeting 2012.

.. [6] "Compiling C/C++ SIMD Extensions for Function and Loop Vectorization on
    Multicore-SIMD Processors", Xinmin Tian and Hideki Saito et al.,
    IPDPSW 2012.

.. [7] "Exploiting mixed SIMD parallelism by reducing data reorganization
    overhead", Hao Zhou and Jingling Xue, CGO 2016.

.. [8] "Register Allocation via Hierarchical Graph Coloring", David Callahan and
    Brian Koblenz, PLDI 1991

.. [9] "Structural analysis: A new approach to flow analysis in optimizing
    compilers", M. Sharir, Journal of Computer Languages, Jan. 1980

.. [10] "RFC: Extending LV to vectorize outerloops", [`llvm-dev
    <http://lists.llvm.org/pipermail/llvm-dev/2016-September/105057.html>`_],
    September 21, 2016.

.. [11] "Extending LoopVectorizer towards supporting OpenMP4.5 SIMD and outer
    loop auto-vectorization", Hideki Saito, `LLVM Developers' Meeting 2016
    <https://www.youtube.com/watch?v=XXAvdUwO7kQ>`_, November 3, 2016.

Examples
--------
An example with a single predicated scalarized instruction - integer division:

.. code-block:: c

  void foo(int* a, int b, int* c) {
    #pragma simd
    for (int i = 0; i < 10000; ++i)
      if (a[i] > 777)
        a[i] = b - (c[i] + a[i] / b);
  }


IR Dump Before Loop Vectorization:

.. code-block:: LLVM
   :emphasize-lines: 6,11

   for.body:                                         ; preds = %for.inc, %entry
     %indvars.iv = phi i64 [ 0, %entry ], [ %indvars.iv.next, %for.inc ]
     %arrayidx = getelementptr inbounds i32, i32* %a, i64 %indvars.iv
     %0 = load i32, i32* %arrayidx, align 4, !tbaa !1
     %cmp1 = icmp sgt i32 %0, 777
     br i1 %cmp1, label %if.then, label %for.inc

   if.then:                                          ; preds = %for.body
     %arrayidx3 = getelementptr inbounds i32, i32* %c, i64 %indvars.iv
     %1 = load i32, i32* %arrayidx3, align 4, !tbaa !1
     %div = sdiv i32 %0, %b
     %add.neg = sub i32 %b, %1
     %sub = sub i32 %add.neg, %div
     store i32 %sub, i32* %arrayidx, align 4, !tbaa !1
     br label %for.inc

   for.inc:                                          ; preds = %for.body, %if.then
     %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
     %exitcond = icmp eq i64 %indvars.iv.next, 10000
     br i1 %exitcond, label %for.cond.cleanup, label %for.body

The VPlan that is built initially:

.. image:: VPlanPrinter.png

Design Guidelines
=================
1. Analysis-like: building and manipulating the Vectorization Plan must not
   modify the IR. In particular, if a VPlan is discarded
   compilation should proceed as if the VPlan had not been built.

2. Support all current capabilities: the Vectorization Plan must be capable of
   representing the exact functionality of LLVM's existing Loop Vectorizer.
   In particular, the transition can start with an NFC patch.
   In particular, VPlan must support efficient selection of VF and/or UF.

3. Align Cost & CodeGen: the Vectorization Plan must serve both the cost
   model and the code generation phases, where the cost estimation must
   evaluate the to-be-generated code reliably.

4. Support vectorizing additional constructs:

   a. vectorization of Outer-loops.
      In particular, VPlan must be able to represent the control-flow of a
      vectorized loop which may include multiple basic-blocks and nested loops.
   b. SLP vectorization.
   c. Combinations of the above, including nested vectorization: vectorizing
      both an inner loop and an outerloop at the same time (each with its own
      VF and UF), mixed vectorization: vectorizing a loop and SLP patterns
      inside [7]_, (re)vectorizing vector code.

5. Support multiple candidates efficiently:
   In particular, similar candidates related to a range of possible VF's and
   UF's must be represented efficiently.
   In particular support potential versionings efficiently.

6. Compact: the Vectorization Plan must be efficient and provide as compact a
   representation as possible. In particular where the transformation is
   straightfoward, and where the plan is to reuse existing IR (e.g.,
   leftover iterations).

VPlan Classes: Definitions
==========================

:VPlan:
  A recipe for generating a vectorized version from a given IR code.
  Takes a "scenario-based approach" to vectorization planning.
  Given IR code required to be SESE, mainly to simplify dominance
  information. This vectorized version is represented using a Hierarchical CFG.

:Hierarchical CFG:
  A control-flow graph whose nodes are basic-blocks or Hierarchical CFG's.
  The Hierarchical CFG data structure we use is similar to the Tile Tree [8]_,
  where cross-Tile edges are lifted to connect Tiles instead of the original
  basic-blocks as in Sharir [9]_, promoting the Tile encapsulation. We use the
  terms Region and Block rather than Tile [8]_ to avoid confusion with loop
  tiling.

:VPBasicBlock:
  Serves as the leaf of the Hierarchical CFG. Represents a sequence of
  instructions that will appear consecutively in a basic block of the vectorized
  version. The instructions of such a basic block originate from one or more
  VPBasicBlocks.
  The VPBasicBlock takes care of the control-flow
  relations with other VPBasicBlock's and Regions.
  Holds a sequence of zero or more
  VPRecipe's that take care of representing the instructions.
  A VPBasicBlock that holds no VPRecipe's represents no instructions; this
  may happen, e.g., to support disjoint Regions and to ensure Regions have a
  single exit, possibly an empty one.

:VPRecipeBase:
  A base class describing one or more instructions that will appear
  consecutively in the vectorized version, based on Instructions from the given
  IR.
  These Instructions are referred to as the "Ingredients" of the Recipe.
  A Recipe specifies how its ingredients are to be vectorized: e.g.,
  copy or reuse them as uniform, scalarize or vectorize them according to an
  enclosing loop dimension, vectorize them according to internal SLP dimension.

  **Design principle:** in order to reason about how to vectorize an Instruction
  or how much it would cost, one has to consult the VPRecipe holding it.

  **Design principle:** when a sequence of instructions conveys additional
  information as a group, we use a VPRecipe to encapsulate them and attach
  this information to the VPRecipe. For instance a VPRecipe can model an
  interleave group of loads or stores with additional information for
  calculating their cost and performing code-gen, as a group.

  **Design principle:** where possible a VPRecipe should reuse the existing
  container of its ingredients. A new containter should be opened on-demand,
  e.g., to facilitate changing the order of Instructions between original
  and vectorized versions.

:VPOneByOneRecipeBase:
  Represents recipes which transform each Instruction in their Ingredients
  independently, in order.
  The Ingredients are a sub-sequence of original Instructions, which reside in
  the same IR BasicBlock and in the same order. The Ingredients are
  accessed by a pointer to the first and last Instruction in their original IR
  basic block. Serves as a base class for the concrete sub-classes
  VPScalarizeOneByOneRecipe and VPVectorizeOneByOneRecipe.

:VPScalarizeOneByOneRecipe:
  A concrete VPRecipe which scalarizes each ingredient, generating either
  instances of lane 0 for a uniform instruction, or instances for a range of
  lanes otherwise.

:VPVectorizeOneByOneRecipe:
  A concrete VPRecipe which vectorizes each ingredient.

:VPInterleaveRecipe:
  A concrete VPRecipe which transforms an interleave group of loads or stores
  into one wide load/store and shuffles.

:VPConditionBitRecipeBase:
  A base class for VPRecipes which provide the condition bit feeding a
  conditional branch. Such cases correspond to scalarized or uniform branches.

:VPExtractMaskBitRecipe:
  A concrete VPRecipe which represents the extraction of a bit from a mask,
  needed when scalarizing a conditional branch.
  Such branches are needed to guard scalarized and predicated instructions.

:VPMergeScalarizeBranchRecipe:
  A concrete VPRecipe which represents Phi's needed when control converges back
  from a scalarized branch.
  Such phi's are needed to merge live-out values that are set under a
  scalarized branch. They can be scalar or vector, depending on the user of the
  live-out value.

:VPWidenIntInductionRecipe:
  A concrete VPRecipe which widens integer reductions, producing their vector
  values and computing the necessary values for producing their scalar values.
  The scalar values themselves are generated, possibly elsewhere, by the
  complementing VPBuildScalarStepsRecipe.

:VPBuildScalarStepsRecipe:
  A concrete VPRecipe complemeting the handling of integer induction variables,
  responsible for generating the scalar values used by the IV's scalar users.

:VPRegionBlock:
  A collection of VPBasicBlocks and VPRegionBlocks which form a
  single-entry-single-exit subgraph of the CFG in the vectorized code.

  **Design principle:** When some additional information relates to an SESE set
  of VPBlocks, we use a VPRegionBlock to wrap them and attach the information to
  it. For example, a VPRegionBlock can be used to indicate that a scalarized
  SESE region is to be replicated. It is also designed to serve predicating
  divergent branches while retaining uniform branches as much as possible /
  desirable, and represent inner loops.

:VPBlockBase:
  The building block of the Hierarchical CFG. A VPBlockBase can be either a
  VPBasicBlock or a VPRegionBlock.
  A VPBlockBase may indicate that its contents are
  to be replicated several times. This is designed to support scalarizing
  VPBlockBases which generate VF replicas of their instructions, which in turn
  remain scalar. And to do so using a single VPlan for multiple candidate VF's.

:VPTransformState:
  Stores information used for code generation, passed from the Planner to its
  selected VPlan for execution, and used to pass additional information down
  from VPBlocks to the VPRecipes.

:VPlanUtils:
  Contains a collection of methods for the construction and modification of
  abstract VPlans.

:VPlanUtilsLoopVectorizer:
  Derived from VPlanUtils, providing additional methods for the construction and
  modification of VPlans.

:LoopVectorizationPlanner:
  The object in charge of creating and manipulating VPlans for a given IR code.


VPlan Classes: Diagram
======================

The classes of VPlan with main fields and methods; sub-classes of VPRecipeBase
are shown in a separate figure:

.. image:: VPlanUML.png


The class hierarchy of VPlan's VPRecipeBase class:

.. image:: VPlanRecipesUML.png


Integration with LoopVectorize.cpp/processLoop()
================================================

Here's the integration within LoopVectorize.cpp's existing flow, in
LoopVectorizePass::processLoop(Loop \*L):

1. Plan only after passing all early bail-outs:

   a. including those that take place after Legal, which is kept intact;
   b. including those that use the Cost Model - refactor it slightly to expose
      its MaxVF upper bound and canVectorize() early exit:

.. code-block:: c++

  // Check if the target supports potentially unsafe FP vectorization.
  // FIXME: Add a check for the type of safety issue (denormal, signaling)
  // for the target we're vectorizing for, to make sure none of the
  // additional fp-math flags can help.
  if (Hints.isPotentiallyUnsafe() &&
      TTI->isFPVectorizationPotentiallyUnsafe()) {
    DEBUG(dbgs() << "LV: Potentially unsafe FP op prevents vectorization.\n");
    ORE->emit(
        createMissedAnalysis(Hints.vectorizeAnalysisPassName(), "UnsafeFP", L)
        << "loop not vectorized due to unsafe FP support.");
    emitMissedWarning(F, L, Hints, ORE);
    return false;
  }

  if (!CM.canVectorize(OptForSize))
    return false;

  // Early prune excessive VF's
  unsigned MaxVF = CM.computeMaxVectorizationFactor(OptForSize);

  // If OptForSize, MaxVF is the only VF we consider. Abort if it needs a tail.
  if (OptForSize && CM.requiresTail(MaxVF))
    return false;

2. Plan:

   a. build VPlans for relevant VF's and optimize them,
   b. compute best cost using Cost Model as before,
   c. compute best interleave-count using Cost Model as before. Above two
      steps are refactored into LVP.plan() (see below):

.. code-block:: c++

  // Use the planner.
  LoopVectorizationPlanner LVP(L, LI, TLI, TTI, &LVL, &CM);

  // Get user vectorization factor.
  unsigned UserVF = Hints.getWidth();

  // Select the vectorization factor.
  LoopVectorizationCostModel::VectorizationFactor VF =
      LVP.plan(OptForSize, UserVF, MaxVF);
  bool VectorizeLoop = (VF.Width > 1);

  std::pair<StringRef, std::string> VecDiagMsg, IntDiagMsg;

  if (!UserVF && !VectorizeLoop) {
    DEBUG(dbgs() << "LV: Vectorization is possible but not beneficial.\n");
    VecDiagMsg = std::make_pair(
        "VectorizationNotBeneficial",
        "the cost-model indicates that vectorization is not beneficial");
  }

  // Select the interleave count.
  unsigned IC = CM.selectInterleaveCount(OptForSize, VF.Width, VF.Cost);

  // Get user interleave count.
  unsigned UserIC = Hints.getInterleave();

3. Transform:

   a. invoke an Unroller to unroll the loop (as before), or
   b. invoke LVP.executeBestPlan() to vectorize the loop:

.. code-block:: c++

  if (!VectorizeLoop) {
    assert(IC > 1 && "interleave count should not be 1 or 0");
    // If we decided that it is not legal to vectorize the loop, then
    // interleave it.
    InnerLoopUnroller Unroller(L, PSE, LI, DT, TLI, TTI, AC, ORE, IC, &LVL,
                               &CM);
    Unroller.vectorize();

    ORE->emit(OptimizationRemark(LV_NAME, "Interleaved", L->getStartLoc(),
                                 L->getHeader())
              << "interleaved loop (interleaved count: "
              << NV("InterleaveCount", IC) << ")");
  } else {

    // If we decided that it is \* legal \* to vectorize the loop, then do it.
    InnerLoopVectorizer LB(L, PSE, LI, DT, TLI, TTI, AC, ORE, VF.Width, IC,
                           &LVL, &CM);

    LVP.executeBestPlan(LB);

    ++LoopsVectorized;

    // Add metadata to disable runtime unrolling a scalar loop when there are
    // no runtime checks about strides and memory. A scalar loop that is
    // rarely used is not worth unrolling.
    if (!LB.areSafetyChecksAdded())
      AddRuntimeUnrollDisableMetaData(L);

    // Report the vectorization decision.
    ORE->emit(OptimizationRemark(LV_NAME, "Vectorized", L->getStartLoc(),
                                 L->getHeader())
              << "vectorized loop (vectorization width: "
              << NV("VectorizationFactor", VF.Width)
              << ", interleaved count: " << NV("InterleaveCount", IC) << ")");
  }

  // Mark the loop as already vectorized to avoid vectorizing again.
  Hints.setAlreadyVectorized();

4. Plan, refactored into LVP.plan():

   a. build VPlans for relevant VF's and optimize them,
   b. compute best cost using Cost Model as before:

.. code-block:: c++

  LoopVectorizationCostModel::VectorizationFactor
  LoopVectorizationPlanner::plan(bool OptForSize, unsigned UserVF,
                                 unsigned MaxVF) {
    if (UserVF) {
      DEBUG(dbgs() << "LV: Using user VF " << UserVF << ".\n");
      if (UserVF == 1)
        return {UserVF, 0};
      assert(isPowerOf2_32(UserVF) && "VF needs to be a power of two");
      // Collect the instructions (and their associated costs) that will be more
      // profitable to scalarize.
      CM->collectInstsToScalarize(UserVF);
      buildInitialVPlans(UserVF, UserVF);
      DEBUG(printCurrentPlans("Initial VPlans", dbgs()));
      optimizePredicatedInstructions();
      DEBUG(printCurrentPlans("After optimize predicated instructions",dbgs()));
      return {UserVF, 0};
    }
    if (MaxVF == 1)
      return {1, 0};
  
    assert(MaxVF > 1 && "MaxVF is zero.");
    // Collect the instructions (and their associated costs) that will be more
    // profitable to scalarize.
    for (unsigned i = 2; i <= MaxVF; i = i+i)
      CM->collectInstsToScalarize(i);
    buildInitialVPlans(2, MaxVF);
    DEBUG(printCurrentPlans("Initial VPlans", dbgs()));
    optimizePredicatedInstructions();
    DEBUG(printCurrentPlans("After optimize predicated instructions", dbgs()));
    // Select the optimal vectorization factor.
    return CM->selectVectorizationFactor(OptForSize, MaxVF);
  }
