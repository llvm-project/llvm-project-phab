# RUN: llvm-mc -triple i686-windows-gnu -filetype obj -o - %s \
# RUN:    | llvm-readobj -coff-directives -symbols | FileCheck %s

# NOTE: this test checks multiple things:
# - that -aligncomm is not emitted for 32-byte alignment
# - that -aligncomm is emitted for the various alignments (greater than 32)
# - that the alignment is represented as a log_2 of the alignment
# - that the section switching occurs correctly
# - that functions after the switch also are emitted into the correct section

	.text

	.def _a
		.scl 3
		.type 32
	.endef
_a:
	ret

	.data

	.comm _s_1,4,0                  # @s_1
	.comm _s_2,4,2                  # @s_2
	.comm _s_3,4,4                  # @s_3
	.comm _s_4,4,8                  # @s_4

	.comm _s_5,4,1                  # @s_5
	.comm _small_but_overalign,1,6  # @s_6


	.text

	.def _b
		.scl 3
		.type 32
	.endef
_b:
	ret

# CHECK-NOT: -aligncomm:"_s_1",0

# CHECK: Symbols [
# CHECK:   Symbol {
# CHECK:     Name: _a
# CHECK:     Section: .text (1)
# CHECK:   }
# CHECK:   Symbol {
# CHECK:     Name: _small_but_overalign
# CHECK-NEXT:Value: 1
# CHECK-NEXT:Section: IMAGE_SYM_UNDEFINED (0)
# CHECK:   }
# CHECK:   Symbol {
# CHECK:     Name: _b
# CHECK:     Section: .text (1)
# CHECK:   }
# CHECK: ]

# CHECK: Directive(s): -aligncomm:"_s_4",8 -aligncomm:"_small_but_overalign",6

