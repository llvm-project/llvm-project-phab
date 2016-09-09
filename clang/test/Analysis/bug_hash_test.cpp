// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=core,debug.DumpBugHash -analyzer-output=plist %s -o %t.plist
// RUN: FileCheck --input-file=%t.plist %s

int function(int p) {
  return 5;
}

namespace {
int variadicParam(int p, ...) {
  return 5;
}
}

constexpr int f() { return 5; }

namespace AA {
  class X {
    int priv;
    X() : priv(5) { priv = 0; }

    static int static_method() {
      return 5;
    }

    int method() && {
      class Y {
        inline int method() const & {
          return 5;
        }
      };

      return 5;
    }

    int OutOfLine();

    X& operator=(int a) {
      return *this;
    }

    operator int() {
      return 0;
    }

    explicit operator float() {
      return 0;
    }
  };
}

int AA::X::OutOfLine() {
  return 5;
}

void testLambda() {
  [] () {
    return;
  }();
}

// CHECK: <key>diagnostics</key>
// CHECK-NEXT: <array>
// CHECK-NEXT:  <dict>
// CHECK-NEXT:   <key>path</key>
// CHECK-NEXT:   <array>
// CHECK-NEXT:    <dict>
// CHECK-NEXT:     <key>kind</key><string>control</string>
// CHECK-NEXT:     <key>edges</key>
// CHECK-NEXT:      <array>
// CHECK-NEXT:       <dict>
// CHECK-NEXT:        <key>start</key>
// CHECK-NEXT:         <array>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>5</integer>
// CHECK-NEXT:           <key>col</key><integer>3</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>5</integer>
// CHECK-NEXT:           <key>col</key><integer>8</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:         </array>
// CHECK-NEXT:        <key>end</key>
// CHECK-NEXT:         <array>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>5</integer>
// CHECK-NEXT:           <key>col</key><integer>10</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>5</integer>
// CHECK-NEXT:           <key>col</key><integer>10</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:         </array>
// CHECK-NEXT:       </dict>
// CHECK-NEXT:      </array>
// CHECK-NEXT:    </dict>
// CHECK-NEXT:    <dict>
// CHECK-NEXT:     <key>kind</key><string>event</string>
// CHECK-NEXT:     <key>location</key>
// CHECK-NEXT:     <dict>
// CHECK-NEXT:      <key>line</key><integer>5</integer>
// CHECK-NEXT:      <key>col</key><integer>10</integer>
// CHECK-NEXT:      <key>file</key><integer>0</integer>
// CHECK-NEXT:     </dict>
// CHECK-NEXT:     <key>ranges</key>
// CHECK-NEXT:     <array>
// CHECK-NEXT:       <array>
// CHECK-NEXT:        <dict>
// CHECK-NEXT:         <key>line</key><integer>5</integer>
// CHECK-NEXT:         <key>col</key><integer>10</integer>
// CHECK-NEXT:         <key>file</key><integer>0</integer>
// CHECK-NEXT:        </dict>
// CHECK-NEXT:        <dict>
// CHECK-NEXT:         <key>line</key><integer>5</integer>
// CHECK-NEXT:         <key>col</key><integer>10</integer>
// CHECK-NEXT:         <key>file</key><integer>0</integer>
// CHECK-NEXT:        </dict>
// CHECK-NEXT:       </array>
// CHECK-NEXT:     </array>
// CHECK-NEXT:     <key>depth</key><integer>0</integer>
// CHECK-NEXT:     <key>extended_message</key>
// CHECK-NEXT:     <string>debug.DumpBugHash$int function(int)$10$return5;$debug [debug.DumpBugHash]</string>
// CHECK-NEXT:     <key>message</key>
// CHECK-NEXT:     <string>debug.DumpBugHash$int function(int)$10$return5;$debug [debug.DumpBugHash]</string>
// CHECK-NEXT:    </dict>
// CHECK-NEXT:   </array>
// CHECK-NEXT:   <key>description</key><string>debug.DumpBugHash$int function(int)$10$return5;$debug [debug.DumpBugHash]</string>
// CHECK-NEXT:   <key>category</key><string>debug</string>
// CHECK-NEXT:   <key>type</key><string>Dump hash components</string>
// CHECK-NEXT:   <key>check_name</key><string>debug.DumpBugHash</string>
// CHECK-NEXT:   <!-- This hash is experimental and going to change! -->
// CHECK-NEXT:   <key>issue_hash_content_of_line_in_context</key><string>e7be204e83f8e5ad3c870ec011d5131d</string>
// CHECK-NEXT:  <key>issue_context_kind</key><string>function</string>
// CHECK-NEXT:  <key>issue_context</key><string>function</string>
// CHECK-NEXT:  <key>issue_hash_function_offset</key><string>1</string>
// CHECK-NEXT:  <key>location</key>
// CHECK-NEXT:  <dict>
// CHECK-NEXT:   <key>line</key><integer>5</integer>
// CHECK-NEXT:   <key>col</key><integer>10</integer>
// CHECK-NEXT:   <key>file</key><integer>0</integer>
// CHECK-NEXT:  </dict>
// CHECK-NEXT:  </dict>
// CHECK-NEXT:  <dict>
// CHECK-NEXT:   <key>path</key>
// CHECK-NEXT:   <array>
// CHECK-NEXT:    <dict>
// CHECK-NEXT:     <key>kind</key><string>control</string>
// CHECK-NEXT:     <key>edges</key>
// CHECK-NEXT:      <array>
// CHECK-NEXT:       <dict>
// CHECK-NEXT:        <key>start</key>
// CHECK-NEXT:         <array>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>10</integer>
// CHECK-NEXT:           <key>col</key><integer>3</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>10</integer>
// CHECK-NEXT:           <key>col</key><integer>8</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:         </array>
// CHECK-NEXT:        <key>end</key>
// CHECK-NEXT:         <array>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>10</integer>
// CHECK-NEXT:           <key>col</key><integer>10</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>10</integer>
// CHECK-NEXT:           <key>col</key><integer>10</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:         </array>
// CHECK-NEXT:       </dict>
// CHECK-NEXT:      </array>
// CHECK-NEXT:    </dict>
// CHECK-NEXT:    <dict>
// CHECK-NEXT:     <key>kind</key><string>event</string>
// CHECK-NEXT:     <key>location</key>
// CHECK-NEXT:     <dict>
// CHECK-NEXT:      <key>line</key><integer>10</integer>
// CHECK-NEXT:      <key>col</key><integer>10</integer>
// CHECK-NEXT:      <key>file</key><integer>0</integer>
// CHECK-NEXT:     </dict>
// CHECK-NEXT:     <key>ranges</key>
// CHECK-NEXT:     <array>
// CHECK-NEXT:       <array>
// CHECK-NEXT:        <dict>
// CHECK-NEXT:         <key>line</key><integer>10</integer>
// CHECK-NEXT:         <key>col</key><integer>10</integer>
// CHECK-NEXT:         <key>file</key><integer>0</integer>
// CHECK-NEXT:        </dict>
// CHECK-NEXT:        <dict>
// CHECK-NEXT:         <key>line</key><integer>10</integer>
// CHECK-NEXT:         <key>col</key><integer>10</integer>
// CHECK-NEXT:         <key>file</key><integer>0</integer>
// CHECK-NEXT:        </dict>
// CHECK-NEXT:       </array>
// CHECK-NEXT:     </array>
// CHECK-NEXT:     <key>depth</key><integer>0</integer>
// CHECK-NEXT:     <key>extended_message</key>
// CHECK-NEXT:     <string>debug.DumpBugHash$int (anonymous namespace)::variadicParam(int, ...)$10$return5;$debug [debug.DumpBugHash]</string>
// CHECK-NEXT:     <key>message</key>
// CHECK-NEXT:     <string>debug.DumpBugHash$int (anonymous namespace)::variadicParam(int, ...)$10$return5;$debug [debug.DumpBugHash]</string>
// CHECK-NEXT:    </dict>
// CHECK-NEXT:   </array>
// CHECK-NEXT:   <key>description</key><string>debug.DumpBugHash$int (anonymous namespace)::variadicParam(int, ...)$10$return5;$debug [debug.DumpBugHash]</string>
// CHECK-NEXT:   <key>category</key><string>debug</string>
// CHECK-NEXT:   <key>type</key><string>Dump hash components</string>
// CHECK-NEXT:   <key>check_name</key><string>debug.DumpBugHash</string>
// CHECK-NEXT:   <!-- This hash is experimental and going to change! -->
// CHECK-NEXT:   <key>issue_hash_content_of_line_in_context</key><string>bc5dc0507ee90f1d14259057d25fb2b9</string>
// CHECK-NEXT:  <key>issue_context_kind</key><string>function</string>
// CHECK-NEXT:  <key>issue_context</key><string>variadicParam</string>
// CHECK-NEXT:  <key>issue_hash_function_offset</key><string>1</string>
// CHECK-NEXT:  <key>location</key>
// CHECK-NEXT:  <dict>
// CHECK-NEXT:   <key>line</key><integer>10</integer>
// CHECK-NEXT:   <key>col</key><integer>10</integer>
// CHECK-NEXT:   <key>file</key><integer>0</integer>
// CHECK-NEXT:  </dict>
// CHECK-NEXT:  </dict>
// CHECK-NEXT:  <dict>
// CHECK-NEXT:   <key>path</key>
// CHECK-NEXT:   <array>
// CHECK-NEXT:    <dict>
// CHECK-NEXT:     <key>kind</key><string>control</string>
// CHECK-NEXT:     <key>edges</key>
// CHECK-NEXT:      <array>
// CHECK-NEXT:       <dict>
// CHECK-NEXT:        <key>start</key>
// CHECK-NEXT:         <array>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>14</integer>
// CHECK-NEXT:           <key>col</key><integer>21</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>14</integer>
// CHECK-NEXT:           <key>col</key><integer>26</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:         </array>
// CHECK-NEXT:        <key>end</key>
// CHECK-NEXT:         <array>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>14</integer>
// CHECK-NEXT:           <key>col</key><integer>28</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>14</integer>
// CHECK-NEXT:           <key>col</key><integer>28</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:         </array>
// CHECK-NEXT:       </dict>
// CHECK-NEXT:      </array>
// CHECK-NEXT:    </dict>
// CHECK-NEXT:    <dict>
// CHECK-NEXT:     <key>kind</key><string>event</string>
// CHECK-NEXT:     <key>location</key>
// CHECK-NEXT:     <dict>
// CHECK-NEXT:      <key>line</key><integer>14</integer>
// CHECK-NEXT:      <key>col</key><integer>28</integer>
// CHECK-NEXT:      <key>file</key><integer>0</integer>
// CHECK-NEXT:     </dict>
// CHECK-NEXT:     <key>ranges</key>
// CHECK-NEXT:     <array>
// CHECK-NEXT:       <array>
// CHECK-NEXT:        <dict>
// CHECK-NEXT:         <key>line</key><integer>14</integer>
// CHECK-NEXT:         <key>col</key><integer>28</integer>
// CHECK-NEXT:         <key>file</key><integer>0</integer>
// CHECK-NEXT:        </dict>
// CHECK-NEXT:        <dict>
// CHECK-NEXT:         <key>line</key><integer>14</integer>
// CHECK-NEXT:         <key>col</key><integer>28</integer>
// CHECK-NEXT:         <key>file</key><integer>0</integer>
// CHECK-NEXT:        </dict>
// CHECK-NEXT:       </array>
// CHECK-NEXT:     </array>
// CHECK-NEXT:     <key>depth</key><integer>0</integer>
// CHECK-NEXT:     <key>extended_message</key>
// CHECK-NEXT:     <string>debug.DumpBugHash$int f()$28$constexprintf(){return5;}$debug [debug.DumpBugHash]</string>
// CHECK-NEXT:     <key>message</key>
// CHECK-NEXT:     <string>debug.DumpBugHash$int f()$28$constexprintf(){return5;}$debug [debug.DumpBugHash]</string>
// CHECK-NEXT:    </dict>
// CHECK-NEXT:   </array>
// CHECK-NEXT:   <key>description</key><string>debug.DumpBugHash$int f()$28$constexprintf(){return5;}$debug [debug.DumpBugHash]</string>
// CHECK-NEXT:   <key>category</key><string>debug</string>
// CHECK-NEXT:   <key>type</key><string>Dump hash components</string>
// CHECK-NEXT:   <key>check_name</key><string>debug.DumpBugHash</string>
// CHECK-NEXT:   <!-- This hash is experimental and going to change! -->
// CHECK-NEXT:   <key>issue_hash_content_of_line_in_context</key><string>f5471f52854dc14167fe96db50c4ba5f</string>
// CHECK-NEXT:  <key>issue_context_kind</key><string>function</string>
// CHECK-NEXT:  <key>issue_context</key><string>f</string>
// CHECK-NEXT:  <key>issue_hash_function_offset</key><string>0</string>
// CHECK-NEXT:  <key>location</key>
// CHECK-NEXT:  <dict>
// CHECK-NEXT:   <key>line</key><integer>14</integer>
// CHECK-NEXT:   <key>col</key><integer>28</integer>
// CHECK-NEXT:   <key>file</key><integer>0</integer>
// CHECK-NEXT:  </dict>
// CHECK-NEXT:  </dict>
// CHECK-NEXT:  <dict>
// CHECK-NEXT:   <key>path</key>
// CHECK-NEXT:   <array>
// CHECK-NEXT:    <dict>
// CHECK-NEXT:     <key>kind</key><string>event</string>
// CHECK-NEXT:     <key>location</key>
// CHECK-NEXT:     <dict>
// CHECK-NEXT:      <key>line</key><integer>19</integer>
// CHECK-NEXT:      <key>col</key><integer>16</integer>
// CHECK-NEXT:      <key>file</key><integer>0</integer>
// CHECK-NEXT:     </dict>
// CHECK-NEXT:     <key>ranges</key>
// CHECK-NEXT:     <array>
// CHECK-NEXT:       <array>
// CHECK-NEXT:        <dict>
// CHECK-NEXT:         <key>line</key><integer>19</integer>
// CHECK-NEXT:         <key>col</key><integer>16</integer>
// CHECK-NEXT:         <key>file</key><integer>0</integer>
// CHECK-NEXT:        </dict>
// CHECK-NEXT:        <dict>
// CHECK-NEXT:         <key>line</key><integer>19</integer>
// CHECK-NEXT:         <key>col</key><integer>16</integer>
// CHECK-NEXT:         <key>file</key><integer>0</integer>
// CHECK-NEXT:        </dict>
// CHECK-NEXT:       </array>
// CHECK-NEXT:     </array>
// CHECK-NEXT:     <key>depth</key><integer>0</integer>
// CHECK-NEXT:     <key>extended_message</key>
// CHECK-NEXT:     <string>debug.DumpBugHash$AA::X::X()$16$X():priv(5){priv=0;}$debug [debug.DumpBugHash]</string>
// CHECK-NEXT:     <key>message</key>
// CHECK-NEXT:     <string>debug.DumpBugHash$AA::X::X()$16$X():priv(5){priv=0;}$debug [debug.DumpBugHash]</string>
// CHECK-NEXT:    </dict>
// CHECK-NEXT:   </array>
// CHECK-NEXT:   <key>description</key><string>debug.DumpBugHash$AA::X::X()$16$X():priv(5){priv=0;}$debug [debug.DumpBugHash]</string>
// CHECK-NEXT:   <key>category</key><string>debug</string>
// CHECK-NEXT:   <key>type</key><string>Dump hash components</string>
// CHECK-NEXT:   <key>check_name</key><string>debug.DumpBugHash</string>
// CHECK-NEXT:   <!-- This hash is experimental and going to change! -->
// CHECK-NEXT:   <key>issue_hash_content_of_line_in_context</key><string>d23266517ac17d5ec5e2fbbdb1922af1</string>
// CHECK-NEXT:  <key>issue_hash_function_offset</key><string>0</string>
// CHECK-NEXT:  <key>location</key>
// CHECK-NEXT:  <dict>
// CHECK-NEXT:   <key>line</key><integer>19</integer>
// CHECK-NEXT:   <key>col</key><integer>16</integer>
// CHECK-NEXT:   <key>file</key><integer>0</integer>
// CHECK-NEXT:  </dict>
// CHECK-NEXT:  </dict>
// CHECK-NEXT:  <dict>
// CHECK-NEXT:   <key>path</key>
// CHECK-NEXT:   <array>
// CHECK-NEXT:    <dict>
// CHECK-NEXT:     <key>kind</key><string>control</string>
// CHECK-NEXT:     <key>edges</key>
// CHECK-NEXT:      <array>
// CHECK-NEXT:       <dict>
// CHECK-NEXT:        <key>start</key>
// CHECK-NEXT:         <array>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>19</integer>
// CHECK-NEXT:           <key>col</key><integer>16</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>19</integer>
// CHECK-NEXT:           <key>col</key><integer>16</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:         </array>
// CHECK-NEXT:        <key>end</key>
// CHECK-NEXT:         <array>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>19</integer>
// CHECK-NEXT:           <key>col</key><integer>21</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>19</integer>
// CHECK-NEXT:           <key>col</key><integer>24</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:         </array>
// CHECK-NEXT:       </dict>
// CHECK-NEXT:      </array>
// CHECK-NEXT:    </dict>
// CHECK-NEXT:    <dict>
// CHECK-NEXT:     <key>kind</key><string>event</string>
// CHECK-NEXT:     <key>location</key>
// CHECK-NEXT:     <dict>
// CHECK-NEXT:      <key>line</key><integer>19</integer>
// CHECK-NEXT:      <key>col</key><integer>21</integer>
// CHECK-NEXT:      <key>file</key><integer>0</integer>
// CHECK-NEXT:     </dict>
// CHECK-NEXT:     <key>ranges</key>
// CHECK-NEXT:     <array>
// CHECK-NEXT:       <array>
// CHECK-NEXT:        <dict>
// CHECK-NEXT:         <key>line</key><integer>19</integer>
// CHECK-NEXT:         <key>col</key><integer>21</integer>
// CHECK-NEXT:         <key>file</key><integer>0</integer>
// CHECK-NEXT:        </dict>
// CHECK-NEXT:        <dict>
// CHECK-NEXT:         <key>line</key><integer>19</integer>
// CHECK-NEXT:         <key>col</key><integer>24</integer>
// CHECK-NEXT:         <key>file</key><integer>0</integer>
// CHECK-NEXT:        </dict>
// CHECK-NEXT:       </array>
// CHECK-NEXT:     </array>
// CHECK-NEXT:     <key>depth</key><integer>0</integer>
// CHECK-NEXT:     <key>extended_message</key>
// CHECK-NEXT:     <string>debug.DumpBugHash$AA::X::X()$21$X():priv(5){priv=0;}$debug [debug.DumpBugHash]</string>
// CHECK-NEXT:     <key>message</key>
// CHECK-NEXT:     <string>debug.DumpBugHash$AA::X::X()$21$X():priv(5){priv=0;}$debug [debug.DumpBugHash]</string>
// CHECK-NEXT:    </dict>
// CHECK-NEXT:   </array>
// CHECK-NEXT:   <key>description</key><string>debug.DumpBugHash$AA::X::X()$21$X():priv(5){priv=0;}$debug [debug.DumpBugHash]</string>
// CHECK-NEXT:   <key>category</key><string>debug</string>
// CHECK-NEXT:   <key>type</key><string>Dump hash components</string>
// CHECK-NEXT:   <key>check_name</key><string>debug.DumpBugHash</string>
// CHECK-NEXT:   <!-- This hash is experimental and going to change! -->
// CHECK-NEXT:   <key>issue_hash_content_of_line_in_context</key><string>7bfcc45512a6a3f61dda6e3ecebc7384</string>
// CHECK-NEXT:  <key>issue_hash_function_offset</key><string>0</string>
// CHECK-NEXT:  <key>location</key>
// CHECK-NEXT:  <dict>
// CHECK-NEXT:   <key>line</key><integer>19</integer>
// CHECK-NEXT:   <key>col</key><integer>21</integer>
// CHECK-NEXT:   <key>file</key><integer>0</integer>
// CHECK-NEXT:  </dict>
// CHECK-NEXT:  </dict>
// CHECK-NEXT:  <dict>
// CHECK-NEXT:   <key>path</key>
// CHECK-NEXT:   <array>
// CHECK-NEXT:    <dict>
// CHECK-NEXT:     <key>kind</key><string>control</string>
// CHECK-NEXT:     <key>edges</key>
// CHECK-NEXT:      <array>
// CHECK-NEXT:       <dict>
// CHECK-NEXT:        <key>start</key>
// CHECK-NEXT:         <array>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>19</integer>
// CHECK-NEXT:           <key>col</key><integer>16</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>19</integer>
// CHECK-NEXT:           <key>col</key><integer>16</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:         </array>
// CHECK-NEXT:        <key>end</key>
// CHECK-NEXT:         <array>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>19</integer>
// CHECK-NEXT:           <key>col</key><integer>21</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>19</integer>
// CHECK-NEXT:           <key>col</key><integer>24</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:         </array>
// CHECK-NEXT:       </dict>
// CHECK-NEXT:      </array>
// CHECK-NEXT:    </dict>
// CHECK-NEXT:    <dict>
// CHECK-NEXT:     <key>kind</key><string>control</string>
// CHECK-NEXT:     <key>edges</key>
// CHECK-NEXT:      <array>
// CHECK-NEXT:       <dict>
// CHECK-NEXT:        <key>start</key>
// CHECK-NEXT:         <array>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>19</integer>
// CHECK-NEXT:           <key>col</key><integer>21</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>19</integer>
// CHECK-NEXT:           <key>col</key><integer>24</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:         </array>
// CHECK-NEXT:        <key>end</key>
// CHECK-NEXT:         <array>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>19</integer>
// CHECK-NEXT:           <key>col</key><integer>26</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>19</integer>
// CHECK-NEXT:           <key>col</key><integer>26</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:         </array>
// CHECK-NEXT:       </dict>
// CHECK-NEXT:      </array>
// CHECK-NEXT:    </dict>
// CHECK-NEXT:    <dict>
// CHECK-NEXT:     <key>kind</key><string>event</string>
// CHECK-NEXT:     <key>location</key>
// CHECK-NEXT:     <dict>
// CHECK-NEXT:      <key>line</key><integer>19</integer>
// CHECK-NEXT:      <key>col</key><integer>26</integer>
// CHECK-NEXT:      <key>file</key><integer>0</integer>
// CHECK-NEXT:     </dict>
// CHECK-NEXT:     <key>ranges</key>
// CHECK-NEXT:     <array>
// CHECK-NEXT:       <array>
// CHECK-NEXT:        <dict>
// CHECK-NEXT:         <key>line</key><integer>19</integer>
// CHECK-NEXT:         <key>col</key><integer>21</integer>
// CHECK-NEXT:         <key>file</key><integer>0</integer>
// CHECK-NEXT:        </dict>
// CHECK-NEXT:        <dict>
// CHECK-NEXT:         <key>line</key><integer>19</integer>
// CHECK-NEXT:         <key>col</key><integer>28</integer>
// CHECK-NEXT:         <key>file</key><integer>0</integer>
// CHECK-NEXT:        </dict>
// CHECK-NEXT:       </array>
// CHECK-NEXT:     </array>
// CHECK-NEXT:     <key>depth</key><integer>0</integer>
// CHECK-NEXT:     <key>extended_message</key>
// CHECK-NEXT:     <string>debug.DumpBugHash$AA::X::X()$21$X():priv(5){priv=0;}$debug [debug.DumpBugHash]</string>
// CHECK-NEXT:     <key>message</key>
// CHECK-NEXT:     <string>debug.DumpBugHash$AA::X::X()$21$X():priv(5){priv=0;}$debug [debug.DumpBugHash]</string>
// CHECK-NEXT:    </dict>
// CHECK-NEXT:   </array>
// CHECK-NEXT:   <key>description</key><string>debug.DumpBugHash$AA::X::X()$21$X():priv(5){priv=0;}$debug [debug.DumpBugHash]</string>
// CHECK-NEXT:   <key>category</key><string>debug</string>
// CHECK-NEXT:   <key>type</key><string>Dump hash components</string>
// CHECK-NEXT:   <key>check_name</key><string>debug.DumpBugHash</string>
// CHECK-NEXT:   <!-- This hash is experimental and going to change! -->
// CHECK-NEXT:   <key>issue_hash_content_of_line_in_context</key><string>95dbfbcdd1dd6401d262994c45d088be</string>
// CHECK-NEXT:  <key>issue_hash_function_offset</key><string>0</string>
// CHECK-NEXT:  <key>location</key>
// CHECK-NEXT:  <dict>
// CHECK-NEXT:   <key>line</key><integer>19</integer>
// CHECK-NEXT:   <key>col</key><integer>26</integer>
// CHECK-NEXT:   <key>file</key><integer>0</integer>
// CHECK-NEXT:  </dict>
// CHECK-NEXT:  </dict>
// CHECK-NEXT:  <dict>
// CHECK-NEXT:   <key>path</key>
// CHECK-NEXT:   <array>
// CHECK-NEXT:    <dict>
// CHECK-NEXT:     <key>kind</key><string>control</string>
// CHECK-NEXT:     <key>edges</key>
// CHECK-NEXT:      <array>
// CHECK-NEXT:       <dict>
// CHECK-NEXT:        <key>start</key>
// CHECK-NEXT:         <array>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>19</integer>
// CHECK-NEXT:           <key>col</key><integer>16</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>19</integer>
// CHECK-NEXT:           <key>col</key><integer>16</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:         </array>
// CHECK-NEXT:        <key>end</key>
// CHECK-NEXT:         <array>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>19</integer>
// CHECK-NEXT:           <key>col</key><integer>21</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>19</integer>
// CHECK-NEXT:           <key>col</key><integer>24</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:         </array>
// CHECK-NEXT:       </dict>
// CHECK-NEXT:      </array>
// CHECK-NEXT:    </dict>
// CHECK-NEXT:    <dict>
// CHECK-NEXT:     <key>kind</key><string>control</string>
// CHECK-NEXT:     <key>edges</key>
// CHECK-NEXT:      <array>
// CHECK-NEXT:       <dict>
// CHECK-NEXT:        <key>start</key>
// CHECK-NEXT:         <array>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>19</integer>
// CHECK-NEXT:           <key>col</key><integer>21</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>19</integer>
// CHECK-NEXT:           <key>col</key><integer>24</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:         </array>
// CHECK-NEXT:        <key>end</key>
// CHECK-NEXT:         <array>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>19</integer>
// CHECK-NEXT:           <key>col</key><integer>28</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>19</integer>
// CHECK-NEXT:           <key>col</key><integer>28</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:         </array>
// CHECK-NEXT:       </dict>
// CHECK-NEXT:      </array>
// CHECK-NEXT:    </dict>
// CHECK-NEXT:    <dict>
// CHECK-NEXT:     <key>kind</key><string>event</string>
// CHECK-NEXT:     <key>location</key>
// CHECK-NEXT:     <dict>
// CHECK-NEXT:      <key>line</key><integer>19</integer>
// CHECK-NEXT:      <key>col</key><integer>28</integer>
// CHECK-NEXT:      <key>file</key><integer>0</integer>
// CHECK-NEXT:     </dict>
// CHECK-NEXT:     <key>ranges</key>
// CHECK-NEXT:     <array>
// CHECK-NEXT:       <array>
// CHECK-NEXT:        <dict>
// CHECK-NEXT:         <key>line</key><integer>19</integer>
// CHECK-NEXT:         <key>col</key><integer>28</integer>
// CHECK-NEXT:         <key>file</key><integer>0</integer>
// CHECK-NEXT:        </dict>
// CHECK-NEXT:        <dict>
// CHECK-NEXT:         <key>line</key><integer>19</integer>
// CHECK-NEXT:         <key>col</key><integer>28</integer>
// CHECK-NEXT:         <key>file</key><integer>0</integer>
// CHECK-NEXT:        </dict>
// CHECK-NEXT:       </array>
// CHECK-NEXT:     </array>
// CHECK-NEXT:     <key>depth</key><integer>0</integer>
// CHECK-NEXT:     <key>extended_message</key>
// CHECK-NEXT:     <string>debug.DumpBugHash$AA::X::X()$28$X():priv(5){priv=0;}$debug [debug.DumpBugHash]</string>
// CHECK-NEXT:     <key>message</key>
// CHECK-NEXT:     <string>debug.DumpBugHash$AA::X::X()$28$X():priv(5){priv=0;}$debug [debug.DumpBugHash]</string>
// CHECK-NEXT:    </dict>
// CHECK-NEXT:   </array>
// CHECK-NEXT:   <key>description</key><string>debug.DumpBugHash$AA::X::X()$28$X():priv(5){priv=0;}$debug [debug.DumpBugHash]</string>
// CHECK-NEXT:   <key>category</key><string>debug</string>
// CHECK-NEXT:   <key>type</key><string>Dump hash components</string>
// CHECK-NEXT:   <key>check_name</key><string>debug.DumpBugHash</string>
// CHECK-NEXT:   <!-- This hash is experimental and going to change! -->
// CHECK-NEXT:   <key>issue_hash_content_of_line_in_context</key><string>064a01551caa8eca3202f1fd55b9c692</string>
// CHECK-NEXT:  <key>issue_hash_function_offset</key><string>0</string>
// CHECK-NEXT:  <key>location</key>
// CHECK-NEXT:  <dict>
// CHECK-NEXT:   <key>line</key><integer>19</integer>
// CHECK-NEXT:   <key>col</key><integer>28</integer>
// CHECK-NEXT:   <key>file</key><integer>0</integer>
// CHECK-NEXT:  </dict>
// CHECK-NEXT:  </dict>
// CHECK-NEXT:  <dict>
// CHECK-NEXT:   <key>path</key>
// CHECK-NEXT:   <array>
// CHECK-NEXT:    <dict>
// CHECK-NEXT:     <key>kind</key><string>control</string>
// CHECK-NEXT:     <key>edges</key>
// CHECK-NEXT:      <array>
// CHECK-NEXT:       <dict>
// CHECK-NEXT:        <key>start</key>
// CHECK-NEXT:         <array>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>22</integer>
// CHECK-NEXT:           <key>col</key><integer>7</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>22</integer>
// CHECK-NEXT:           <key>col</key><integer>12</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:         </array>
// CHECK-NEXT:        <key>end</key>
// CHECK-NEXT:         <array>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>22</integer>
// CHECK-NEXT:           <key>col</key><integer>14</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>22</integer>
// CHECK-NEXT:           <key>col</key><integer>14</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:         </array>
// CHECK-NEXT:       </dict>
// CHECK-NEXT:      </array>
// CHECK-NEXT:    </dict>
// CHECK-NEXT:    <dict>
// CHECK-NEXT:     <key>kind</key><string>event</string>
// CHECK-NEXT:     <key>location</key>
// CHECK-NEXT:     <dict>
// CHECK-NEXT:      <key>line</key><integer>22</integer>
// CHECK-NEXT:      <key>col</key><integer>14</integer>
// CHECK-NEXT:      <key>file</key><integer>0</integer>
// CHECK-NEXT:     </dict>
// CHECK-NEXT:     <key>ranges</key>
// CHECK-NEXT:     <array>
// CHECK-NEXT:       <array>
// CHECK-NEXT:        <dict>
// CHECK-NEXT:         <key>line</key><integer>22</integer>
// CHECK-NEXT:         <key>col</key><integer>14</integer>
// CHECK-NEXT:         <key>file</key><integer>0</integer>
// CHECK-NEXT:        </dict>
// CHECK-NEXT:        <dict>
// CHECK-NEXT:         <key>line</key><integer>22</integer>
// CHECK-NEXT:         <key>col</key><integer>14</integer>
// CHECK-NEXT:         <key>file</key><integer>0</integer>
// CHECK-NEXT:        </dict>
// CHECK-NEXT:       </array>
// CHECK-NEXT:     </array>
// CHECK-NEXT:     <key>depth</key><integer>0</integer>
// CHECK-NEXT:     <key>extended_message</key>
// CHECK-NEXT:     <string>debug.DumpBugHash$int AA::X::static_method()$14$return5;$debug [debug.DumpBugHash]</string>
// CHECK-NEXT:     <key>message</key>
// CHECK-NEXT:     <string>debug.DumpBugHash$int AA::X::static_method()$14$return5;$debug [debug.DumpBugHash]</string>
// CHECK-NEXT:    </dict>
// CHECK-NEXT:   </array>
// CHECK-NEXT:   <key>description</key><string>debug.DumpBugHash$int AA::X::static_method()$14$return5;$debug [debug.DumpBugHash]</string>
// CHECK-NEXT:   <key>category</key><string>debug</string>
// CHECK-NEXT:   <key>type</key><string>Dump hash components</string>
// CHECK-NEXT:   <key>check_name</key><string>debug.DumpBugHash</string>
// CHECK-NEXT:   <!-- This hash is experimental and going to change! -->
// CHECK-NEXT:   <key>issue_hash_content_of_line_in_context</key><string>651fcca72f8ad65771702903ecd5f68a</string>
// CHECK-NEXT:  <key>issue_context_kind</key><string>C++ method</string>
// CHECK-NEXT:  <key>issue_context</key><string>static_method</string>
// CHECK-NEXT:  <key>issue_hash_function_offset</key><string>1</string>
// CHECK-NEXT:  <key>location</key>
// CHECK-NEXT:  <dict>
// CHECK-NEXT:   <key>line</key><integer>22</integer>
// CHECK-NEXT:   <key>col</key><integer>14</integer>
// CHECK-NEXT:   <key>file</key><integer>0</integer>
// CHECK-NEXT:  </dict>
// CHECK-NEXT:  </dict>
// CHECK-NEXT:  <dict>
// CHECK-NEXT:   <key>path</key>
// CHECK-NEXT:   <array>
// CHECK-NEXT:    <dict>
// CHECK-NEXT:     <key>kind</key><string>control</string>
// CHECK-NEXT:     <key>edges</key>
// CHECK-NEXT:      <array>
// CHECK-NEXT:       <dict>
// CHECK-NEXT:        <key>start</key>
// CHECK-NEXT:         <array>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>32</integer>
// CHECK-NEXT:           <key>col</key><integer>7</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>32</integer>
// CHECK-NEXT:           <key>col</key><integer>12</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:         </array>
// CHECK-NEXT:        <key>end</key>
// CHECK-NEXT:         <array>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>32</integer>
// CHECK-NEXT:           <key>col</key><integer>14</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>32</integer>
// CHECK-NEXT:           <key>col</key><integer>14</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:         </array>
// CHECK-NEXT:       </dict>
// CHECK-NEXT:      </array>
// CHECK-NEXT:    </dict>
// CHECK-NEXT:    <dict>
// CHECK-NEXT:     <key>kind</key><string>event</string>
// CHECK-NEXT:     <key>location</key>
// CHECK-NEXT:     <dict>
// CHECK-NEXT:      <key>line</key><integer>32</integer>
// CHECK-NEXT:      <key>col</key><integer>14</integer>
// CHECK-NEXT:      <key>file</key><integer>0</integer>
// CHECK-NEXT:     </dict>
// CHECK-NEXT:     <key>ranges</key>
// CHECK-NEXT:     <array>
// CHECK-NEXT:       <array>
// CHECK-NEXT:        <dict>
// CHECK-NEXT:         <key>line</key><integer>32</integer>
// CHECK-NEXT:         <key>col</key><integer>14</integer>
// CHECK-NEXT:         <key>file</key><integer>0</integer>
// CHECK-NEXT:        </dict>
// CHECK-NEXT:        <dict>
// CHECK-NEXT:         <key>line</key><integer>32</integer>
// CHECK-NEXT:         <key>col</key><integer>14</integer>
// CHECK-NEXT:         <key>file</key><integer>0</integer>
// CHECK-NEXT:        </dict>
// CHECK-NEXT:       </array>
// CHECK-NEXT:     </array>
// CHECK-NEXT:     <key>depth</key><integer>0</integer>
// CHECK-NEXT:     <key>extended_message</key>
// CHECK-NEXT:     <string>debug.DumpBugHash$int AA::X::method() &amp;&amp;$14$return5;$debug [debug.DumpBugHash]</string>
// CHECK-NEXT:     <key>message</key>
// CHECK-NEXT:     <string>debug.DumpBugHash$int AA::X::method() &amp;&amp;$14$return5;$debug [debug.DumpBugHash]</string>
// CHECK-NEXT:    </dict>
// CHECK-NEXT:   </array>
// CHECK-NEXT:   <key>description</key><string>debug.DumpBugHash$int AA::X::method() &amp;&amp;$14$return5;$debug [debug.DumpBugHash]</string>
// CHECK-NEXT:   <key>category</key><string>debug</string>
// CHECK-NEXT:   <key>type</key><string>Dump hash components</string>
// CHECK-NEXT:   <key>check_name</key><string>debug.DumpBugHash</string>
// CHECK-NEXT:   <!-- This hash is experimental and going to change! -->
// CHECK-NEXT:   <key>issue_hash_content_of_line_in_context</key><string>c8ac8f24467234bea1f34adf5ad5007b</string>
// CHECK-NEXT:  <key>issue_context_kind</key><string>C++ method</string>
// CHECK-NEXT:  <key>issue_context</key><string>method</string>
// CHECK-NEXT:  <key>issue_hash_function_offset</key><string>7</string>
// CHECK-NEXT:  <key>location</key>
// CHECK-NEXT:  <dict>
// CHECK-NEXT:   <key>line</key><integer>32</integer>
// CHECK-NEXT:   <key>col</key><integer>14</integer>
// CHECK-NEXT:   <key>file</key><integer>0</integer>
// CHECK-NEXT:  </dict>
// CHECK-NEXT:  </dict>
// CHECK-NEXT:  <dict>
// CHECK-NEXT:   <key>path</key>
// CHECK-NEXT:   <array>
// CHECK-NEXT:    <dict>
// CHECK-NEXT:     <key>kind</key><string>control</string>
// CHECK-NEXT:     <key>edges</key>
// CHECK-NEXT:      <array>
// CHECK-NEXT:       <dict>
// CHECK-NEXT:        <key>start</key>
// CHECK-NEXT:         <array>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>38</integer>
// CHECK-NEXT:           <key>col</key><integer>7</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>38</integer>
// CHECK-NEXT:           <key>col</key><integer>12</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:         </array>
// CHECK-NEXT:        <key>end</key>
// CHECK-NEXT:         <array>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>38</integer>
// CHECK-NEXT:           <key>col</key><integer>14</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>38</integer>
// CHECK-NEXT:           <key>col</key><integer>14</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:         </array>
// CHECK-NEXT:       </dict>
// CHECK-NEXT:      </array>
// CHECK-NEXT:    </dict>
// CHECK-NEXT:    <dict>
// CHECK-NEXT:     <key>kind</key><string>event</string>
// CHECK-NEXT:     <key>location</key>
// CHECK-NEXT:     <dict>
// CHECK-NEXT:      <key>line</key><integer>38</integer>
// CHECK-NEXT:      <key>col</key><integer>14</integer>
// CHECK-NEXT:      <key>file</key><integer>0</integer>
// CHECK-NEXT:     </dict>
// CHECK-NEXT:     <key>ranges</key>
// CHECK-NEXT:     <array>
// CHECK-NEXT:       <array>
// CHECK-NEXT:        <dict>
// CHECK-NEXT:         <key>line</key><integer>38</integer>
// CHECK-NEXT:         <key>col</key><integer>14</integer>
// CHECK-NEXT:         <key>file</key><integer>0</integer>
// CHECK-NEXT:        </dict>
// CHECK-NEXT:        <dict>
// CHECK-NEXT:         <key>line</key><integer>38</integer>
// CHECK-NEXT:         <key>col</key><integer>18</integer>
// CHECK-NEXT:         <key>file</key><integer>0</integer>
// CHECK-NEXT:        </dict>
// CHECK-NEXT:       </array>
// CHECK-NEXT:     </array>
// CHECK-NEXT:     <key>depth</key><integer>0</integer>
// CHECK-NEXT:     <key>extended_message</key>
// CHECK-NEXT:     <string>debug.DumpBugHash$class AA::X &amp; AA::X::operator=(int)$14$return*this;$debug [debug.DumpBugHash]</string>
// CHECK-NEXT:     <key>message</key>
// CHECK-NEXT:     <string>debug.DumpBugHash$class AA::X &amp; AA::X::operator=(int)$14$return*this;$debug [debug.DumpBugHash]</string>
// CHECK-NEXT:    </dict>
// CHECK-NEXT:   </array>
// CHECK-NEXT:   <key>description</key><string>debug.DumpBugHash$class AA::X &amp; AA::X::operator=(int)$14$return*this;$debug [debug.DumpBugHash]</string>
// CHECK-NEXT:   <key>category</key><string>debug</string>
// CHECK-NEXT:   <key>type</key><string>Dump hash components</string>
// CHECK-NEXT:   <key>check_name</key><string>debug.DumpBugHash</string>
// CHECK-NEXT:   <!-- This hash is experimental and going to change! -->
// CHECK-NEXT:   <key>issue_hash_content_of_line_in_context</key><string>b47cf7973c9b459d9c99c483e722db8e</string>
// CHECK-NEXT:  <key>issue_context_kind</key><string>C++ method</string>
// CHECK-NEXT:  <key>issue_context</key><string>operator=</string>
// CHECK-NEXT:  <key>issue_hash_function_offset</key><string>1</string>
// CHECK-NEXT:  <key>location</key>
// CHECK-NEXT:  <dict>
// CHECK-NEXT:   <key>line</key><integer>38</integer>
// CHECK-NEXT:   <key>col</key><integer>14</integer>
// CHECK-NEXT:   <key>file</key><integer>0</integer>
// CHECK-NEXT:  </dict>
// CHECK-NEXT:  </dict>
// CHECK-NEXT:  <dict>
// CHECK-NEXT:   <key>path</key>
// CHECK-NEXT:   <array>
// CHECK-NEXT:    <dict>
// CHECK-NEXT:     <key>kind</key><string>control</string>
// CHECK-NEXT:     <key>edges</key>
// CHECK-NEXT:      <array>
// CHECK-NEXT:       <dict>
// CHECK-NEXT:        <key>start</key>
// CHECK-NEXT:         <array>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>42</integer>
// CHECK-NEXT:           <key>col</key><integer>7</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>42</integer>
// CHECK-NEXT:           <key>col</key><integer>12</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:         </array>
// CHECK-NEXT:        <key>end</key>
// CHECK-NEXT:         <array>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>42</integer>
// CHECK-NEXT:           <key>col</key><integer>14</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>42</integer>
// CHECK-NEXT:           <key>col</key><integer>14</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:         </array>
// CHECK-NEXT:       </dict>
// CHECK-NEXT:      </array>
// CHECK-NEXT:    </dict>
// CHECK-NEXT:    <dict>
// CHECK-NEXT:     <key>kind</key><string>event</string>
// CHECK-NEXT:     <key>location</key>
// CHECK-NEXT:     <dict>
// CHECK-NEXT:      <key>line</key><integer>42</integer>
// CHECK-NEXT:      <key>col</key><integer>14</integer>
// CHECK-NEXT:      <key>file</key><integer>0</integer>
// CHECK-NEXT:     </dict>
// CHECK-NEXT:     <key>ranges</key>
// CHECK-NEXT:     <array>
// CHECK-NEXT:       <array>
// CHECK-NEXT:        <dict>
// CHECK-NEXT:         <key>line</key><integer>42</integer>
// CHECK-NEXT:         <key>col</key><integer>14</integer>
// CHECK-NEXT:         <key>file</key><integer>0</integer>
// CHECK-NEXT:        </dict>
// CHECK-NEXT:        <dict>
// CHECK-NEXT:         <key>line</key><integer>42</integer>
// CHECK-NEXT:         <key>col</key><integer>14</integer>
// CHECK-NEXT:         <key>file</key><integer>0</integer>
// CHECK-NEXT:        </dict>
// CHECK-NEXT:       </array>
// CHECK-NEXT:     </array>
// CHECK-NEXT:     <key>depth</key><integer>0</integer>
// CHECK-NEXT:     <key>extended_message</key>
// CHECK-NEXT:     <string>debug.DumpBugHash$AA::X::operator int()$14$return0;$debug [debug.DumpBugHash]</string>
// CHECK-NEXT:     <key>message</key>
// CHECK-NEXT:     <string>debug.DumpBugHash$AA::X::operator int()$14$return0;$debug [debug.DumpBugHash]</string>
// CHECK-NEXT:    </dict>
// CHECK-NEXT:   </array>
// CHECK-NEXT:   <key>description</key><string>debug.DumpBugHash$AA::X::operator int()$14$return0;$debug [debug.DumpBugHash]</string>
// CHECK-NEXT:   <key>category</key><string>debug</string>
// CHECK-NEXT:   <key>type</key><string>Dump hash components</string>
// CHECK-NEXT:   <key>check_name</key><string>debug.DumpBugHash</string>
// CHECK-NEXT:   <!-- This hash is experimental and going to change! -->
// CHECK-NEXT:   <key>issue_hash_content_of_line_in_context</key><string>0cbb0e1e5b03ba5b4f7f8f17504de671</string>
// CHECK-NEXT:  <key>issue_hash_function_offset</key><string>1</string>
// CHECK-NEXT:  <key>location</key>
// CHECK-NEXT:  <dict>
// CHECK-NEXT:   <key>line</key><integer>42</integer>
// CHECK-NEXT:   <key>col</key><integer>14</integer>
// CHECK-NEXT:   <key>file</key><integer>0</integer>
// CHECK-NEXT:  </dict>
// CHECK-NEXT:  </dict>
// CHECK-NEXT:  <dict>
// CHECK-NEXT:   <key>path</key>
// CHECK-NEXT:   <array>
// CHECK-NEXT:    <dict>
// CHECK-NEXT:     <key>kind</key><string>control</string>
// CHECK-NEXT:     <key>edges</key>
// CHECK-NEXT:      <array>
// CHECK-NEXT:       <dict>
// CHECK-NEXT:        <key>start</key>
// CHECK-NEXT:         <array>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>46</integer>
// CHECK-NEXT:           <key>col</key><integer>7</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>46</integer>
// CHECK-NEXT:           <key>col</key><integer>12</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:         </array>
// CHECK-NEXT:        <key>end</key>
// CHECK-NEXT:         <array>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>46</integer>
// CHECK-NEXT:           <key>col</key><integer>14</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>46</integer>
// CHECK-NEXT:           <key>col</key><integer>14</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:         </array>
// CHECK-NEXT:       </dict>
// CHECK-NEXT:      </array>
// CHECK-NEXT:    </dict>
// CHECK-NEXT:    <dict>
// CHECK-NEXT:     <key>kind</key><string>event</string>
// CHECK-NEXT:     <key>location</key>
// CHECK-NEXT:     <dict>
// CHECK-NEXT:      <key>line</key><integer>46</integer>
// CHECK-NEXT:      <key>col</key><integer>14</integer>
// CHECK-NEXT:      <key>file</key><integer>0</integer>
// CHECK-NEXT:     </dict>
// CHECK-NEXT:     <key>ranges</key>
// CHECK-NEXT:     <array>
// CHECK-NEXT:       <array>
// CHECK-NEXT:        <dict>
// CHECK-NEXT:         <key>line</key><integer>46</integer>
// CHECK-NEXT:         <key>col</key><integer>14</integer>
// CHECK-NEXT:         <key>file</key><integer>0</integer>
// CHECK-NEXT:        </dict>
// CHECK-NEXT:        <dict>
// CHECK-NEXT:         <key>line</key><integer>46</integer>
// CHECK-NEXT:         <key>col</key><integer>14</integer>
// CHECK-NEXT:         <key>file</key><integer>0</integer>
// CHECK-NEXT:        </dict>
// CHECK-NEXT:       </array>
// CHECK-NEXT:     </array>
// CHECK-NEXT:     <key>depth</key><integer>0</integer>
// CHECK-NEXT:     <key>extended_message</key>
// CHECK-NEXT:     <string>debug.DumpBugHash$AA::X::operator float()$14$return0;$debug [debug.DumpBugHash]</string>
// CHECK-NEXT:     <key>message</key>
// CHECK-NEXT:     <string>debug.DumpBugHash$AA::X::operator float()$14$return0;$debug [debug.DumpBugHash]</string>
// CHECK-NEXT:    </dict>
// CHECK-NEXT:   </array>
// CHECK-NEXT:   <key>description</key><string>debug.DumpBugHash$AA::X::operator float()$14$return0;$debug [debug.DumpBugHash]</string>
// CHECK-NEXT:   <key>category</key><string>debug</string>
// CHECK-NEXT:   <key>type</key><string>Dump hash components</string>
// CHECK-NEXT:   <key>check_name</key><string>debug.DumpBugHash</string>
// CHECK-NEXT:   <!-- This hash is experimental and going to change! -->
// CHECK-NEXT:   <key>issue_hash_content_of_line_in_context</key><string>df306826bf89e50c1b55e1d379a761b3</string>
// CHECK-NEXT:  <key>issue_hash_function_offset</key><string>1</string>
// CHECK-NEXT:  <key>location</key>
// CHECK-NEXT:  <dict>
// CHECK-NEXT:   <key>line</key><integer>46</integer>
// CHECK-NEXT:   <key>col</key><integer>14</integer>
// CHECK-NEXT:   <key>file</key><integer>0</integer>
// CHECK-NEXT:  </dict>
// CHECK-NEXT:  </dict>
// CHECK-NEXT:  <dict>
// CHECK-NEXT:   <key>path</key>
// CHECK-NEXT:   <array>
// CHECK-NEXT:    <dict>
// CHECK-NEXT:     <key>kind</key><string>control</string>
// CHECK-NEXT:     <key>edges</key>
// CHECK-NEXT:      <array>
// CHECK-NEXT:       <dict>
// CHECK-NEXT:        <key>start</key>
// CHECK-NEXT:         <array>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>52</integer>
// CHECK-NEXT:           <key>col</key><integer>3</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>52</integer>
// CHECK-NEXT:           <key>col</key><integer>8</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:         </array>
// CHECK-NEXT:        <key>end</key>
// CHECK-NEXT:         <array>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>52</integer>
// CHECK-NEXT:           <key>col</key><integer>10</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>52</integer>
// CHECK-NEXT:           <key>col</key><integer>10</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:         </array>
// CHECK-NEXT:       </dict>
// CHECK-NEXT:      </array>
// CHECK-NEXT:    </dict>
// CHECK-NEXT:    <dict>
// CHECK-NEXT:     <key>kind</key><string>event</string>
// CHECK-NEXT:     <key>location</key>
// CHECK-NEXT:     <dict>
// CHECK-NEXT:      <key>line</key><integer>52</integer>
// CHECK-NEXT:      <key>col</key><integer>10</integer>
// CHECK-NEXT:      <key>file</key><integer>0</integer>
// CHECK-NEXT:     </dict>
// CHECK-NEXT:     <key>ranges</key>
// CHECK-NEXT:     <array>
// CHECK-NEXT:       <array>
// CHECK-NEXT:        <dict>
// CHECK-NEXT:         <key>line</key><integer>52</integer>
// CHECK-NEXT:         <key>col</key><integer>10</integer>
// CHECK-NEXT:         <key>file</key><integer>0</integer>
// CHECK-NEXT:        </dict>
// CHECK-NEXT:        <dict>
// CHECK-NEXT:         <key>line</key><integer>52</integer>
// CHECK-NEXT:         <key>col</key><integer>10</integer>
// CHECK-NEXT:         <key>file</key><integer>0</integer>
// CHECK-NEXT:        </dict>
// CHECK-NEXT:       </array>
// CHECK-NEXT:     </array>
// CHECK-NEXT:     <key>depth</key><integer>0</integer>
// CHECK-NEXT:     <key>extended_message</key>
// CHECK-NEXT:     <string>debug.DumpBugHash$int AA::X::OutOfLine()$10$return5;$debug [debug.DumpBugHash]</string>
// CHECK-NEXT:     <key>message</key>
// CHECK-NEXT:     <string>debug.DumpBugHash$int AA::X::OutOfLine()$10$return5;$debug [debug.DumpBugHash]</string>
// CHECK-NEXT:    </dict>
// CHECK-NEXT:   </array>
// CHECK-NEXT:   <key>description</key><string>debug.DumpBugHash$int AA::X::OutOfLine()$10$return5;$debug [debug.DumpBugHash]</string>
// CHECK-NEXT:   <key>category</key><string>debug</string>
// CHECK-NEXT:   <key>type</key><string>Dump hash components</string>
// CHECK-NEXT:   <key>check_name</key><string>debug.DumpBugHash</string>
// CHECK-NEXT:   <!-- This hash is experimental and going to change! -->
// CHECK-NEXT:   <key>issue_hash_content_of_line_in_context</key><string>9dd7b17a6f62ed8c95b37a38cf71f3a9</string>
// CHECK-NEXT:  <key>issue_context_kind</key><string>C++ method</string>
// CHECK-NEXT:  <key>issue_context</key><string>OutOfLine</string>
// CHECK-NEXT:  <key>issue_hash_function_offset</key><string>1</string>
// CHECK-NEXT:  <key>location</key>
// CHECK-NEXT:  <dict>
// CHECK-NEXT:   <key>line</key><integer>52</integer>
// CHECK-NEXT:   <key>col</key><integer>10</integer>
// CHECK-NEXT:   <key>file</key><integer>0</integer>
// CHECK-NEXT:  </dict>
// CHECK-NEXT:  </dict>
// CHECK-NEXT:  <dict>
// CHECK-NEXT:   <key>path</key>
// CHECK-NEXT:   <array>
// CHECK-NEXT:    <dict>
// CHECK-NEXT:     <key>kind</key><string>event</string>
// CHECK-NEXT:     <key>location</key>
// CHECK-NEXT:     <dict>
// CHECK-NEXT:      <key>line</key><integer>56</integer>
// CHECK-NEXT:      <key>col</key><integer>3</integer>
// CHECK-NEXT:      <key>file</key><integer>0</integer>
// CHECK-NEXT:     </dict>
// CHECK-NEXT:     <key>ranges</key>
// CHECK-NEXT:     <array>
// CHECK-NEXT:       <array>
// CHECK-NEXT:        <dict>
// CHECK-NEXT:         <key>line</key><integer>56</integer>
// CHECK-NEXT:         <key>col</key><integer>3</integer>
// CHECK-NEXT:         <key>file</key><integer>0</integer>
// CHECK-NEXT:        </dict>
// CHECK-NEXT:        <dict>
// CHECK-NEXT:         <key>line</key><integer>58</integer>
// CHECK-NEXT:         <key>col</key><integer>3</integer>
// CHECK-NEXT:         <key>file</key><integer>0</integer>
// CHECK-NEXT:        </dict>
// CHECK-NEXT:       </array>
// CHECK-NEXT:     </array>
// CHECK-NEXT:     <key>depth</key><integer>0</integer>
// CHECK-NEXT:     <key>extended_message</key>
// CHECK-NEXT:     <string>debug.DumpBugHash$void testLambda()$3$[](){$debug [debug.DumpBugHash]</string>
// CHECK-NEXT:     <key>message</key>
// CHECK-NEXT:     <string>debug.DumpBugHash$void testLambda()$3$[](){$debug [debug.DumpBugHash]</string>
// CHECK-NEXT:    </dict>
// CHECK-NEXT:   </array>
// CHECK-NEXT:   <key>description</key><string>debug.DumpBugHash$void testLambda()$3$[](){$debug [debug.DumpBugHash]</string>
// CHECK-NEXT:   <key>category</key><string>debug</string>
// CHECK-NEXT:   <key>type</key><string>Dump hash components</string>
// CHECK-NEXT:   <key>check_name</key><string>debug.DumpBugHash</string>
// CHECK-NEXT:   <!-- This hash is experimental and going to change! -->
// CHECK-NEXT:   <key>issue_hash_content_of_line_in_context</key><string>6ad4400e40885a78a0f57f585421a515</string>
// CHECK-NEXT:  <key>issue_context_kind</key><string>function</string>
// CHECK-NEXT:  <key>issue_context</key><string>testLambda</string>
// CHECK-NEXT:  <key>issue_hash_function_offset</key><string>1</string>
// CHECK-NEXT:  <key>location</key>
// CHECK-NEXT:  <dict>
// CHECK-NEXT:   <key>line</key><integer>56</integer>
// CHECK-NEXT:   <key>col</key><integer>3</integer>
// CHECK-NEXT:   <key>file</key><integer>0</integer>
// CHECK-NEXT:  </dict>
// CHECK-NEXT:  </dict>
// CHECK-NEXT:  <dict>
// CHECK-NEXT:   <key>path</key>
// CHECK-NEXT:   <array>
// CHECK-NEXT:    <dict>
// CHECK-NEXT:     <key>kind</key><string>event</string>
// CHECK-NEXT:     <key>location</key>
// CHECK-NEXT:     <dict>
// CHECK-NEXT:      <key>line</key><integer>56</integer>
// CHECK-NEXT:      <key>col</key><integer>3</integer>
// CHECK-NEXT:      <key>file</key><integer>0</integer>
// CHECK-NEXT:     </dict>
// CHECK-NEXT:     <key>ranges</key>
// CHECK-NEXT:     <array>
// CHECK-NEXT:       <array>
// CHECK-NEXT:        <dict>
// CHECK-NEXT:         <key>line</key><integer>56</integer>
// CHECK-NEXT:         <key>col</key><integer>3</integer>
// CHECK-NEXT:         <key>file</key><integer>0</integer>
// CHECK-NEXT:        </dict>
// CHECK-NEXT:        <dict>
// CHECK-NEXT:         <key>line</key><integer>58</integer>
// CHECK-NEXT:         <key>col</key><integer>5</integer>
// CHECK-NEXT:         <key>file</key><integer>0</integer>
// CHECK-NEXT:        </dict>
// CHECK-NEXT:       </array>
// CHECK-NEXT:     </array>
// CHECK-NEXT:     <key>depth</key><integer>0</integer>
// CHECK-NEXT:     <key>extended_message</key>
// CHECK-NEXT:     <string>debug.DumpBugHash$void testLambda()$3$[](){$debug [debug.DumpBugHash]</string>
// CHECK-NEXT:     <key>message</key>
// CHECK-NEXT:     <string>debug.DumpBugHash$void testLambda()$3$[](){$debug [debug.DumpBugHash]</string>
// CHECK-NEXT:    </dict>
// CHECK-NEXT:   </array>
// CHECK-NEXT:   <key>description</key><string>debug.DumpBugHash$void testLambda()$3$[](){$debug [debug.DumpBugHash]</string>
// CHECK-NEXT:   <key>category</key><string>debug</string>
// CHECK-NEXT:   <key>type</key><string>Dump hash components</string>
// CHECK-NEXT:   <key>check_name</key><string>debug.DumpBugHash</string>
// CHECK-NEXT:   <!-- This hash is experimental and going to change! -->
// CHECK-NEXT:   <key>issue_hash_content_of_line_in_context</key><string>6ad4400e40885a78a0f57f585421a515</string>
// CHECK-NEXT:  <key>issue_context_kind</key><string>function</string>
// CHECK-NEXT:  <key>issue_context</key><string>testLambda</string>
// CHECK-NEXT:  <key>issue_hash_function_offset</key><string>1</string>
// CHECK-NEXT:  <key>location</key>
// CHECK-NEXT:  <dict>
// CHECK-NEXT:   <key>line</key><integer>56</integer>
// CHECK-NEXT:   <key>col</key><integer>3</integer>
// CHECK-NEXT:   <key>file</key><integer>0</integer>
// CHECK-NEXT:  </dict>
// CHECK-NEXT:  </dict>
// CHECK-NEXT:  <dict>
// CHECK-NEXT:   <key>path</key>
// CHECK-NEXT:   <array>
// CHECK-NEXT:    <dict>
// CHECK-NEXT:     <key>kind</key><string>control</string>
// CHECK-NEXT:     <key>edges</key>
// CHECK-NEXT:      <array>
// CHECK-NEXT:       <dict>
// CHECK-NEXT:        <key>start</key>
// CHECK-NEXT:         <array>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>56</integer>
// CHECK-NEXT:           <key>col</key><integer>3</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>56</integer>
// CHECK-NEXT:           <key>col</key><integer>3</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:         </array>
// CHECK-NEXT:        <key>end</key>
// CHECK-NEXT:         <array>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>58</integer>
// CHECK-NEXT:           <key>col</key><integer>4</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:          <dict>
// CHECK-NEXT:           <key>line</key><integer>58</integer>
// CHECK-NEXT:           <key>col</key><integer>4</integer>
// CHECK-NEXT:           <key>file</key><integer>0</integer>
// CHECK-NEXT:          </dict>
// CHECK-NEXT:         </array>
// CHECK-NEXT:       </dict>
// CHECK-NEXT:      </array>
// CHECK-NEXT:    </dict>
// CHECK-NEXT:    <dict>
// CHECK-NEXT:     <key>kind</key><string>event</string>
// CHECK-NEXT:     <key>location</key>
// CHECK-NEXT:     <dict>
// CHECK-NEXT:      <key>line</key><integer>58</integer>
// CHECK-NEXT:      <key>col</key><integer>4</integer>
// CHECK-NEXT:      <key>file</key><integer>0</integer>
// CHECK-NEXT:     </dict>
// CHECK-NEXT:     <key>ranges</key>
// CHECK-NEXT:     <array>
// CHECK-NEXT:       <array>
// CHECK-NEXT:        <dict>
// CHECK-NEXT:         <key>line</key><integer>58</integer>
// CHECK-NEXT:         <key>col</key><integer>4</integer>
// CHECK-NEXT:         <key>file</key><integer>0</integer>
// CHECK-NEXT:        </dict>
// CHECK-NEXT:        <dict>
// CHECK-NEXT:         <key>line</key><integer>58</integer>
// CHECK-NEXT:         <key>col</key><integer>5</integer>
// CHECK-NEXT:         <key>file</key><integer>0</integer>
// CHECK-NEXT:        </dict>
// CHECK-NEXT:       </array>
// CHECK-NEXT:     </array>
// CHECK-NEXT:     <key>depth</key><integer>0</integer>
// CHECK-NEXT:     <key>extended_message</key>
// CHECK-NEXT:     <string>debug.DumpBugHash$void testLambda()$4$}();$debug [debug.DumpBugHash]</string>
// CHECK-NEXT:     <key>message</key>
// CHECK-NEXT:     <string>debug.DumpBugHash$void testLambda()$4$}();$debug [debug.DumpBugHash]</string>
// CHECK-NEXT:    </dict>
// CHECK-NEXT:   </array>
// CHECK-NEXT:   <key>description</key><string>debug.DumpBugHash$void testLambda()$4$}();$debug [debug.DumpBugHash]</string>
// CHECK-NEXT:   <key>category</key><string>debug</string>
// CHECK-NEXT:   <key>type</key><string>Dump hash components</string>
// CHECK-NEXT:   <key>check_name</key><string>debug.DumpBugHash</string>
// CHECK-NEXT:   <!-- This hash is experimental and going to change! -->
// CHECK-NEXT:   <key>issue_hash_content_of_line_in_context</key><string>378e6de75fb41b05bcef3950ad5ffa5e</string>
// CHECK-NEXT:  <key>issue_context_kind</key><string>function</string>
// CHECK-NEXT:  <key>issue_context</key><string>testLambda</string>
// CHECK-NEXT:  <key>issue_hash_function_offset</key><string>3</string>
// CHECK-NEXT:  <key>location</key>
// CHECK-NEXT:  <dict>
// CHECK-NEXT:   <key>line</key><integer>58</integer>
// CHECK-NEXT:   <key>col</key><integer>4</integer>
// CHECK-NEXT:   <key>file</key><integer>0</integer>
// CHECK-NEXT:  </dict>
// CHECK-NEXT:  </dict>
// CHECK-NEXT: </array>
