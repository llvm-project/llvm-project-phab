// Check that appropriate macros are defined for every supported AMDGPU
// "-target" and "-mcpu".

//
// R600.
//

// RUN: %clang -target r600 -mcpu=r600 -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-R600-CHECK --check-prefix=R600-CHECK %s
// RUN: %clang -target r600 -mcpu=rv630 -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-R600-CHECK --check-prefix=R600-CHECK %s
// RUN: %clang -target r600 -mcpu=rv635 -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-R600-CHECK --check-prefix=R600-CHECK %s
// RUN: %clang -target r600 -mcpu=rv610 -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-R600-CHECK --check-prefix=RS880-CHECK %s
// RUN: %clang -target r600 -mcpu=rv620 -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-R600-CHECK --check-prefix=RS880-CHECK %s
// RUN: %clang -target r600 -mcpu=rs780 -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-R600-CHECK --check-prefix=RS880-CHECK %s
// RUN: %clang -target r600 -mcpu=rs880 -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-R600-CHECK --check-prefix=RS880-CHECK %s
// RUN: %clang -target r600 -mcpu=rv670 -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-R600-CHECK --check-prefix=RV670-CHECK %s
// RUN: %clang -target r600 -mcpu=rv710 -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-R600-CHECK --check-prefix=RV710-CHECK %s
// RUN: %clang -target r600 -mcpu=rv730 -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-R600-CHECK --check-prefix=RV730-CHECK %s
// RUN: %clang -target r600 -mcpu=rv740 -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-R600-CHECK --check-prefix=RV770-CHECK %s
// RUN: %clang -target r600 -mcpu=rv770 -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-R600-CHECK --check-prefix=RV770-CHECK %s
// RUN: %clang -target r600 -mcpu=palm -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-R600-CHECK --check-prefix=CEDAR-CHECK %s
// RUN: %clang -target r600 -mcpu=cedar -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-R600-CHECK --check-prefix=CEDAR-CHECK %s
// RUN: %clang -target r600 -mcpu=sumo -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-R600-CHECK --check-prefix=SUMO-CHECK %s
// RUN: %clang -target r600 -mcpu=sumo2 -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-R600-CHECK --check-prefix=SUMO-CHECK %s
// RUN: %clang -target r600 -mcpu=redwood -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-R600-CHECK --check-prefix=REDWOOD-CHECK %s
// RUN: %clang -target r600 -mcpu=juniper -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-R600-CHECK --check-prefix=JUNIPER-CHECK %s
// RUN: %clang -target r600 -mcpu=juniper -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-R600-CHECK --check-prefix=JUNIPER-CHECK %s
// RUN: %clang -target r600 -mcpu=hemlock -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-R600-CHECK --check-prefix=CYPRESS-CHECK %s
// RUN: %clang -target r600 -mcpu=cypress -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-R600-CHECK --check-prefix=CYPRESS-CHECK %s
// RUN: %clang -target r600 -mcpu=barts -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-R600-CHECK --check-prefix=BARTS-CHECK %s
// RUN: %clang -target r600 -mcpu=turks -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-R600-CHECK --check-prefix=TURKS-CHECK %s
// RUN: %clang -target r600 -mcpu=caicos -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-R600-CHECK --check-prefix=CAICOS-CHECK %s
// RUN: %clang -target r600 -mcpu=cayman -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-R600-CHECK --check-prefix=CAYMAN-CHECK %s
// RUN: %clang -target r600 -mcpu=aruba -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-R600-CHECK --check-prefix=CAYMAN-CHECK %s

//
// GCN.
//

