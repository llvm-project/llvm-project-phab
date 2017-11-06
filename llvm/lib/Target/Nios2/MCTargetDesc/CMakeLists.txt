#MCTargetDesc / CMakeLists.txt
add_llvm_library(LLVMNios2Desc Nios2MCTargetDesc.cpp Nios2MCExpr.cpp
                 Nios2MCAsmInfo.cpp Nios2TargetStreamer.cpp Nios2AsmBackend.cpp
                 Nios2ELFObjectWriter.cpp)
