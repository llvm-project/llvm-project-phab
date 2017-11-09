//===- TemplateInstCallback.h - Template Instantiation Callback - C++ --===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===//
//
// This file defines the TemplateInstantiationCallback class, which is the
// base class for callbacks that will be notified at template instantiations.
//
//===---------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TEMPLATE_INST_CALLBACK_H
#define LLVM_CLANG_TEMPLATE_INST_CALLBACK_H

#include "clang/Sema/Sema.h"

namespace clang {

/// \brief This is a base class for callbacks that will be notified at every
/// template instantiation.
class TemplateInstantiationCallback {
public:
  virtual ~TemplateInstantiationCallback() {}

  /// \brief Called before doing AST-parsing.
  virtual void initialize(const Sema &TheSema) = 0;

  /// \brief Called after AST-parsing is completed.
  virtual void finalize(const Sema &TheSema) = 0;

  /// \brief Called when instantiation of a template just began.
  virtual void atTemplateBegin(const Sema &TheSema,
                               const Sema::CodeSynthesisContext &Inst) = 0;

  /// \brief Called when instantiation of a template is just about to end.
  virtual void atTemplateEnd(const Sema &TheSema,
                             const Sema::CodeSynthesisContext &Inst) = 0;
};

template <class TemplateInstantiationCallbackPtrs>
void initialize(TemplateInstantiationCallbackPtrs& Callbacks_, const Sema &TheSema)
{
  for (auto& C : Callbacks_)
  {
    if (C)
    {
      C->initialize(TheSema);
    }
  }
}

template <class TemplateInstantiationCallbackPtrs>
void finalize(TemplateInstantiationCallbackPtrs& Callbacks_, const Sema &TheSema)
{
  for (auto& C : Callbacks_)
  {
    if (C)
    {
      C->finalize(TheSema);
    }
  }
}

template <class TemplateInstantiationCallbackPtrs>
void atTemplateBegin(TemplateInstantiationCallbackPtrs& Callbacks_, const Sema &TheSema, const Sema::CodeSynthesisContext &Inst)
{
  for (auto& C : Callbacks_)
  {
    if (C)
    {
      C->atTemplateBegin(TheSema, Inst);
    }
  }
}

template <class TemplateInstantiationCallbackPtrs>
void atTemplateEnd(TemplateInstantiationCallbackPtrs& Callbacks_, const Sema &TheSema, const Sema::CodeSynthesisContext &Inst)
{
  for (auto& C : Callbacks_)
  {
    if (C)
    {
      C->atTemplateEnd(TheSema, Inst);
    }
  }
}

}

#endif
