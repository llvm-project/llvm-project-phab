// RUN: %clang_cc1 -verify -fopenmp-simd -x c -triple aarch64-unknown-unknown -emit-llvm %s -o - | FileCheck %s
// RUN: %clang_cc1 -verify -fopenmp-simd -x c -triple x86_64-unknown-unknown -emit-llvm %s -o - | FileCheck %s
// expected-no-diagnostics

// CHECK-LABEL: @simd_plain
// CHECK-LABEL: omp.inner.for.body:
// CHECK: load float, float* %arrayidx{{.*}} !llvm.mem.parallel_loop_access
// CHECK: load float, float* %arrayidx{{.*}} !llvm.mem.parallel_loop_access
// CHECK: store float %{{.*}}, float* %arrayidx{{.*}} !llvm.mem.parallel_loop_access
// CHECK: ret void
void simd_plain(float *a, float *b, float *c, int N) {
  #pragma omp simd
  for (int i = 0; i < N; i += 2)
    a[i] = b[i] * c[i];
}

// CHECK-LABEL: @simd_safelen_clause
// CHECK-NOT: !llvm.mem.parallel_loop_access
// CHECK-LABEL: omp.inner.for.inc:
// CHECK: br label %omp.inner.for.cond, !llvm.loop
// CHECK: ret void
void simd_safelen_clause(float *a, float *b, float *c, int N) {
  #pragma omp simd safelen(4)
  for (int i = 0; i < N; i += 2)
    a[i] = b[i] * c[i];
}

extern long long initial_val();

// CHECK-LABEL: @simd_simdlen_and_linear_clause
// CHECK: omp.inner.for.body:
// CHECK: !llvm.mem.parallel_loop_access
// CHECK: ret void
void simd_simdlen_and_linear_clause(float *a, float *b, float *c, int N) {
  long long lv = initial_val();
  #pragma omp simd simdlen(2) linear(lv: 4)
  for (int i = 0; i < N; ++i) {
    a[lv] = b[lv] * c[lv];
    lv += 4;
  }
}

extern float gfloat;

// CHECK-LABEL: @simd_aligned_and_private_clause
// CHECK-LABEL: entry:
// CHECK: %gfloat = alloca float, align 4
// CHECK: store float 1.000000e+00, float* @gfloat, align 4
// CHECK-LABEL: omp.inner.for.body:
// CHECK-NOT: @gfloat
// CHECK: load{{.*}}!llvm.mem.parallel_loop_access
// CHECK: store float {{.*}}, float* %gfloat, align 4, !llvm.mem.parallel_loop_access
// CHECK: %[[FADD:add[0-9]+]] = fadd float %{{[0-9]+}}, 2.000000e+00
// CHECK: store float %[[FADD]], float* {{.*}}, align 4, !llvm.mem.parallel_loop_access
// CHECK: ret void
void simd_aligned_and_private_clause(float *a, float *b, float *c, int N) {
  gfloat = 1.0f;
  #pragma omp simd aligned(a:4) private(gfloat)
  for (int i = 0; i < N; i += 2) {
    gfloat = b[i] * c[i];
    a[i] = gfloat + 2.0f;
  }
}

