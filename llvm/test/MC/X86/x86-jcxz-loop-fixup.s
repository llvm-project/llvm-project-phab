# RUN: not llvm-mc -filetype=obj -triple=x86_64-linux-gnu %s 2>&1 | FileCheck %s

       .balign 256
label00:
// CHECK: Value 509 does not fit in the Fixup field
	jecxz   label01
// CHECK: Value 507 does not fit in the Fixup field
	jrcxz   label01
// CHECK: Value 505 does not fit in the Fixup field
	loop	label01
// CHECK: Value 503 does not fit in the Fixup field
	loope	label01
// CHECK: Value 501 does not fit in the Fixup field
	loopne	label01
        .balign 512
label01:
// CHECK: Value -515 does not fit in the Fixup field
	jecxz   label00
// CHECK: Value -517 does not fit in the Fixup field
	jrcxz   label00
// CHECK: Value -519 does not fit in the Fixup field
	loop	label00
// CHECK: Value -521 does not fit in the Fixup field
	loope	label00
// CHECK: Value -523 does not fit in the Fixup field
	loopne	label00