// RUN: %clang -target amdgcn -mcpu=gfx600 -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-GCN-CHECK --check-prefix=GFX600-CHECK %s
// RUN: %clang -target amdgcn -mcpu=tahiti -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-GCN-CHECK --check-prefix=TAHITI-CHECK %s
// RUN: %clang -target amdgcn -mcpu=gfx601 -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-GCN-CHECK --check-prefix=GFX601-CHECK %s
// RUN: %clang -target amdgcn -mcpu=pitcairn -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-GCN-CHECK --check-prefix=PITCAIRN-CHECK %s
// RUN: %clang -target amdgcn -mcpu=verde -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-GCN-CHECK --check-prefix=VERDE-CHECK %s
// RUN: %clang -target amdgcn -mcpu=oland -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-GCN-CHECK --check-prefix=OLAND-CHECK %s
// RUN: %clang -target amdgcn -mcpu=hainan -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-GCN-CHECK --check-prefix=HAINAN-CHECK %s
// RUN: %clang -target amdgcn -mcpu=gfx700 -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-GCN-CHECK --check-prefix=GFX700-CHECK %s
// RUN: %clang -target amdgcn -mcpu=bonaire -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-GCN-CHECK --check-prefix=BONAIRE-CHECK %s
// RUN: %clang -target amdgcn -mcpu=kaveri -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-GCN-CHECK --check-prefix=KAVERI-CHECK %s
// RUN: %clang -target amdgcn -mcpu=gfx701 -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-GCN-CHECK --check-prefix=GFX701-CHECK %s
// RUN: %clang -target amdgcn -mcpu=hawaii -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-GCN-CHECK --check-prefix=HAWAII-CHECK %s
// RUN: %clang -target amdgcn -mcpu=gfx702 -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-GCN-CHECK --check-prefix=GFX702-CHECK %s
// RUN: %clang -target amdgcn -mcpu=gfx703 -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-GCN-CHECK --check-prefix=GFX703-CHECK %s
// RUN: %clang -target amdgcn -mcpu=kabini -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-GCN-CHECK --check-prefix=KABINI-CHECK %s
// RUN: %clang -target amdgcn -mcpu=mullins -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-GCN-CHECK --check-prefix=MULLINS-CHECK %s
// RUN: %clang -target amdgcn -mcpu=gfx800 -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-GCN-CHECK --check-prefix=GFX800-CHECK %s
// RUN: %clang -target amdgcn -mcpu=iceland -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-GCN-CHECK --check-prefix=ICELAND-CHECK %s
// RUN: %clang -target amdgcn -mcpu=gfx801 -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-GCN-CHECK --check-prefix=GFX801-CHECK %s
// RUN: %clang -target amdgcn -mcpu=carrizo -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-GCN-CHECK --check-prefix=CARRIZO-CHECK %s
// RUN: %clang -target amdgcn -mcpu=gfx802 -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-GCN-CHECK --check-prefix=GFX802-CHECK %s
// RUN: %clang -target amdgcn -mcpu=tonga -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-GCN-CHECK --check-prefix=TONGA-CHECK %s
// RUN: %clang -target amdgcn -mcpu=gfx803 -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-GCN-CHECK --check-prefix=GFX803-CHECK %s
// RUN: %clang -target amdgcn -mcpu=fiji -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-GCN-CHECK --check-prefix=FIJI-CHECK %s
// RUN: %clang -target amdgcn -mcpu=polaris10 -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-GCN-CHECK --check-prefix=POLARIS10-CHECK %s
// RUN: %clang -target amdgcn -mcpu=polaris11 -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-GCN-CHECK --check-prefix=POLARIS11-CHECK %s
// RUN: %clang -target amdgcn -mcpu=gfx804 -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-GCN-CHECK --check-prefix=GFX804-CHECK %s
// RUN: %clang -target amdgcn -mcpu=gfx810 -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-GCN-CHECK --check-prefix=GFX810-CHECK %s
// RUN: %clang -target amdgcn -mcpu=stoney -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-GCN-CHECK --check-prefix=STONEY-CHECK %s
// RUN: %clang -target amdgcn -mcpu=gfx900 -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-GCN-CHECK --check-prefix=GFX900-CHECK %s
// RUN: %clang -target amdgcn -mcpu=gfx901 -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-GCN-CHECK --check-prefix=GFX901-CHECK %s
// RUN: %clang -target amdgcn -mcpu=gfx902 -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-GCN-CHECK --check-prefix=GFX902-CHECK %s
// RUN: %clang -target amdgcn -mcpu=gfx903 -E -dM %s | FileCheck --check-prefix=AMDGPU-CHECK --check-prefix=ARCH-GCN-CHECK --check-prefix=GFX903-CHECK %s

// AMDGPU-CHECK-DAG: #define __AMD__ 1
// AMDGPU-CHECK-DAG: #define __AMDGPU__ 1

//
// R600.
//

