#ifndef LLVM_TESTING_IR_PASSES_H
#define LLVM_TESTING_IR_PASSES_H
#include "llvm/Analysis/CGSCCPassManager.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Transforms/Scalar/LoopPassManager.h"

// Workaround for the gcc 7.1 bug PR80916.
#if defined(__GNUC__) && __GNUC__ > 6
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#if defined(__GNUC__) && __GNUC__ > 6
#pragma GCC diagnostic pop
#endif

namespace llvm {
using ::testing::Return;
using ::testing::Invoke;
using ::testing::_;

/// \brief A CRTP base for analysis mock handles
///
/// This class reconciles mocking with the value semantics implementation of
/// the
/// AnalysisManager. Analysis mock handles should derive from this class and
/// call \c setDefault() in their constroctur for wiring up the defaults
/// defined
/// by this base with their mock run() and invalidate() implementations.
template <typename DerivedT, typename IRUnitT,
          typename AnalysisManagerT = AnalysisManager<IRUnitT>,
          typename... ExtraArgTs>
class MockAnalysisHandleBase {
public:
  class Analysis : public AnalysisInfoMixin<Analysis> {
    friend AnalysisInfoMixin<Analysis>;
    friend MockAnalysisHandleBase;
    static AnalysisKey Key;

    DerivedT *Handle;

    Analysis(DerivedT &Handle) : Handle(&Handle) {
      static_assert(std::is_base_of<MockAnalysisHandleBase, DerivedT>::value,
                    "Must pass the derived type to this template!");
    }

  public:
    class Result {
      friend MockAnalysisHandleBase;

      DerivedT *Handle;

      Result(DerivedT &Handle) : Handle(&Handle) {}

    public:
      // Forward invalidation events to the mock handle.
      bool invalidate(IRUnitT &IR, const PreservedAnalyses &PA,
                      typename AnalysisManagerT::Invalidator &Inv) {
        return Handle->invalidate(IR, PA, Inv);
      }
    };

    Result run(IRUnitT &IR, AnalysisManagerT &AM, ExtraArgTs... ExtraArgs) {
      return Handle->run(IR, AM, ExtraArgs...);
    }
  };

  Analysis getAnalysis() { return Analysis(static_cast<DerivedT &>(*this)); }
  typename Analysis::Result getResult() {
    return typename Analysis::Result(static_cast<DerivedT &>(*this));
  }

protected:
  // FIXME: MSVC seems unable to handle a lambda argument to Invoke from within
  // the template, so we use a boring static function.
  static bool invalidateCallback(IRUnitT &IR, const PreservedAnalyses &PA,
                                 typename AnalysisManagerT::Invalidator &Inv) {
    auto PAC = PA.template getChecker<Analysis>();
    return !PAC.preserved() &&
           !PAC.template preservedSet<AllAnalysesOn<IRUnitT>>();
  }

  /// Derived classes should call this in their constructor to set up default
  /// mock actions. (We can't do this in our constructor because this has to
  /// run after the DerivedT is constructed.)
  void setDefaults() {
    ON_CALL(static_cast<DerivedT &>(*this),
            run(_, _, testing::Matcher<ExtraArgTs>(_)...))
        .WillByDefault(Return(this->getResult()));
    ON_CALL(static_cast<DerivedT &>(*this), invalidate(_, _, _))
        .WillByDefault(Invoke(&invalidateCallback));
  }
};

/// \brief A CRTP base for pass mock handles
///
/// This class reconciles mocking with the value semantics implementation of the
/// PassManager. Pass mock handles should derive from this class and
/// call \c setDefault() in their constroctur for wiring up the defaults defined
/// by this base with their mock run() and invalidate() implementations.
template <typename DerivedT, typename IRUnitT, typename AnalysisManagerT,
          typename... ExtraArgTs>
AnalysisKey MockAnalysisHandleBase<DerivedT, IRUnitT, AnalysisManagerT,
                                   ExtraArgTs...>::Analysis::Key;

template <typename DerivedT, typename IRUnitT,
          typename AnalysisManagerT = AnalysisManager<IRUnitT>,
          typename... ExtraArgTs>
class MockPassHandleBase {
public:
  class Pass : public PassInfoMixin<Pass> {
    friend MockPassHandleBase;

    DerivedT *Handle;

    Pass(DerivedT &Handle) : Handle(&Handle) {
      static_assert(std::is_base_of<MockPassHandleBase, DerivedT>::value,
                    "Must pass the derived type to this template!");
    }

  public:
    PreservedAnalyses run(IRUnitT &IR, AnalysisManagerT &AM,
                          ExtraArgTs... ExtraArgs) {
      return Handle->run(IR, AM, ExtraArgs...);
    }
  };

