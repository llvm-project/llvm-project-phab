/*===-- indexstore/indexstore.h - Index Store C API ----------------- C -*-===*\
|*                                                                            *|
|*                     The LLVM Compiler Infrastructure                       *|
|*                                                                            *|
|* This file is distributed under the University of Illinois Open Source      *|
|* License. See LICENSE.TXT for details.                                      *|
|*                                                                            *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* This header provides a C API for the index store.                          *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef LLVM_CLANG_C_INDEXSTORE_INDEXSTORE_H
#define LLVM_CLANG_C_INDEXSTORE_INDEXSTORE_H

#include <stdint.h>
#include <stddef.h>
#include <ctime>

/**
 * \brief The version constants for the Index Store C API.
 * INDEXSTORE_VERSION_MINOR should increase when there are API additions.
 * INDEXSTORE_VERSION_MAJOR is intended for "major" source/ABI breaking changes.
 */
#define INDEXSTORE_VERSION_MAJOR 0
#define INDEXSTORE_VERSION_MINOR 9

#define INDEXSTORE_VERSION_ENCODE(major, minor) ( \
      ((major) * 10000)                           \
    + ((minor) *     1))

#define INDEXSTORE_VERSION INDEXSTORE_VERSION_ENCODE( \
    INDEXSTORE_VERSION_MAJOR,                         \
    INDEXSTORE_VERSION_MINOR )

#define INDEXSTORE_VERSION_STRINGIZE_(major, minor)   \
    #major"."#minor
#define INDEXSTORE_VERSION_STRINGIZE(major, minor)    \
    INDEXSTORE_VERSION_STRINGIZE_(major, minor)

#define INDEXSTORE_VERSION_STRING INDEXSTORE_VERSION_STRINGIZE( \
    INDEXSTORE_VERSION_MAJOR,                                   \
    INDEXSTORE_VERSION_MINOR)

#ifdef  __cplusplus
# define INDEXSTORE_BEGIN_DECLS  extern "C" {
# define INDEXSTORE_END_DECLS    }
#else
# define INDEXSTORE_BEGIN_DECLS
# define INDEXSTORE_END_DECLS
#endif

INDEXSTORE_BEGIN_DECLS

typedef enum {
  INDEXSTORE_SYMBOL_KIND_UNKNOWN = 0,
  INDEXSTORE_SYMBOL_KIND_MODULE = 1,
  INDEXSTORE_SYMBOL_KIND_NAMESPACE = 2,
  INDEXSTORE_SYMBOL_KIND_NAMESPACEALIAS = 3,
  INDEXSTORE_SYMBOL_KIND_MACRO = 4,
  INDEXSTORE_SYMBOL_KIND_ENUM = 5,
  INDEXSTORE_SYMBOL_KIND_STRUCT = 6,
  INDEXSTORE_SYMBOL_KIND_CLASS = 7,
  INDEXSTORE_SYMBOL_KIND_PROTOCOL = 8,
  INDEXSTORE_SYMBOL_KIND_EXTENSION = 9,
  INDEXSTORE_SYMBOL_KIND_UNION = 10,
  INDEXSTORE_SYMBOL_KIND_TYPEALIAS = 11,
  INDEXSTORE_SYMBOL_KIND_FUNCTION = 12,
  INDEXSTORE_SYMBOL_KIND_VARIABLE = 13,
  INDEXSTORE_SYMBOL_KIND_FIELD = 14,
  INDEXSTORE_SYMBOL_KIND_ENUMCONSTANT = 15,
  INDEXSTORE_SYMBOL_KIND_INSTANCEMETHOD = 16,
  INDEXSTORE_SYMBOL_KIND_CLASSMETHOD = 17,
  INDEXSTORE_SYMBOL_KIND_STATICMETHOD = 18,
  INDEXSTORE_SYMBOL_KIND_INSTANCEPROPERTY = 19,
  INDEXSTORE_SYMBOL_KIND_CLASSPROPERTY = 20,
  INDEXSTORE_SYMBOL_KIND_STATICPROPERTY = 21,
  INDEXSTORE_SYMBOL_KIND_CONSTRUCTOR = 22,
  INDEXSTORE_SYMBOL_KIND_DESTRUCTOR = 23,
  INDEXSTORE_SYMBOL_KIND_CONVERSIONFUNCTION = 24,
  INDEXSTORE_SYMBOL_KIND_PARAMETER = 25,
  INDEXSTORE_SYMBOL_KIND_USING = 26,

  INDEXSTORE_SYMBOL_KIND_COMMENTTAG = 1000,
} indexstore_symbol_kind_t;

