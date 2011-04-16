//===-- StackFrame.h --------------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef liblldb_StackFrame_h_
#define liblldb_StackFrame_h_

// C Includes
// C++ Includes
// Other libraries and framework includes
// Project includes
#include "lldb/Core/Error.h"
#include "lldb/Core/Flags.h"
#include "lldb/Core/Scalar.h"
#include "lldb/Core/StreamString.h"
#include "lldb/Core/UserID.h"
#include "lldb/Core/ValueObjectList.h"
#include "lldb/Symbol/SymbolContext.h"
#include "lldb/Target/ExecutionContextScope.h"
#include "lldb/Target/StackID.h"

namespace lldb_private {

class StackFrame :
    public ExecutionContextScope
{
public:
    enum ExpressionPathOption
    {
        eExpressionPathOptionCheckPtrVsMember   = (1u << 0),
        eExpressionPathOptionsNoFragileObjcIvar = (1u << 1),
        eExpressionPathOptionsDynamicValue      = (1u << 2)
    };
    //------------------------------------------------------------------
    // Constructors and Destructors
    //------------------------------------------------------------------
    StackFrame (lldb::user_id_t frame_idx, 
                lldb::user_id_t concrete_frame_idx, 
                Thread &thread, 
                lldb::addr_t cfa, 
                lldb::addr_t pc, 
                const SymbolContext *sc_ptr);

    StackFrame (lldb::user_id_t frame_idx, 
                lldb::user_id_t concrete_frame_idx, 
                Thread &thread, 
                const lldb::RegisterContextSP &reg_context_sp, 
                lldb::addr_t cfa, 
                lldb::addr_t pc, 
                const SymbolContext *sc_ptr);
    
    StackFrame (lldb::user_id_t frame_idx, 
                lldb::user_id_t concrete_frame_idx, 
                Thread &thread, 
                const lldb::RegisterContextSP &reg_context_sp, 
                lldb::addr_t cfa, 
                const Address& pc, 
                const SymbolContext *sc_ptr);

    virtual ~StackFrame ();

    Thread &
    GetThread()
    { return m_thread; }

    const Thread &
    GetThread() const
    { return m_thread; }

    StackID&
    GetStackID();

    Address&
    GetFrameCodeAddress();
    
    void
    ChangePC (lldb::addr_t pc);

    const SymbolContext&
    GetSymbolContext (uint32_t resolve_scope);

    bool
    GetFrameBaseValue(Scalar &value, Error *error_ptr);

    Block *
    GetFrameBlock ();

    lldb::RegisterContextSP
    GetRegisterContext ();

    const lldb::RegisterContextSP &
    GetRegisterContextSP () const
    {
        return m_reg_context_sp;
    }

    VariableList *
    GetVariableList (bool get_file_globals);

    // See ExpressionPathOption enumeration for "options" values
    lldb::ValueObjectSP
    GetValueForVariableExpressionPath (const char *var_expr, uint32_t options, Error &error);

    bool
    HasDebugInformation ();

    const char *
    Disassemble ();

    void
    DumpUsingSettingsFormat (Stream *strm);
    
    void
    Dump (Stream *strm, bool show_frame_index, bool show_fullpaths);
    
    bool
    IsInlined ();

    uint32_t
    GetFrameIndex () const
    {
        return m_frame_index;
    }

    uint32_t
    GetConcreteFrameIndex () const
    {
        return m_concrete_frame_index;
    }
    
    lldb::ValueObjectSP
    GetValueObjectForFrameVariable (const lldb::VariableSP &variable_sp, bool use_dynamic);

    lldb::ValueObjectSP
    TrackGlobalVariable (const lldb::VariableSP &variable_sp, bool use_dynamic);
    
    //------------------------------------------------------------------
    // lldb::ExecutionContextScope pure virtual functions
    //------------------------------------------------------------------
    virtual Target *
    CalculateTarget ();

    virtual Process *
    CalculateProcess ();

    virtual Thread *
    CalculateThread ();

    virtual StackFrame *
    CalculateStackFrame ();

    virtual void
    CalculateExecutionContext (ExecutionContext &exe_ctx);
    
    lldb::StackFrameSP
    GetSP ();

protected:
    friend class StackFrameList;

    void
    SetSymbolContextScope (SymbolContextScope *symbol_scope);

    void
    UpdateCurrentFrameFromPreviousFrame (StackFrame &prev_frame);
    
    void
    UpdatePreviousFrameFromCurrentFrame (StackFrame &curr_frame);

    bool
    HasCachedData () const;
private:
    //------------------------------------------------------------------
    // For StackFrame only
    //------------------------------------------------------------------
    Thread &m_thread;
    uint32_t m_frame_index;
    uint32_t m_concrete_frame_index;
    lldb::RegisterContextSP m_reg_context_sp;
    StackID m_id;
    Address m_frame_code_addr;   // The frame code address (might not be the same as the actual PC for inlined frames) as a section/offset address
    SymbolContext m_sc;
    Flags m_flags;
    Scalar m_frame_base;
    Error m_frame_base_error;
    lldb::VariableListSP m_variable_list_sp;
    ValueObjectList m_variable_list_value_objects;  // Value objects for each variable in m_variable_list_sp
    StreamString m_disassembly;
    DISALLOW_COPY_AND_ASSIGN (StackFrame);
};

} // namespace lldb_private

#endif  // liblldb_StackFrame_h_
