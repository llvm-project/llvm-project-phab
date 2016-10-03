// RUN: %clang_cc1 -triple x86_64-apple-darwin -O1 -disable-llvm-optzns %s -emit-llvm -o - | FileCheck %s

int r;
void ex1(int *);

int *a;
int *foo() {
  int * restrict x = a;
  return x;

// CHECK-LABEL: define i32* @foo()
// CHECK: [[x_addr_foo1:%[a-z0-9_.]+]] = alloca i32*
// CHECK: [[x_foo1:%[a-z0-9_.]+]] = load i32*, i32** @a{{.*}}, !noalias [[TAG_foo1:!.*]]
// CHECK: [[x_a_foo1:%[a-z0-9_.]+]] = call i32* @llvm.noalias.p0i32(i32* [[x_foo1]], metadata [[TAG_foo1]])
// CHECK: store i32* [[x_a_foo1]], i32** [[x_addr_foo1]]{{.*}}, !noalias [[TAG_foo1]]
}

int *a2;
int *foo1(int b) {
  int * restrict x;

// CHECK-LABEL: define i32* @foo1(i32 %b)
// CHECK: [[x_addr_foo2:%[a-z0-9_.]+]] = alloca i32*
// CHECK: [[x2_addr_foo2:%[a-z0-9_.]+]] = alloca i32*

  if (b) {
    x = a;
    r += *x;
    ex1(x);

// CHECK: [[x_foo2:%[a-z0-9_.]+]] = load i32*, i32** @a{{.*}}, !noalias [[x_x2_tag_foo2:!.*]]
// CHECK: [[x_a_foo2:%[a-z0-9_.]+]] = call i32* @llvm.noalias.p0i32(i32* [[x_foo2]], metadata [[x_tag_foo2:!.*]])
// CHECK: store i32* [[x_a_foo2]], i32** [[x_addr_foo2]]{{.*}}, !noalias [[x_x2_tag_foo2]]
// CHECK: call void @ex1
    ++x;
    *x = r;
    ex1(x);

// CHECK:  [[old_x_foo2:%[a-z0-9_.]+]] = load i32*, i32** [[x_addr_foo2]]{{.*}}, !noalias [[x_x2_tag_foo2]]
// CHECK:  [[x_foo2:%[a-z0-9_.]+]] = getelementptr inbounds i32, i32* [[old_x_foo2]], i32 1
// CHECK:  [[x_a_foo2:%[a-z0-9_.]+]] = call i32* @llvm.noalias.p0i32(i32* [[x_foo2]], metadata [[x_tag_foo2]])
// CHECK:  store i32* [[x_a_foo2]], i32** [[x_addr_foo2]]{{.*}}, !noalias [[x_x2_tag_foo2]]
// CHECK: call void @ex1

    x += b;
    *x = r;
    ex1(x);

// CHECK:  [[old_x_foo2:%[a-z0-9_.]+]] = load i32*, i32** [[x_addr_foo2]]{{.*}}, !noalias [[x_x2_tag_foo2]]
// CHECK:  [[x_foo2:%[a-z0-9_.]+]] = getelementptr inbounds i32, i32* [[old_x_foo2]], i64
// CHECK:  [[x_a_foo2:%[a-z0-9_.]+]] = call i32* @llvm.noalias.p0i32(i32* [[x_foo2]], metadata [[x_tag_foo2]])
// CHECK:  store i32* [[x_a_foo2]], i32** [[x_addr_foo2]]{{.*}}, !noalias [[x_x2_tag_foo2]]
// CHECK: call void @ex1

    int * restrict x2 = a2;
    *x2 = r;
    ex1(x2);

// CHECK: [[x2_foo2:%[a-z0-9_.]+]] = load i32*, i32** @a2{{.*}}, !noalias [[x_x2_tag_foo2]]
// CHECK: [[x2_a_foo2:%[a-z0-9_.]+]] = call i32* @llvm.noalias.p0i32(i32* [[x2_foo2]], metadata [[x2_tag_foo2:!.*]])
// CHECK: store i32* [[x2_a_foo2]], i32** [[x2_addr_foo2]]{{.*}}, !noalias [[x_x2_tag_foo2]]
// CHECK: call void @ex1
  } else {
    x = a2;
    r += *x;

// CHECK: [[x_foo2:%[a-z0-9_.]+]] = load i32*, i32** @a2{{.*}}, !noalias [[x_tag_foo2]]
// CHECK: [[x_a_foo2:%[a-z0-9_.]+]] = call i32* @llvm.noalias.p0i32(i32* [[x_foo2]], metadata [[x_tag_foo2]])
// CHECK: store i32* [[x_a_foo2]], i32** [[x_addr_foo2]]{{.*}}, !noalias [[x_tag_foo2]]
  }

  return x;
}

int *bar() {
  int * x = a;
  return x;

// CHECK-LABEL: define i32* @bar()
// CHECK-NOT: noalias
// CHECK: ret i32*
}

