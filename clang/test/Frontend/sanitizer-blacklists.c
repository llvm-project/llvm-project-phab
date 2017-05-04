// Blacklisting a function for ASan shouldn't affect TSan instrumentation.
//
// RUN: echo "fun:foo" > %t.blacklist
//
// RUN: %clang_cc1 -fsanitize=address,thread -triple x86_64-apple-darwin10 \
// RUN:   -fsanitize-blacklist=address:%t.blacklist \
// RUN:   -S -emit-llvm -o - %s | FileCheck %s --check-prefixes=COMMON,CHECK1

// Check that we can blacklist a function for multiple sanitizers using
// separate blacklists.
//
// RUN: %clang_cc1 -fsanitize=address,thread -triple x86_64-apple-darwin10 \
// RUN:   -fsanitize-blacklist=address:%t.blacklist \
// RUN:   -fsanitize-blacklist=thread:%t.tsan_blacklist \
// RUN:   -S -emit-llvm -o - %s | FileCheck %s --check-prefixes=COMMON,CHECK2

// Check that we can blacklist a function for multiple sanitizers using the
// same blacklist.
//
// RUN: %clang_cc1 -fsanitize=address,thread -triple x86_64-apple-darwin10 \
// RUN:   -fsanitize-blacklist=address,thread:%t.blacklist \
// RUN:   -S -emit-llvm -o - %s | FileCheck %s --check-prefixes=COMMON,CHECK2

// COMMON: define void @foo() [[FOO_ATTR:#[0-9]+]]
void foo() {}

// CHECK1: attributes [[FOO_ATTR]] = { {{[^}]*}} sanitize_thread
// CHECK1-NOT: sanitize_address

// CHECK2: attributes [[FOO_ATTR]]
// CHECK2-NOT: sanitize_address
// CHECK2-NOT: sanitize_thread