typedef enum {
  INDEXSTORE_SYMBOL_SUBKIND_NONE = 0,
  INDEXSTORE_SYMBOL_SUBKIND_CXXCOPYCONSTRUCTOR = 1,
  INDEXSTORE_SYMBOL_SUBKIND_CXXMOVECONSTRUCTOR = 2,
  INDEXSTORE_SYMBOL_SUBKIND_ACCESSORGETTER = 3,
  INDEXSTORE_SYMBOL_SUBKIND_ACCESSORSETTER = 4,
  INDEXSTORE_SYMBOL_SUBKIND_USINGTYPENAME = 5,
  INDEXSTORE_SYMBOL_SUBKIND_USINGVALUE = 6,
} indexstore_symbol_subkind_t;

typedef enum {
  INDEXSTORE_SYMBOL_PROPERTY_GENERIC                          = 1 << 0,
  INDEXSTORE_SYMBOL_PROPERTY_TEMPLATE_PARTIAL_SPECIALIZATION  = 1 << 1,
  INDEXSTORE_SYMBOL_PROPERTY_TEMPLATE_SPECIALIZATION          = 1 << 2,
  INDEXSTORE_SYMBOL_PROPERTY_UNITTEST                         = 1 << 3,
  INDEXSTORE_SYMBOL_PROPERTY_IBANNOTATED                      = 1 << 4,
  INDEXSTORE_SYMBOL_PROPERTY_IBOUTLETCOLLECTION               = 1 << 5,
  INDEXSTORE_SYMBOL_PROPERTY_GKINSPECTABLE                    = 1 << 6,
  INDEXSTORE_SYMBOL_PROPERTY_LOCAL                            = 1 << 7,
} indexstore_symbol_property_t;

typedef enum {
  INDEXSTORE_SYMBOL_LANG_C = 0,
  INDEXSTORE_SYMBOL_LANG_OBJC = 1,
  INDEXSTORE_SYMBOL_LANG_CXX = 2,

  INDEXSTORE_SYMBOL_LANG_SWIFT = 100,
} indexstore_symbol_language_t;

typedef enum {
  INDEXSTORE_SYMBOL_ROLE_DECLARATION = 1 << 0,
  INDEXSTORE_SYMBOL_ROLE_DEFINITION  = 1 << 1,
  INDEXSTORE_SYMBOL_ROLE_REFERENCE   = 1 << 2,
  INDEXSTORE_SYMBOL_ROLE_READ        = 1 << 3,
  INDEXSTORE_SYMBOL_ROLE_WRITE       = 1 << 4,
  INDEXSTORE_SYMBOL_ROLE_CALL        = 1 << 5,
  INDEXSTORE_SYMBOL_ROLE_DYNAMIC     = 1 << 6,
  INDEXSTORE_SYMBOL_ROLE_ADDRESSOF   = 1 << 7,
  INDEXSTORE_SYMBOL_ROLE_IMPLICIT    = 1 << 8,

  // Relation roles.
  INDEXSTORE_SYMBOL_ROLE_REL_CHILDOF     = 1 << 9,
  INDEXSTORE_SYMBOL_ROLE_REL_BASEOF      = 1 << 10,
  INDEXSTORE_SYMBOL_ROLE_REL_OVERRIDEOF  = 1 << 11,
  INDEXSTORE_SYMBOL_ROLE_REL_RECEIVEDBY  = 1 << 12,
  INDEXSTORE_SYMBOL_ROLE_REL_CALLEDBY    = 1 << 13,
  INDEXSTORE_SYMBOL_ROLE_REL_EXTENDEDBY  = 1 << 14,
  INDEXSTORE_SYMBOL_ROLE_REL_ACCESSOROF  = 1 << 15,
  INDEXSTORE_SYMBOL_ROLE_REL_CONTAINEDBY = 1 << 16,
  INDEXSTORE_SYMBOL_ROLE_REL_IBTYPEOF    = 1 << 17,
  INDEXSTORE_SYMBOL_ROLE_REL_SPECIALIZATIONOF = 1 << 18,
} indexstore_symbol_role_t;

INDEXSTORE_END_DECLS

#endif