  Pass getPass() { return Pass(static_cast<DerivedT &>(*this)); }

protected:
  /// Derived classes should call this in their constructor to set up default
  /// mock actions. (We can't do this in our constructor because this has to
  /// run after the DerivedT is constructed.)
  void setDefaults() {
    ON_CALL(static_cast<DerivedT &>(*this),
            run(_, _, testing::Matcher<ExtraArgTs>(_)...))
        .WillByDefault(Return(PreservedAnalyses::all()));
  }
};

/// Mock handles for passes for the IRUnits Module, CGSCC, Function, Loop.
/// These handles define the appropriate run() mock interface for the respective
/// IRUnit type.
template <typename IRUnitT> struct MockPassHandle;
template <>
struct MockPassHandle<Loop>
    : MockPassHandleBase<MockPassHandle<Loop>, Loop, LoopAnalysisManager,
                         LoopStandardAnalysisResults &, LPMUpdater &> {
  MOCK_METHOD4(run,
               PreservedAnalyses(Loop &, LoopAnalysisManager &,
                                 LoopStandardAnalysisResults &, LPMUpdater &));
  MockPassHandle() { setDefaults(); }
};

template <>
struct MockPassHandle<Function>
    : MockPassHandleBase<MockPassHandle<Function>, Function> {
  MOCK_METHOD2(run, PreservedAnalyses(Function &, FunctionAnalysisManager &));

  MockPassHandle() { setDefaults(); }
};

template <>
struct MockPassHandle<LazyCallGraph::SCC>
    : MockPassHandleBase<MockPassHandle<LazyCallGraph::SCC>, LazyCallGraph::SCC,
                         CGSCCAnalysisManager, LazyCallGraph &,
                         CGSCCUpdateResult &> {
  MOCK_METHOD4(run,
               PreservedAnalyses(LazyCallGraph::SCC &, CGSCCAnalysisManager &,
                                 LazyCallGraph &G, CGSCCUpdateResult &UR));

  MockPassHandle() { setDefaults(); }
};

template <>
struct MockPassHandle<Module>
    : MockPassHandleBase<MockPassHandle<Module>, Module> {
  MOCK_METHOD2(run, PreservedAnalyses(Module &, ModuleAnalysisManager &));

  MockPassHandle() { setDefaults(); }
};

/// Mock handles for analyses for the IRUnits Module, CGSCC, Function, Loop.
/// These handles define the appropriate run() and invalidate() mock interfaces
/// for the respective IRUnit type.
template <typename IRUnitT> struct MockAnalysisHandle;
template <>
struct MockAnalysisHandle<Loop>
    : MockAnalysisHandleBase<MockAnalysisHandle<Loop>, Loop,
                             LoopAnalysisManager,
                             LoopStandardAnalysisResults &> {

  MOCK_METHOD3_T(run, typename Analysis::Result(Loop &, LoopAnalysisManager &,
                                                LoopStandardAnalysisResults &));

  MOCK_METHOD3_T(invalidate, bool(Loop &, const PreservedAnalyses &,
                                  LoopAnalysisManager::Invalidator &));

  MockAnalysisHandle<Loop>() { this->setDefaults(); }
};

template <>
struct MockAnalysisHandle<Function>
    : MockAnalysisHandleBase<MockAnalysisHandle<Function>, Function> {
  MOCK_METHOD2(run, Analysis::Result(Function &, FunctionAnalysisManager &));

  MOCK_METHOD3(invalidate, bool(Function &, const PreservedAnalyses &,
                                FunctionAnalysisManager::Invalidator &));

  MockAnalysisHandle<Function>() { setDefaults(); }
};

template <>
struct MockAnalysisHandle<LazyCallGraph::SCC>
    : MockAnalysisHandleBase<MockAnalysisHandle<LazyCallGraph::SCC>,
                             LazyCallGraph::SCC, CGSCCAnalysisManager,
                             LazyCallGraph &> {
  MOCK_METHOD3(run, Analysis::Result(LazyCallGraph::SCC &,
                                     CGSCCAnalysisManager &, LazyCallGraph &));

  MOCK_METHOD3(invalidate, bool(LazyCallGraph::SCC &, const PreservedAnalyses &,
                                CGSCCAnalysisManager::Invalidator &));

  MockAnalysisHandle<LazyCallGraph::SCC>() { setDefaults(); }
};

template <>
struct MockAnalysisHandle<Module>
    : MockAnalysisHandleBase<MockAnalysisHandle<Module>, Module> {
  MOCK_METHOD2(run, Analysis::Result(Module &, ModuleAnalysisManager &));

  MOCK_METHOD3(invalidate, bool(Module &, const PreservedAnalyses &,
                                ModuleAnalysisManager::Invalidator &));

  MockAnalysisHandle<Module>() { setDefaults(); }
};
}
#endif /* LLVM_TESTING_IR_PASSES_H */
