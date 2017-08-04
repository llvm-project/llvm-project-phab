// RUN: %clang_cc1 -emit-llvm -cl-ext=+cl_khr_subgroups -O0 -cl-std=CL2.0 -o - %s | FileCheck -check-prefixes=CHECK,NAMD %s
// RUN: %clang_cc1 -triple amdgcn---amdgizcl -emit-llvm -cl-ext=+cl_khr_subgroups -O0 -cl-std=CL2.0 -o - %s | FileCheck -check-prefixes=CHECK,AMD %s

// CHECK: %opencl.pipe_t = type opaque
// CHECK: %opencl.reserve_id_t = type opaque

#pragma OPENCL EXTENSION cl_khr_subgroups : enable

typedef struct {
  int x[100];
} S;

typedef long long2 __attribute__((ext_vector_type(2)));
typedef long long3 __attribute__((ext_vector_type(3)));
typedef long long4 __attribute__((ext_vector_type(4)));
typedef long long8 __attribute__((ext_vector_type(8)));
typedef long long16 __attribute__((ext_vector_type(16)));

void test1(read_only pipe int p, global int *ptr) {
  // NAMD: call i32 @__read_pipe_2(%opencl.pipe_t* %{{.*}}, i8* %{{.*}}, i32 4, i32 4)
  // AMD: call i32 @__read_pipe_2_4(%opencl.pipe_t addrspace(1)* %{{.*}}, i32* %{{.*}})
  read_pipe(p, ptr);
  // CHECK: call %opencl.reserve_id_t* @__reserve_read_pipe(%opencl.pipe_t{{.*}}* %{{.*}}, i32 {{.*}}, i32 4, i32 4)
  reserve_id_t rid = reserve_read_pipe(p, 2);
  // NAMD: call i32 @__read_pipe_4(%opencl.pipe_t* %{{.*}}, %opencl.reserve_id_t* %{{.*}}, i32 {{.*}}, i8* %{{.*}}, i32 4, i32 4)
  // AMD: call i32 @__read_pipe_4_4(%opencl.pipe_t addrspace(1)* %{{.*}}, %opencl.reserve_id_t* %{{.*}}, i32 {{.*}}, i32* %{{.*}})
  read_pipe(p, rid, 2, ptr);
  // CHECK: call void @__commit_read_pipe(%opencl.pipe_t{{.*}}* %{{.*}}, %opencl.reserve_id_t{{.*}}* %{{.*}}, i32 4, i32 4)
  commit_read_pipe(p, rid);
}

void test2(write_only pipe int p, global int *ptr) {
  // NAMD: call i32 @__write_pipe_2(%opencl.pipe_t* %{{.*}}, i8* %{{.*}}, i32 4, i32 4)
  // AMD: call i32 @__write_pipe_2_4(%opencl.pipe_t addrspace(1)* %{{.*}}, i32* %{{.*}})
  write_pipe(p, ptr);
  // CHECK: call %opencl.reserve_id_t* @__reserve_write_pipe(%opencl.pipe_t{{.*}}* %{{.*}}, i32 {{.*}}, i32 4, i32 4)
  reserve_id_t rid = reserve_write_pipe(p, 2);
  // NAMD: call i32 @__write_pipe_4(%opencl.pipe_t* %{{.*}}, %opencl.reserve_id_t* %{{.*}}, i32 {{.*}}, i8* %{{.*}}, i32 4, i32 4)
  // AMD: call i32 @__write_pipe_4_4(%opencl.pipe_t addrspace(1)* %{{.*}}, %opencl.reserve_id_t* %{{.*}}, i32 {{.*}}, i32* %{{.*}})
  write_pipe(p, rid, 2, ptr);
  // CHECK: call void @__commit_write_pipe(%opencl.pipe_t{{.*}}* %{{.*}}, %opencl.reserve_id_t* %{{.*}}, i32 4, i32 4)
  commit_write_pipe(p, rid);
}

void test3(read_only pipe int p, global int *ptr) {
  // CHECK: call %opencl.reserve_id_t* @__work_group_reserve_read_pipe(%opencl.pipe_t{{.*}}* %{{.*}}, i32 {{.*}}, i32 4, i32 4)
  reserve_id_t rid = work_group_reserve_read_pipe(p, 2);
  // CHECK: call void @__work_group_commit_read_pipe(%opencl.pipe_t{{.*}}* %{{.*}}, %opencl.reserve_id_t{{.*}}* %{{.*}}, i32 4, i32 4)
  work_group_commit_read_pipe(p, rid);
}

void test4(write_only pipe int p, global int *ptr) {
  // CHECK: call %opencl.reserve_id_t* @__work_group_reserve_write_pipe(%opencl.pipe_t{{.*}}* %{{.*}}, i32 {{.*}}, i32 4, i32 4)
  reserve_id_t rid = work_group_reserve_write_pipe(p, 2);
  // CHECK: call void @__work_group_commit_write_pipe(%opencl.pipe_t{{.*}}* %{{.*}}, %opencl.reserve_id_t{{.*}}* %{{.*}}, i32 4, i32 4)
  work_group_commit_write_pipe(p, rid);
}

