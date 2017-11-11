// RUN: clang-refactor extract -selection=test:%s %s -- 2>&1 | grep -v CHECK | FileCheck %s

void captureStaticVars() {
  static int x;
  /*range astaticvar=->+0:39*/int y = x;
  x += 1;
}
// CHECK: 1 'astaticvar' results:
// CHECK:      static void extracted(int x) {
// CHECK-NEXT: int y = x;{{$}}
// CHECK-NEXT: }{{[[:space:]].*}}
// CHECK-NEXT: void captureStaticVars() {
// CHECK-NEXT: static int x;
// CHECK-NEXT: extracted(x);{{$}}

typedef struct {
  int width, height;
} Rectangle;

void basicTypes(int i, float f, char c, const int *ip, float *fp, const Rectangle *structPointer) {
  /*range basictypes=->+0:73*/basicTypes(i, f, c, ip, fp, structPointer);
}
// CHECK: 1 'basictypes' results:
// CHECK:      static void extracted(char c, float f, float *fp, int i, const int *ip, const Rectangle *structPointer) {
// CHECK-NEXT: basicTypes(i, f, c, ip, fp, structPointer);{{$}}
// CHECK-NEXT: }{{[[:space:]].*}}
// CHECK-NEXT: void basicTypes(int i, float f, char c, const int *ip, float *fp, const Rectangle *structPointer) {
// CHECK-NEXT: extracted(c, f, fp, i, ip, structPointer);{{$}}

int noGlobalsPlease = 0;

void cantCaptureGlobals() {
  /*range cantCaptureGlobals=->+0:62*/int y = noGlobalsPlease;
}
// CHECK: 1 'cantCaptureGlobals' results:
// CHECK:      static void extracted() {
// CHECK-NEXT: int y = noGlobalsPlease;{{$}}
// CHECK-NEXT: }{{[[:space:]].*}}
// CHECK-NEXT: void cantCaptureGlobals() {
// CHECK-NEXT: extracted();{{$}}
