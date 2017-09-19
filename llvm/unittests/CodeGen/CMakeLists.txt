set(LLVM_LINK_COMPONENTS
  ${LLVM_TARGETS_TO_BUILD}
  AsmPrinter
  CodeGen
  Core
  Support
  Target
  )

set(CodeGenSources
  DIEHashTest.cpp
  LowLevelTypeTest.cpp
  MachineInstrBundleIteratorTest.cpp
  MachineOperandTest.cpp
  ScalableVectorMVTsTest.cpp
  TargetOptionsTest.cpp
  )

add_llvm_unittest(CodeGenTests
  ${CodeGenSources}
  )

add_subdirectory(GlobalISel)
