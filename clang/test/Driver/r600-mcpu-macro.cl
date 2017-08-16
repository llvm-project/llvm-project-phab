// RUN: %clang -target r600 -mcpu=r600 -E -dM  %s | FileCheck --check-prefix=R600-CHECK %s
// RUN: %clang -target r600 -mcpu=rv630 -E -dM  %s | FileCheck --check-prefix=R600-CHECK %s
// RUN: %clang -target r600 -mcpu=rv635 -E -dM  %s | FileCheck --check-prefix=R600-CHECK %s
// RUN: %clang -target r600 -mcpu=rv610 -E -dM  %s | FileCheck --check-prefix=RS880-CHECK %s
// RUN: %clang -target r600 -mcpu=rv620 -E -dM  %s | FileCheck --check-prefix=RS880-CHECK %s
// RUN: %clang -target r600 -mcpu=rs780 -E -dM  %s | FileCheck --check-prefix=RS880-CHECK %s
// RUN: %clang -target r600 -mcpu=rs880 -E -dM  %s | FileCheck --check-prefix=RS880-CHECK %s
// RUN: %clang -target r600 -mcpu=rv670 -E -dM  %s | FileCheck --check-prefix=RV670-CHECK %s
// RUN: %clang -target r600 -mcpu=rv710 -E -dM  %s | FileCheck --check-prefix=RV710-CHECK %s
// RUN: %clang -target r600 -mcpu=rv730 -E -dM  %s | FileCheck --check-prefix=RV730-CHECK %s
// RUN: %clang -target r600 -mcpu=rv740 -E -dM  %s | FileCheck --check-prefix=RV770-CHECK %s
// RUN: %clang -target r600 -mcpu=rv770 -E -dM  %s | FileCheck --check-prefix=RV770-CHECK %s
// RUN: %clang -target r600 -mcpu=palm -E -dM  %s | FileCheck --check-prefix=CEDAR-CHECK %s
// RUN: %clang -target r600 -mcpu=cedar -E -dM  %s | FileCheck --check-prefix=CEDAR-CHECK %s
// RUN: %clang -target r600 -mcpu=sumo -E -dM  %s | FileCheck --check-prefix=SUMO-CHECK %s
// RUN: %clang -target r600 -mcpu=sumo2 -E -dM  %s | FileCheck --check-prefix=SUMO-CHECK %s
// RUN: %clang -target r600 -mcpu=redwood -E -dM  %s | FileCheck --check-prefix=REDWOOD-CHECK %s
// RUN: %clang -target r600 -mcpu=juniper -E -dM  %s | FileCheck --check-prefix=JUNIPER-CHECK %s
// RUN: %clang -target r600 -mcpu=juniper -E -dM  %s | FileCheck --check-prefix=JUNIPER-CHECK %s
// RUN: %clang -target r600 -mcpu=hemlock -E -dM  %s | FileCheck --check-prefix=CYPRESS-CHECK %s
// RUN: %clang -target r600 -mcpu=cypress -E -dM  %s | FileCheck --check-prefix=CYPRESS-CHECK %s
// RUN: %clang -target r600 -mcpu=barts -E -dM  %s | FileCheck --check-prefix=BARTS-CHECK %s
// RUN: %clang -target r600 -mcpu=turks -E -dM  %s | FileCheck --check-prefix=TURKS-CHECK %s
// RUN: %clang -target r600 -mcpu=caicos -E -dM  %s | FileCheck --check-prefix=CAICOS-CHECK %s
// RUN: %clang -target r600 -mcpu=cayman -E -dM  %s | FileCheck --check-prefix=CAYMAN-CHECK %s
// RUN: %clang -target r600 -mcpu=aruba -E -dM  %s | FileCheck --check-prefix=CAYMAN-CHECK %s

// R600-CHECK: #define __r600__
// RS880-CHECK: #define __rs880__
// RV670-CHECK: #define __rv670__
// RV710-CHECK: #define __rv710__
// RV730-CHECK: #define __rv730__
// RV770-CHECK: #define __rv770__
// CEDAR-CHECK: #define __cedar__
// REDWOOD-CHECK: #define __redwood__
// SUMO-CHECK: #define __sumo__
// JUNIPER-CHECK: #define __juniper__
// CYPRESS-CHECK: #define __cypress__
// BARTS-CHECK: #define __barts__
// TURKS-CHECK: #define __turks__
// CAICOS-CHECK: #define __caicos__
// CAYMAN-CHECK: #define __cayman__
