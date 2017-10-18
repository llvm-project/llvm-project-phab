=========================================
Tutorial for building refactoring actions
=========================================

.. warning::

  This tutorial talks about a work-in-progress library in Clang.
  Some of the described features might not be available yet in trunk, but should
  be there in the near future.

This document is intended to show how to build refactoring actions that
perform source-to-source transformations and integrate with editors that
use ``libclang/clangd`` and the ``clang-refactor`` command-line refactoring
tool. The document uses Clang's `refactoring engine <RefactoringEngine.html>`_
and the `LibTooling <LibTooling.html>`_ library.

In order to work on the compiler, you need some basic knowledge of the
abstract syntax tree (AST). To this end, the reader is encouraged to
skim the :doc:`Introduction to the Clang
AST <IntroductionToTheClangAST>`.

How to obtain & build Clang
===========================

Before working on the refactoring action, you'll need to download and build
LLVM and Clang. The
:doc:`AST matchers tutorial<LibASTMatchersTutorial.html#step-0-obtaining-clang>`
provides a section that describes how to obtain and build LLVM and Clang.

Implementing a local source transformation
==========================================

This tutorial walks through an implementation of the
"flatten nested if statements" refactoring operation. This action can take a
selected ``if`` statement that's nested in another ``if`` and merge the two
into ``if``s one just one ``if``. For example, the following code:

.. code-block:: c++

      int computeArrayOffset(int x, int y) {
        if (x >= 0) {
          if (y >= 0) {
            return x * 4 + y;
          }
        }
        throw std::runtime_error("negative indices");
      }


becomes:

.. code-block:: c++

      int computeArrayOffset(int x, int y) {
        if (x >= 0 && y >= 0)
          return x * 4 + y;
        throw std::runtime_error("negative indices");
      }

after this refactoring.

This refactoring is a local source transformation, since it only requires one
translation unit in order to perform the refactoring.

Before diving into a potential implementation of a refactoring, let's try
to break it down. This operation is a "flatten" operation, which in theory
could be applied to other types of directives, like the ``switch`` statements
or maybe even loops. We can think of the particular flavor of the refactoring
that merges the ``if`` statements as just one refactoring operation that's
contained in the "flatten" refactoring action. The refactoring library
encourages you to think in terms like these. In particular, it encourages
developers to decompose refactorings into a single action that contains one or
more operation that either produce similar results or have different refactoring
modes of operation. The different operations should still be identifiable
by just one name, like "flatten". For the rest of the tutorial I will talk
about the decomposed refactoring using terms like the "flatten" refactoring
action and the "flatten nested if statements" refactoring operation.

Now that we've established and decomposed the refactoring action, let's go
ahead and look at how it can be implemented.

Step 1: Create the "flatten nested if statements" refactoring operation
-----------------------------------------------------------------------

Let's start our implementation with the refactoring operation class that will
be responsible for making the source changes.

We can start by adding an implementation file in ``lib/Tooling/Refactoring``.
Let's call it something like ``FlattenIfStatements.cpp``. Don't forget to add
it to the ``CMakeLists.txt`` file that's located in the
``lib/Tooling/Refactoring`` directory.

The file will hold the implementation of our refactoring operation. Let's
take a look at the outline of the operation class:

.. code-block:: c++

      #include "clang/Refactoring/RefactoringActions.h"
      #include "clang/Refactoring/RefactoringActionRules.h"

      using namespace clang;
      using namespace tooling;

      namespace {

      class FlattenNestedIfStatements final : public SourceChangeRefactoringRule {
      public:
        FlattenNestedIfStatements(CodeRangeASTSelection CodeSelection) :
          CodeSelection(std::move(CodeSelection)) { }

        Expected<AtomicChanges>
        createSourceReplacements(RefactoringRuleContext &Context) override;
      private:
        CodeRangeASTSelection CodeSelection;
      };

      } // end anonymous namespace

The ``FlattenNestedIfStatements`` refactoring operation class derives from
the ``SourceChangeRefactoringRule`` class because it's a local refactoring
operation. The refactoring library supports a number of different kinds of
refactoring operations, which are describes in the
:doc:`rule types <RefactoringEngine.html#rule-types>`_ section of the
refactoring engine documentation.

The constructor of the ``FlattenNestedIfStatements`` takes in the inputs
that are necessary to perform the refactoring. The lifecycle of an instance
of this class is managed automatically by the refactoring library, and the
initiation refactoring stage produces the input values that are passed to
the constructor. More specifically, each refactoring operation is
associated with a set of initiation requirements which produce the input
values. We will take a look at the requirements that we need in the next
section of this tutorial.

