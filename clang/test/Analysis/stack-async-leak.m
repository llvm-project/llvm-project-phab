// RUN: %clang_analyze_cc1 -triple x86_64-apple-darwin10 -analyzer-checker=core -analyzer-store=region -fblocks -verify %s

typedef struct dispatch_queue_s *dispatch_queue_t;
typedef void (^dispatch_block_t)(void);
void dispatch_async(dispatch_queue_t queue, dispatch_block_t block);
typedef long dispatch_once_t;
void dispatch_once(dispatch_once_t *predicate, dispatch_block_t block);
typedef long dispatch_time_t;
void dispatch_after(dispatch_time_t when, dispatch_queue_t queue, dispatch_block_t block);

extern dispatch_queue_t queue;
extern dispatch_once_t *predicate;
extern dispatch_time_t when;

void test_block_expr_async() {
  int x = 123;
  int *p = &x;

  dispatch_async(queue, ^{
    *p = 321;
  });
  // expected-warning@-3 {{Address of stack memory associated with local variable 'x' was captured by the block \
which will be executed asynchronously}}
}

void test_block_expr_once() {
  int x = 123;
  int *p = &x;
  dispatch_once(predicate, ^{
    *p = 321;
  });
  // expected-warning@-3 {{Address of stack memory associated with local variable 'x' was captured by the block \
which will be executed asynchronously}}
}

void test_block_expr_after() {
  int x = 123;
  int *p = &x;
  dispatch_after(when, queue, ^{
    *p = 321;
  });
  // expected-warning@-3 {{Address of stack memory associated with local variable 'x' was captured by the block \
which will be executed asynchronously}}
}

void test_block_var_async() {
  int x = 123;
  int *p = &x;
  void (^b)(void) = ^void(void) {
    *p = 1; 
  };
  dispatch_async(queue, b);
  // expected-warning@-1 {{Address of stack memory associated with local variable 'x' was captured by the block \
which will be executed asynchronously}}
}

void test_block_var_once() {
  int x = 123;
  int *p = &x;
  void (^b)(void) = ^void(void) {
    *p = 1; 
  };
  dispatch_once(predicate, b);
  // expected-warning@-1 {{Address of stack memory associated with local variable 'x' was captured by the block \
which will be executed asynchronously}}
}

void test_block_var_after() {
  int x = 123;
  int *p = &x;
  void (^b)(void) = ^void(void) {
    *p = 1; 
  };
  dispatch_after(when, queue, b);
  // expected-warning@-1 {{Address of stack memory associated with local variable 'x' was captured by the block \
which will be executed asynchronously}}
}
