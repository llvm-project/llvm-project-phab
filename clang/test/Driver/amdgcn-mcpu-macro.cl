// RUN: %clang -target amdgcn -mcpu=gfx600 -E -dM %s | FileCheck --check-prefix=GFX600-CHECK %s
// RUN: %clang -target amdgcn -mcpu=tahiti -E -dM %s | FileCheck --check-prefix=TAHITI-CHECK %s
// RUN: %clang -target amdgcn -mcpu=gfx601 -E -dM %s | FileCheck --check-prefix=GFX601-CHECK %s
// RUN: %clang -target amdgcn -mcpu=pitcairn -E -dM %s | FileCheck --check-prefix=PITCAIRN-CHECK %s
// RUN: %clang -target amdgcn -mcpu=verde -E -dM %s | FileCheck --check-prefix=VERDE-CHECK %s
// RUN: %clang -target amdgcn -mcpu=oland -E -dM %s | FileCheck --check-prefix=OLAND-CHECK %s
// RUN: %clang -target amdgcn -mcpu=hainan -E -dM %s | FileCheck --check-prefix=HAINAN-CHECK %s
// RUN: %clang -target amdgcn -mcpu=gfx700 -E -dM %s | FileCheck --check-prefix=GFX700-CHECK %s
// RUN: %clang -target amdgcn -mcpu=bonaire -E -dM %s | FileCheck --check-prefix=BONAIRE-CHECK %s
// RUN: %clang -target amdgcn -mcpu=kaveri -E -dM %s | FileCheck --check-prefix=KAVERI-CHECK %s
// RUN: %clang -target amdgcn -mcpu=gfx701 -E -dM %s | FileCheck --check-prefix=GFX701-CHECK %s
// RUN: %clang -target amdgcn -mcpu=hawaii -E -dM %s | FileCheck --check-prefix=HAWAII-CHECK %s
// RUN: %clang -target amdgcn -mcpu=gfx702 -E -dM %s | FileCheck --check-prefix=GFX702-CHECK %s
// RUN: %clang -target amdgcn -mcpu=gfx703 -E -dM %s | FileCheck --check-prefix=GFX703-CHECK %s
// RUN: %clang -target amdgcn -mcpu=kabini -E -dM %s | FileCheck --check-prefix=KABINI-CHECK %s
// RUN: %clang -target amdgcn -mcpu=mullins -E -dM %s | FileCheck --check-prefix=MULLINS-CHECK %s
// RUN: %clang -target amdgcn -mcpu=gfx800 -E -dM %s | FileCheck --check-prefix=GFX800-CHECK %s
// RUN: %clang -target amdgcn -mcpu=iceland -E -dM %s | FileCheck --check-prefix=ICELAND-CHECK %s
// RUN: %clang -target amdgcn -mcpu=gfx801 -E -dM %s | FileCheck --check-prefix=GFX801-CHECK %s
// RUN: %clang -target amdgcn -mcpu=carrizo -E -dM %s | FileCheck --check-prefix=CARRIZO-CHECK %s
// RUN: %clang -target amdgcn -mcpu=gfx802 -E -dM %s | FileCheck --check-prefix=GFX802-CHECK %s
// RUN: %clang -target amdgcn -mcpu=tonga -E -dM %s | FileCheck --check-prefix=TONGA-CHECK %s
// RUN: %clang -target amdgcn -mcpu=gfx803 -E -dM %s | FileCheck --check-prefix=GFX803-CHECK %s
// RUN: %clang -target amdgcn -mcpu=fiji -E -dM %s | FileCheck --check-prefix=FIJI-CHECK %s
// RUN: %clang -target amdgcn -mcpu=polaris10 -E -dM %s | FileCheck --check-prefix=POLARIS10-CHECK %s
// RUN: %clang -target amdgcn -mcpu=polaris11 -E -dM %s | FileCheck --check-prefix=POLARIS11-CHECK %s
// RUN: %clang -target amdgcn -mcpu=gfx804 -E -dM %s | FileCheck --check-prefix=GFX804-CHECK %s
// RUN: %clang -target amdgcn -mcpu=gfx810 -E -dM %s | FileCheck --check-prefix=GFX810-CHECK %s
// RUN: %clang -target amdgcn -mcpu=stoney -E -dM %s | FileCheck --check-prefix=STONEY-CHECK %s
// RUN: %clang -target amdgcn -mcpu=gfx900 -E -dM %s | FileCheck --check-prefix=GFX900-CHECK %s
// RUN: %clang -target amdgcn -mcpu=gfx901 -E -dM %s | FileCheck --check-prefix=GFX901-CHECK %s
// RUN: %clang -target amdgcn -mcpu=gfx902 -E -dM %s | FileCheck --check-prefix=GFX902-CHECK %s
// RUN: %clang -target amdgcn -mcpu=gfx903 -E -dM %s | FileCheck --check-prefix=GFX903-CHECK %s

// GFX600-CHECK: #define __gfx600__
// TAHITI-CHECK: #define __tahiti__
// GFX601-CHECK: #define __gfx601__
// PITCAIRN-CHECK: #define __pitcairn__
// VERDE-CHECK: #define __verde__
// OLAND-CHECK: #define __oland__
// HAINAN-CHECK: #define __hainan__
// GFX700-CHECK: #define __gfx700__
// BONAIRE-CHECK: #define __bonaire__
// KAVERI-CHECK: #define __kaveri__
// GFX701-CHECK: #define __gfx701__
// HAWAII-CHECK: #define __hawaii__
// GFX702-CHECK: #define __gfx702__
// GFX703-CHECK: #define __gfx703__
// KABINI-CHECK: #define __kabini__
// MULLINS-CHECK: #define __mullins__
// GFX800-CHECK: #define __gfx800__
// ICELAND-CHECK: #define __iceland__
// GFX801-CHECK: #define __gfx801__
// CARRIZO-CHECK: #define __carrizo__
// GFX802-CHECK: #define __gfx802__
// TONGA-CHECK: #define __tonga__
// GFX803-CHECK: #define __gfx803__
// FIJI-CHECK: #define __fiji__
// POLARIS10-CHECK: #define __polaris10__
// POLARIS11-CHECK: #define __polaris11__
// GFX804-CHECK: #define __gfx804__
// GFX810-CHECK: #define __gfx810__
// STONEY-CHECK: #define __stoney__
// GFX900-CHECK: #define __gfx900__
// GFX901-CHECK: #define __gfx901__
// GFX902-CHECK: #define __gfx902__
// GFX903-CHECK: #define __gfx903__