void test5(read_only pipe int p, global int *ptr) {
  // CHECK: call %opencl.reserve_id_t* @__sub_group_reserve_read_pipe(%opencl.pipe_t{{.*}}* %{{.*}}, i32 {{.*}}, i32 4, i32 4)
  reserve_id_t rid = sub_group_reserve_read_pipe(p, 2);
  // CHECK: call void @__sub_group_commit_read_pipe(%opencl.pipe_t{{.*}}* %{{.*}}, %opencl.reserve_id_t{{.*}}* %{{.*}}, i32 4, i32 4)
  sub_group_commit_read_pipe(p, rid);
}

void test6(write_only pipe int p, global int *ptr) {
  // CHECK: call %opencl.reserve_id_t* @__sub_group_reserve_write_pipe(%opencl.pipe_t{{.*}}* %{{.*}}, i32 {{.*}}, i32 4, i32 4)
  reserve_id_t rid = sub_group_reserve_write_pipe(p, 2);
  // CHECK: call void @__sub_group_commit_write_pipe(%opencl.pipe_t{{.*}}* %{{.*}}, %opencl.reserve_id_t{{.*}}* %{{.*}}, i32 4, i32 4)
  sub_group_commit_write_pipe(p, rid);
}

void test7(write_only pipe int p, global int *ptr) {
  // CHECK: call i32 @__get_pipe_num_packets(%opencl.pipe_t{{.*}}* %{{.*}}, i32 4, i32 4)
  *ptr = get_pipe_num_packets(p);
  // CHECK: call i32 @__get_pipe_max_packets(%opencl.pipe_t{{.*}}* %{{.*}}, i32 4, i32 4)
  *ptr = get_pipe_max_packets(p);
}

void test8(read_only pipe int r, write_only pipe int w, global int *ptr) {
  // verify that return type is correctly casted to i1 value
  // NAMD: %[[R:[0-9]+]] = call i32 @__read_pipe_2
  // AMD: %[[R:[0-9]+]] = call i32 @__read_pipe_2_4
  // CHECK: icmp ne i32 %[[R]], 0
  if (read_pipe(r, ptr)) *ptr = -1;
  // NAMD: %[[W:[0-9]+]] = call i32 @__write_pipe_2
  // AMD: %[[W:[0-9]+]] = call i32 @__write_pipe_2_4
  // CHECK: icmp ne i32 %[[W]], 0
  if (write_pipe(w, ptr)) *ptr = -1;
  // CHECK: %[[N:[0-9]+]] = call i32 @__get_pipe_num_packets
  // CHECK: icmp ne i32 %[[N]], 0
  if (get_pipe_num_packets(r)) *ptr = -1;
  // CHECK: %[[M:[0-9]+]] = call i32 @__get_pipe_max_packets
  // CHECK: icmp ne i32 %[[M]], 0
  if (get_pipe_max_packets(w)) *ptr = -1;
}

// CHECK-LABEL: @test9
void test9(read_only pipe char p1, global char *ptr1,
           read_only pipe short p2, global short *ptr2,
           read_only pipe int p4, global int *ptr4,
           read_only pipe long p8, global long *ptr8,
           read_only pipe long2 p16, global long2 *ptr16,
           read_only pipe long4 p32, global long4 *ptr32,
           read_only pipe long8 p64, global long8 *ptr64,
           read_only pipe long16 p128, global long16 *ptr128,
           read_only pipe S pu, global S *ptru) {
  // AMD: call i32 @__read_pipe_2_1(%opencl.pipe_t addrspace(1)* {{.*}}, i8* %{{.*}})
  read_pipe(p1, ptr1);
  // AMD: call i32 @__read_pipe_2_2(%opencl.pipe_t addrspace(1)* {{.*}}, i16* %{{.*}})
  read_pipe(p2, ptr2);
  // AMD: call i32 @__read_pipe_2_4(%opencl.pipe_t addrspace(1)* {{.*}}, i32* %{{.*}})
  read_pipe(p4, ptr4);
  // AMD: call i32 @__read_pipe_2_8(%opencl.pipe_t addrspace(1)* {{.*}}, i64* %{{.*}})
  read_pipe(p8, ptr8);
  // AMD: call i32 @__read_pipe_2_16(%opencl.pipe_t addrspace(1)* %{{.*}}, <2 x i64>* %{{.*}})
  read_pipe(p16, ptr16);
  // AMD: call i32 @__read_pipe_2_32(%opencl.pipe_t addrspace(1)* %{{.*}}, <4 x i64>* %{{.*}})
  read_pipe(p32, ptr32);
  // AMD: call i32 @__read_pipe_2_64(%opencl.pipe_t addrspace(1)* %{{.*}}, <8 x i64>* %{{.*}})
  read_pipe(p64, ptr64);
  // AMD: call i32 @__read_pipe_2_128(%opencl.pipe_t addrspace(1)* %{{.*}}, <16 x i64>* %{{.*}})
  read_pipe(p128, ptr128);
  // AMD: call i32 @__read_pipe_2(%opencl.pipe_t addrspace(1)* %{{.*}}, i8* %{{.*}}, i32 400, i32 4)
  read_pipe(pu, ptru);
}