This class also declares the actual refactoring function that's responsible
for creating the source changes: ``createSourceReplacements``. The source
changes are represented using a list of ``AtomicChange`` values. Let's take a
look at one possible implementation of this function. This example is
simplified as it doesn't account for all the possible variations of the AST,
but still manages to illustrate the point:

.. code-block:: c++

      Expected<AtomicChanges>
      FlattenNestedIfStatements::createSourceReplacements(RefactoringRuleContext &Context) override {
        // Assume that we've selected a nested 'if' statement.
        // We'll talk about how the selection is verified in the next section.
        const IfStmt *NestedIf = cast<IfStmt>(CodeSelection[0]);
        // The parent if should be the second parent as the first is the
        // compound statement.
        const IfStmt *ParentIf = CodeSelection.getParentNode(/*Nth=*/1).get<IfStmt>();

        // We'd like to move the condition of the nested 'if' into the parent if.
        const Expr *NestedCond = NestedIf->getCond();
        const Expr *ParentCond = ParentIf->getCond();

        // Take the source range of the condition of the nested 'if'.
        StringRef NestedCondStr = Lexer::getSourceText(
           CharSourceRange::getTokenRange(NestedCond->getSourceRange()),
           Context.getASTContext().getSourceManager(),
           Context.getASTContext().getLangOpts());

        AtomicChanges Changes;
        AtomicChange Change(Context.getSources(), CodeSelection[0]->getLocStart());
        // And insert it with '&&' into the condition of the parent 'if'.
        Change.insert(Context.getSources(), ParentCond->getLocEnd(),
                      (llvm::Twine(" && ") + NestedCondStr).str());

        // Finally, remove the nested if.
        Change.replace(Context.getSources(),
                       CharSorceRange::getTokenRange(NestedIf->getLocStart(),
                          NestedIf->getThen()->getLocStart()), "");
        Change.replace(Context.getSources(),
                       CharSorceRange::getTokenRange(NestedIf->getLocEnd(),
                          NestedIf->getLocEnd()), "");

        Changes.push_back(std::move(Change));
        return std::move(Changes);
      }


Step 2: Create the "flatten" refactoring action
-----------------------------------------------

Now that we've have the refactoring operation class, let's create the
"flatten" refactoring action. We can add this class to our
``FlattenIfStatements.cpp`` implementation file:

.. code-block:: c++

      class FlattenCodeRefactoring final : public RefactoringAction {
      public:
        StringRef getCommand() const override { return "flatten"; }

        StringRef getDescription() const override {
          return "Flattens nested statements like 'if' into one";
        }

        RefactoringActionRules createActionRules() const override {
          RefactoringActionRules Rules;
          // The refactoring action rule wraps around the
          // FlattenNestedIfStatements operation.
          Rules.push_back(
              createRefactoringActionRule<FlattenNestedIfStatements>(
                  NestedIfSelectionRequirement()));
          return Rules;
        }
      };


The ``FlattenCodeRefactoring`` class provides a high-level description
of the refactoring, in particular the name and the description of the
subcommand that's available in ``clang-refactor``.

This class also implements the ``createActionRules`` function that creates the
rule that wraps around the ``FlattenNestedIfStatements`` operation. The
``FlattenNestedIfStatements`` class is passed as a template parameter to
the function. This call also passes in another ``NestedIfSelectionRequirement``
value to the function. This is our custom initiation requirement that
actually verifies that a nested ``if`` has been selected. If this requirement
is satisfied, the refactoring library constructs the
``FlattenNestedIfStatements`` and passes in the input ``CodeRangeASTSelection``
that's produced by this requirement class to the constructor. However, the
refactoring either fails or is completely unavailable when this requirement
is not satisfied.

Initiation requirements
^^^^^^^^^^^^^^^^^^^^^^^

We mentioned that the ``NestedIfSelectionRequirement`` class is a custom
initiation requirement. Let's take a look at how it can be implemented:

.. code-block:: c++

      class NestedIfSelectionRequirement final
          : final CodeRangeSelectionRequirement {
      public:
        Expected<CodeRangeASTSelection>
        evalute(RefactoringRuleContext &Context) const {
          // Get the selection from the base class.
          Expected<CodeRangeASTSelection> Selection =
              CodeRangeSelectionRequirement::evalute(Context);
          if (!Selection)
            return Selection.takeError();

          CodeRangeASTSelection &Code = *Selection;

          // Verify that we've selected an 'if' statement (without 'else')
          // and return a builtin error if no such 'if' is selected.
          if (Code.size() != 1 || !isa<IfStmt>(Code[0]) ||
              cast<IfStmt>(Code[0])->getElse() != nullptr)
            return Context.createDiagnosticError(
                       diag::err_refactor_selection_invalid_ast);

          // Verify that the selected 'if' is nested in another 'if' (that
          // doesn't have an 'else') and return a builtin error if there is
          // not such parent 'if'.
          const CompoundStmt *ThenBody = CodeSelection.getParentNode().get<CompoundStmt>();
          const IfStmt *ParentIf = CodeSelection.getParentNode(/*Nth=*/1).get<IfStmt>();
          if (!ThenBody || !ParentIf || ParentIf->getThen() != ThenBody ||
              ParentIf->getElse() != nullptr)
              return Context.createDiagnosticError(
                         diag::err_refactor_selection_invalid_ast);

          return Selection;
       }
     };

