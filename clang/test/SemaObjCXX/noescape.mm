// RUN: %clang_cc1 -fsyntax-only -verify -fblocks -std=c++11 %s

typedef void (^BlockTy)();

void noescapeFunc(id, __attribute__((noescape)) BlockTy);
void escapeFunc(id, BlockTy); // expected-note {{parameter is not annotated with noescape}}
void noescapeFuncId(id, __attribute__((noescape)) id);
void escapeFuncId(id, id); // expected-note {{parameter is not annotated with noescape}}
void variadicFunc(int, ...);
void invalidFunc0(int __attribute__((noescape))); // expected-warning {{'noescape' attribute ignored on parameter of non-pointer type}}

__attribute__((objc_root_class)) @interface C
-(BlockTy)noescapeMethod:(id)a blockArg:(__attribute__((noescape)) BlockTy)b;
-(void)escapeMethod:(id)a blockArg:(BlockTy)b; // expected-note {{parameter is not annotated with noescape}}
@end

@implementation C
-(BlockTy)noescapeMethod:(id)a blockArg:(__attribute__((noescape)) BlockTy)b { // expected-note {{parameter is annotated with noescape}}
  return b; // expected-error {{cannot return non-escaping parameter}}
}
-(void)escapeMethod:(id)a blockArg:(BlockTy)b {
}
@end

struct S {
  void noescapeMethod(__attribute__((noescape)) BlockTy);
  void escapeMethod(BlockTy); // expected-note {{parameter is not annotated with noescape}}
};

BlockTy test1([[clang::noescape]] BlockTy neb, BlockTy eb, C *c1, S *s1) { // expected-note 11 {{parameter is annotated with noescape}}
  id i;
  BlockTy t;
  t = neb; // expected-error {{cannot assign non-escaping parameter}}
  t = eb;
  (void)^{ neb(); }; // expected-error 3 {{block cannot capture non-escaping parameter}}
  (void)^{ eb(); };
  auto noescapeBlock = ^(__attribute__((noescape)) BlockTy neb) {};
  auto escapeBlock = ^(BlockTy eb) {}; // expected-note {{parameter is not annotated with noescape}}
  noescapeBlock(neb);
  noescapeBlock(eb);
  escapeBlock(neb); // expected-error {{cannot pass non-escaping parameter}}
  escapeBlock(eb);
  noescapeFunc(i, neb);
  noescapeFunc(i, eb);
  escapeFunc(i, neb); // expected-error {{cannot pass non-escaping parameter}}
  escapeFunc(i, eb);
  noescapeFuncId(i, neb);
  noescapeFuncId(i, eb);
  escapeFuncId(i, neb); // expected-error {{cannot pass non-escaping parameter}}
  escapeFuncId(i, eb);
  variadicFunc(1, neb); // expected-error {{cannot pass non-escaping parameter}}
  variadicFunc(1, eb);
  [c1 noescapeMethod:i blockArg:neb];
  [c1 noescapeMethod:i blockArg:eb];
  [c1 escapeMethod:i blockArg:neb]; // expected-error {{cannot pass non-escaping parameter}}
  [c1 escapeMethod:i blockArg:eb];
  s1->noescapeMethod(neb);
  s1->noescapeMethod(eb);
  s1->escapeMethod(neb); // expected-error {{cannot pass non-escaping parameter}}
  s1->escapeMethod(eb);
  return neb; // expected-error {{cannot return non-escaping parameter}}
}