// CHECK-LABEL: @simd_lastprivate_and_reduction_clause
// CHECK-LABEL: entry:
// CHECK: %[[SUMVAR:sum[0-9]+]] = alloca float, align 4
// CHECK: store float 0.000000e+00, float* %[[SUMVAR]], align 4
// CHECK-LABEL: omp.inner.for.body
// CHECK: %[[LOAD:[0-9]+]] = load float, float* %[[SUMVAR]], align 4, !llvm.mem.parallel_loop_access
// CHECK: %[[FADD:add[0-9]+]] = fadd float %[[LOAD]], %mul{{.*}}
// CHECK: store float %[[FADD]], float* %[[SUMVAR]], align 4, !llvm.mem.parallel_loop_access
// CHECK: store i32{{.*}}, i32* %[[IDXVAR:idx[0-9]+]]
// CHECK-LABEL: omp.inner.for.end:
// CHECK-DAG: %[[TMP1:[0-9]+]] = load i32, i32* %[[IDXVAR]], align 4
// CHECK-DAG: store i32 %[[TMP1]], i32* %idx, align 4
// CHECK-DAG: %[[INITVAL:[0-9]+]] = load float, float* %sum, align 4
// CHECK-DAG: %[[TMP2:[0-9]+]] = load float, float* %[[SUMVAR]], align 4
// CHECK-DAG: %[[SUMMED:add[0-9]+]] = fadd float %[[INITVAL]], %[[TMP2]]
// CHECK-DAG: store float %[[SUMMED]], float* %sum, align 4
// CHECK-LABEL: simd.if.end:
// CHECK: %[[OUTVAL:[0-9]+]] = load float, float* %sum, align 4
// CHECK: %[[OUTADDR:[0-9]+]] = load float*, float** %a.addr, align 8
// CHECK: store float %[[OUTVAL]], float* %[[OUTADDR]], align 4
// CHECK: %[[RETIDX:[0-9]+]] = load i32, i32* %idx, align 4
// CHECK: ret i32 %[[RETIDX]]
int simd_lastprivate_and_reduction_clause(float *a, float *b, float *c, int N) {
  float sum = 0.0f;
  int idx;
  #pragma omp simd lastprivate(idx) reduction(+:sum)
  for (int i = 0; i < N; ++i) {
    sum += b[i] * c[i];
    idx = i * 2;
  }

  *a = sum;
  return idx;
}

// CHECK-LABEL: @simd_collapse_clause
// CHECK: omp.inner.for.body:
// CHECK-NOT: for.body:
// CHECK: ret void
void simd_collapse_clause(float **a, float **b, float **c, int N, int M) {
  #pragma omp simd collapse(2)
  for (int i = 0; i < N; ++i)
    for (int j = 0; j < N; ++j)
      a[i][j] = b[i][j] * c[i][j];
}

// Negative tests; no simd directive, so should be normal code.

// CHECK-LABEL: @parallel_for
// CHECK-NOT: call void {{.*}} @__kmpc_fork_call
// CHECK-NOT: @.omp_outlined.
// CHECK-NOT: omp.inner.for.body:
// CHECK: ret void
void parallel_for(float *a, float *b, float *c, int N) {
  #pragma omp parallel for
  for (int i = 0; i < N; ++i)
    a[i] = b[i] * c[i];
}

extern void long_running_func(int);

// CHECK-LABEL: @taskloop
// CHECK-NOT: call i8* @__kmpc_omp_task_alloc
// CHECK-NOT: call void @__kmpc_taskloop
// CHECK: ret void
void taskloop(int N) {
  #pragma omp taskloop
  for (int i = 0; i < N; ++i)
    long_running_func(i);
}

// Combined constructs; simd part should work, rest should be ignored.

// CHECK-LABEL: @parallel_for_simd
// CHECK-NOT: call void {{.*}} @__kmpc_fork_call
// CHECK-NOT: @.omp_outlined.
// CHECK: omp.inner.for.body:
// CHECK: ret void
void parallel_for_simd(float *a, float *b, float *c, int N) {
#pragma omp parallel for simd num_threads(2) simdlen(4)
  for (int i = 0; i < N; ++i)
    a[i] = b[i] * c[i];
}

// Make sure there's no declarations for libomp runtime functions
// CHECK-NOT: declare void @__kmpc

// CHECK-LABEL: !llvm.ident = !{!0}

// simd_safelen_clause width md
// CHECK-DAG: !{{[0-9]+}} = !{!"llvm.loop.vectorize.width", i32 4}
// simd_simdlen_clause width md
// CHECK-DAG: !{{[0-9]+}} = !{!"llvm.loop.vectorize.width", i32 2}
