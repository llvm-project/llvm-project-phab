// Test handling of -f(no-)function-sections and -f(no-)data-sections
//
// CHECK-FS: -ffunction-sections
// CHECK-NOFS-NOT: -ffunction-sections
// CHECK-DS: -fdata-sections
// CHECK-NODS-NOT: -fdata-sections
// CHECK-US-NOT: -fno-unique-section-names
// CHECK-NOUS: -fno-unique-section-names
// CHECK-RF-NOT: -fno-reorder-functions
// CHECK-NORF: -fno-reorder-functions

// RUN: %clang -no-canonical-prefixes %s -### -fsyntax-only 2>&1       \
// RUN:     -target i386-unknown-linux \
// RUN:   | FileCheck --check-prefix=CHECK-NOFS --check-prefix=CHECK-NODS %s

// RUN: %clang -no-canonical-prefixes %s -### -fsyntax-only 2>&1       \
// RUN:     -target i386-unknown-linux \
// RUN:     -ffunction-sections \
// RUN:   | FileCheck --check-prefix=CHECK-FS %s

// RUN: %clang -no-canonical-prefixes %s -### -fsyntax-only 2>&1       \
// RUN:     -target i386-unknown-linux \
// RUN:     -fno-function-sections \
// RUN:   | FileCheck --check-prefix=CHECK-NOFS %s

// RUN: %clang -no-canonical-prefixes %s -### -fsyntax-only 2>&1       \
// RUN:     -target i386-unknown-linux \
// RUN:     -ffunction-sections -fno-function-sections \
// RUN:   | FileCheck --check-prefix=CHECK-NOFS %s

// RUN: %clang -no-canonical-prefixes %s -### -fsyntax-only 2>&1       \
// RUN:     -target i386-unknown-linux \
// RUN:     -fno-function-sections -ffunction-sections \
// RUN:   | FileCheck --check-prefix=CHECK-FS %s

// RUN: %clang -no-canonical-prefixes %s -### -fsyntax-only 2>&1       \
// RUN:     -target i386-unknown-linux \
// RUN:     -ffunction-sections -fno-function-sections -ffunction-sections \
// RUN:   | FileCheck --check-prefix=CHECK-FS %s


// RUN: %clang -no-canonical-prefixes %s -### -fsyntax-only 2>&1       \
// RUN:     -target i386-unknown-linux \
// RUN:     -fdata-sections \
// RUN:   | FileCheck --check-prefix=CHECK-DS %s

// RUN: %clang -no-canonical-prefixes %s -### -fsyntax-only 2>&1       \
// RUN:     -target i386-unknown-linux \
// RUN:     -fno-data-sections \
// RUN:   | FileCheck --check-prefix=CHECK-NODS %s

// RUN: %clang -no-canonical-prefixes %s -### -fsyntax-only 2>&1       \
// RUN:     -target i386-unknown-linux \
// RUN:     -fdata-sections -fno-data-sections \
// RUN:   | FileCheck --check-prefix=CHECK-NODS %s

// RUN: %clang -no-canonical-prefixes %s -### -fsyntax-only 2>&1       \
// RUN:     -target i386-unknown-linux \
// RUN:     -fno-data-sections -fdata-sections \
// RUN:   | FileCheck --check-prefix=CHECK-DS %s

// RUN: %clang -no-canonical-prefixes %s -### -fsyntax-only 2>&1       \
// RUN:     -target i386-unknown-linux \
// RUN:     -fdata-sections -fno-data-sections -fdata-sections \
// RUN:   | FileCheck --check-prefix=CHECK-DS %s


// RUN: %clang -no-canonical-prefixes %s -### -fsyntax-only 2>&1        \
// RUN:     -target i386-unknown-linux \
// RUN:     -funique-section-names \
// RUN:   | FileCheck --check-prefix=CHECK-US %s

// RUN: %clang -no-canonical-prefixes %s -### -fsyntax-only 2>&1        \
// RUN:     -target i386-unknown-linux \
// RUN:     -fno-unique-section-names \
// RUN:   | FileCheck --check-prefix=CHECK-NOUS %s

// RUN: %clang -no-canonical-prefixes %s -### -fsyntax-only 2>&1        \
// RUN:     -target i386-unknown-linux \
// RUN:     -freorder-functions \
// RUN:   | FileCheck --check-prefix=CHECK-RF %s

// RUN: %clang -no-canonical-prefixes %s -### -fsyntax-only 2>&1        \
// RUN:     -target i386-unknown-linux \
// RUN:     -fno-reorder-functions \
// RUN:   | FileCheck --check-prefix=CHECK-NORF %s
