// RUN: %clang_analyze_cc1 -triple x86_64-apple-darwin10 -analyzer-checker=core -fblocks -verify %s

typedef struct dispatch_queue_s *dispatch_queue_t;
typedef void (^dispatch_block_t)(void);
void dispatch_async(dispatch_queue_t queue, dispatch_block_t block);
extern dispatch_queue_t queue;

void test_block_inside_block_async_leak_without_arc() {
  int x = 123;
  int *p = &x;
  void (^inner)(void) = ^void(void) {
    int y = x;
    ++y; 
  };
  dispatch_async(queue, ^void(void) {
    int z = x;
    ++z;
    inner(); 
  });
  // expected-warning-re@-5{{Address of stack-allocated block declared on line {{.+}} \
is captured by an asynchronously-executed block}}
}