// ARCH-R600-CHECK-DAG: #define __R600__ 1
// R600-CHECK: #define __r600 1
// R600-CHECK: #define __r600__ 1
// RS880-CHECK: #define __rs880 1
// RS880-CHECK: #define __rs880__ 1
// RV670-CHECK: #define __rv670 1
// RV670-CHECK: #define __rv670__ 1
// RV710-CHECK: #define __rv710 1
// RV710-CHECK: #define __rv710__ 1
// RV730-CHECK: #define __rv730 1
// RV730-CHECK: #define __rv730__ 1
// RV770-CHECK: #define __rv770 1
// RV770-CHECK: #define __rv770__ 1
// CEDAR-CHECK: #define __cedar 1
// CEDAR-CHECK: #define __cedar__ 1
// REDWOOD-CHECK: #define __redwood 1
// REDWOOD-CHECK: #define __redwood__ 1
// SUMO-CHECK: #define __sumo 1
// SUMO-CHECK: #define __sumo__ 1
// JUNIPER-CHECK: #define __juniper 1
// JUNIPER-CHECK: #define __juniper__ 1
// CYPRESS-CHECK: #define __cypress 1
// CYPRESS-CHECK: #define __cypress__ 1
// BARTS-CHECK: #define __barts 1
// BARTS-CHECK: #define __barts__ 1
// TURKS-CHECK: #define __turks 1
// TURKS-CHECK: #define __turks__ 1
// CAICOS-CHECK: #define __caicos 1
// CAICOS-CHECK: #define __caicos__ 1
// CAYMAN-CHECK: #define __cayman 1
// CAYMAN-CHECK: #define __cayman__ 1

//
// GCN.
//

// ARCH-GCN-CHECK-DAG: #define __AMDGCN__ 1
// GFX600-CHECK: #define __gfx600 1
// GFX600-CHECK: #define __gfx600__ 1
// TAHITI-CHECK: #define __tahiti 1
// TAHITI-CHECK: #define __tahiti__ 1
// GFX601-CHECK: #define __gfx601 1
// GFX601-CHECK: #define __gfx601__ 1
// PITCAIRN-CHECK: #define __pitcairn 1
// PITCAIRN-CHECK: #define __pitcairn__ 1
// VERDE-CHECK: #define __verde 1
// VERDE-CHECK: #define __verde__ 1
// OLAND-CHECK: #define __oland 1
// OLAND-CHECK: #define __oland__ 1
// HAINAN-CHECK: #define __hainan 1
// HAINAN-CHECK: #define __hainan__ 1
// GFX700-CHECK: #define __gfx700 1
// GFX700-CHECK: #define __gfx700__ 1
// BONAIRE-CHECK: #define __bonaire 1
// BONAIRE-CHECK: #define __bonaire__ 1
// KAVERI-CHECK: #define __kaveri 1
// KAVERI-CHECK: #define __kaveri__ 1
// GFX701-CHECK: #define __gfx701 1
// GFX701-CHECK: #define __gfx701__ 1
// HAWAII-CHECK: #define __hawaii 1
// HAWAII-CHECK: #define __hawaii__ 1
// GFX702-CHECK: #define __gfx702 1
// GFX702-CHECK: #define __gfx702__ 1
// GFX703-CHECK: #define __gfx703 1
// GFX703-CHECK: #define __gfx703__ 1
// KABINI-CHECK: #define __kabini 1
// KABINI-CHECK: #define __kabini__ 1
// MULLINS-CHECK: #define __mullins 1
// MULLINS-CHECK: #define __mullins__ 1
// GFX800-CHECK: #define __gfx800 1
// GFX800-CHECK: #define __gfx800__ 1
// ICELAND-CHECK: #define __iceland 1
// ICELAND-CHECK: #define __iceland__ 1
// GFX801-CHECK: #define __gfx801 1
// GFX801-CHECK: #define __gfx801__ 1
// CARRIZO-CHECK: #define __carrizo 1
// CARRIZO-CHECK: #define __carrizo__ 1
// GFX802-CHECK: #define __gfx802 1
// GFX802-CHECK: #define __gfx802__ 1
// TONGA-CHECK: #define __tonga 1
// TONGA-CHECK: #define __tonga__ 1
// GFX803-CHECK: #define __gfx803 1
// GFX803-CHECK: #define __gfx803__ 1
// FIJI-CHECK: #define __fiji 1
// FIJI-CHECK: #define __fiji__ 1
// POLARIS10-CHECK: #define __polaris10 1
// POLARIS10-CHECK: #define __polaris10__ 1
// POLARIS11-CHECK: #define __polaris11 1
// POLARIS11-CHECK: #define __polaris11__ 1
// GFX804-CHECK: #define __gfx804 1
// GFX804-CHECK: #define __gfx804__ 1
// GFX810-CHECK: #define __gfx810 1
// GFX810-CHECK: #define __gfx810__ 1
// STONEY-CHECK: #define __stoney 1
// STONEY-CHECK: #define __stoney__ 1
// GFX900-CHECK: #define __gfx900 1
// GFX900-CHECK: #define __gfx900__ 1
// GFX901-CHECK: #define __gfx901 1
// GFX901-CHECK: #define __gfx901__ 1
// GFX902-CHECK: #define __gfx902 1
// GFX902-CHECK: #define __gfx902__ 1
// GFX903-CHECK: #define __gfx903 1
// GFX903-CHECK: #define __gfx903__ 1