The ``NestedIfSelectionRequirement`` class implements the custom initiation
logic that verifies correctness of the selected ``if``. It derives from the
``CodeRangeSelectionRequirement`` because this is a source selection requirement
that requires either the ``-selection`` option in ``clang-refactor``, or an
actual selection in an editor.

The ``evaluate`` member function implements the initiation logic. Firstly, it
gets the selection from the base class. Then it verifies that an ``if``
statement is selected by inspecting the ``CodeRangeASTSelection`` value, and
then verifies that the ``if`` has a corresponding parent ``if`` as well.

Step 3: Register the action
---------------------------

The "flatten" refactoring action has to be registered before it can be
used in ``clang-refactor`` and other clients. This process is rather simple, and
has two steps only:

1) The refactoring action should have a corresponding entry in the refactoring
   action registry. We can add "flatten" to the registry by adding the following
   line to the ``RefactoringActionRegistry.def`` file that's located in the
   ``include/clang/Tooling/Refactoring`` directory:

.. code-block:: c++

      REFACTORING_ACTION(Flatten)

2) In addition to the entry in the registry, we have to provide a factory
   function that can create an instance of the action class. The following
   function should be sufficient for the "flatten" action:

.. code-block:: c++

      std::unique_ptr<RefactoringAction> createFlattenAction() {
        return llvm::make_unique<FlattenCodeRefactoring>();
      }

Once these steps are complete, the "flatten" refactoring action and the
"flatten nested if statements" operation are automatically supported by the
``clang-refactor`` command-line tool.

Step 4: Integrate with the editor
---------------------------------

In addition to ``clang-refactor`` support, we would like to integrate the
"flatten nested if statements" operation with an editor client that supports
Clang's refactoring library. We have to complete the following two things in
order to achieve this integration:

1) A new editor command will enable us to use the refactoring operation in
   an editor (probably through a menu item). We can add a new command for
   our operation by adding the following line to the
   ``EditorCommandRegistry.def`` file that's located in the
   ``include/clang/Tooling/Refactoring`` directory:

.. code-block:: c++

      REFACTORING_EDITOR_COMMAND(FlattenIf, "Flatten Nested If")

2) Once the editor command is defined, it has to be bound to our refactoring
   operation. We have to modify the ``createActionRules`` member function in
   the ``FlattenCodeRefactoring`` class to bind the
   "flatten nested if statements" operation to the editor command:

.. code-block:: c++

        RefactoringActionRules createActionRules() const override {
          RefactoringActionRules Rules;
          // The refactoring action rule wraps around the
          // FlattenNestedIfStatements operation.
          Rules.push_back(
              EditorCommand::FlattenIf().bind( // This line is added!
              createRefactoringActionRule<FlattenNestedIfStatements>(
                  NestedIfSelectionRequirement())));
          return Rules;
        }

Once these steps are complete, the "flatten nested if statements" refactoring
operation becomes supported by editor services like ``libclang`` or ``clangd``
and is available in editors that use these services.

Testing
-------

The ``clang-refactor`` command-line tool can be used to test a refactoring
operation like "flatten nested if statements". The tool has a special testing
mode that can be enabled using a special selection argument. This is an
example of a test you can use for the "flatten" action:

.. code-block:: c++

      // RUN: clang-refactor flatten -selection=test:%s %s -- | FileCheck %s

      int computeArrayOffset(int x, int y) {
        if (x >= 0) {
          /*range=->+2:5*/if (y >= 0) {
            return x * 4 + y;
          }
          // CHECK: 1 '' results:
          //
          // CHECK:      int computeArrayOffset(int x, int y) {
          // CHECK-NEXT:   if (x >= 0 && y >= 0)
          // CHECK-NEXT:     return x * 4 + y;
          // CHECK-NEXT:    throw std::runtime_error("negative indices");
          // CHECK-NEXT:  }
        }
        throw std::runtime_error("negative indices");
      }

The test mode in ``clang-refactor`` scans the test file and looks for the
``range=`` commands in comments. These commands specify the selection ranges
that are used for different refactoring invocations. The exact syntax for
the range command is specified in comments to the ``findTestSelectionRanges``
function in ``TestSupport.h`` in the implementation of ``clang-refactor``.
