// RUN: %clang_cc1 -triple x86_64-linux-gnu -emit-llvm -o - %s | FileCheck -check-prefix=WITHOUT %s
// RUN: %clang_cc1 -triple x86_64-linux-gnu -emit-llvm -o - %s -fsanitize=tbaa | FileCheck -check-prefix=TBAASAN %s
// RUN: echo "src:%s" | sed -e 's/\\/\\\\/g' > %t
// RUN: %clang_cc1 -triple x86_64-linux-gnu -emit-llvm -o - %s -fsanitize=tbaa -fsanitize-blacklist=%t | FileCheck -check-prefix=BL %s

// The sanitize_tbaa attribute should be attached to functions
// when TBAASanitizer is enabled, unless no_sanitize("tbaa") attribute
// is present.

// WITHOUT:  NoTBAASAN1{{.*}}) [[NOATTR:#[0-9]+]]
// BL:  NoTBAASAN1{{.*}}) [[NOATTR:#[0-9]+]]
// TBAASAN:  NoTBAASAN1{{.*}}) [[NOATTR:#[0-9]+]]
__attribute__((no_sanitize("tbaa")))
int NoTBAASAN1(int *a) { return *a; }

// WITHOUT:  NoTBAASAN2{{.*}}) [[NOATTR]]
// BL:  NoTBAASAN2{{.*}}) [[NOATTR]]
// TBAASAN:  NoTBAASAN2{{.*}}) [[NOATTR]]
__attribute__((no_sanitize("tbaa")))
int NoTBAASAN2(int *a);
int NoTBAASAN2(int *a) { return *a; }

// WITHOUT:  NoTBAASAN3{{.*}}) [[NOATTR:#[0-9]+]]
// BL:  NoTBAASAN3{{.*}}) [[NOATTR:#[0-9]+]]
// TBAASAN:  NoTBAASAN3{{.*}}) [[NOATTR:#[0-9]+]]
__attribute__((no_sanitize("tbaa")))
int NoTBAASAN3(int *a) { return *a; }

// WITHOUT:  TBAASANOk{{.*}}) [[NOATTR]]
// BL:  TBAASANOk{{.*}}) [[NOATTR]]
// TBAASAN: TBAASANOk{{.*}}) [[WITH:#[0-9]+]]
int TBAASANOk(int *a) { return *a; }

// WITHOUT:  TemplateTBAASANOk{{.*}}) [[NOATTR]]
// BL:  TemplateTBAASANOk{{.*}}) [[NOATTR]]
// TBAASAN: TemplateTBAASANOk{{.*}}) [[WITH]]
template<int i>
int TemplateTBAASANOk() { return i; }

// WITHOUT:  TemplateNoTBAASAN{{.*}}) [[NOATTR]]
// BL:  TemplateNoTBAASAN{{.*}}) [[NOATTR]]
// TBAASAN: TemplateNoTBAASAN{{.*}}) [[NOATTR]]
template<int i>
__attribute__((no_sanitize("tbaa")))
int TemplateNoTBAASAN() { return i; }

int force_instance = TemplateTBAASANOk<42>()
                   + TemplateNoTBAASAN<42>();

// Check that __cxx_global_var_init* get the sanitize_tbaa attribute.
int global1 = 0;
int global2 = *(int*)((char*)&global1+1);
// WITHOUT: @__cxx_global_var_init{{.*}}[[NOATTR:#[0-9]+]]
// BL: @__cxx_global_var_init{{.*}}[[NOATTR:#[0-9]+]]
// TBAASAN: @__cxx_global_var_init{{.*}}[[WITH:#[0-9]+]]

// WITHOUT: attributes [[NOATTR]] = { noinline nounwind{{.*}} }

// BL: attributes [[NOATTR]] = { noinline nounwind{{.*}} }

// TBAASAN: attributes [[NOATTR]] = { noinline nounwind{{.*}} }
// TBAASAN: attributes [[WITH]] = { noinline nounwind sanitize_tbaa{{.*}} }

// TBAASAN: Simple C++ TBAA
