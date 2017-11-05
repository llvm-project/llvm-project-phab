// RUN: clang-diff -ast-dump-json %s -- \
// RUN: | %python -c 'import json, sys; json.dump(json.loads(sys.stdin.read()), sys.stdout, sort_keys=True, indent=2)' \
// RUN: | FileCheck %s

// CHECK: "begin": 297,
// CHECK: "type": "FieldDecl"
// CHECK: "end": 318,
// CHECK: "type": "CXXRecordDecl"
class A {
  int x;
};

// CHECK: "children": [
// CHECK-NEXT: {
// CHECK-NEXT: "begin":
// CHECK-NEXT: "children": []
// CHECK-NEXT: "end":
// CHECK-NEXT: "id":
// CHECK-NEXT: "type": "CharacterLiteral"
// CHECK-NEXT: }
// CHECK: ]
// CHECK: "type": "VarDecl"
char nl = '\n';
