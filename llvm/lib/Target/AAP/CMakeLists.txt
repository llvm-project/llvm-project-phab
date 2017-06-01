set(LLVM_TARGET_DEFINITIONS AAP.td)

tablegen(LLVM AAPGenRegisterInfo.inc       -gen-register-info)
tablegen(LLVM AAPGenInstrInfo.inc          -gen-instr-info)
add_public_tablegen_target(AAPCommonTableGen)

add_llvm_target(AAPCodeGen
  AAPTargetMachine.cpp
)

add_subdirectory(TargetInfo)
