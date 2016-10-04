#pragma once

#ifdef _MSC_VER
# define DECODE_DECL_EXPORT __declspec(dllexport)
# define DECODE_DECL_IMPORT __declspec(dllimport)
#else
# define DECODE_DECL_EXPORT
# define DECODE_DECL_IMPORT
#endif

#ifdef BUILDING_DECODE
# define DECODE_EXPORT DECODE_DECL_EXPORT
#else
# define DECODE_EXPORT DECODE_DECL_IMPORT
#endif
