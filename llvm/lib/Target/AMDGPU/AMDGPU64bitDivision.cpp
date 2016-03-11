#include "AMDGPU64bitDivision.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Instructions.h"

using namespace llvm;


bool llvm::AMDExpandUDivision(BinaryOperator *Div)
{

	errs()<<"\n AMDExpandUDivision Called\n";

	return true;

}
