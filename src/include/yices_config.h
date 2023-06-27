#pragma once

#include "yices_build.h"

#ifdef __cplusplus
#ifdef YICES_CPP
#define YICES_EXTERN extern
#define YICES_DEFINE
#else
#define YICES_EXTERN extern "C"
#define YICES_DEFINE extern "C"
#endif
#else
#ifdef YICES_CPP
#error "This is pure c++ build which cannot be used from c"
#endif
#define YICES_EXTERN extern
#define YICES_DEFINE
#endif

#if (defined(YICES_STATIC) || !defined(_WIN32)) && !defined(NOYICES_DLL)
#define NOYICES_DLL
#endif

#ifdef NOYICES_DLL
#define YICES_EXPORTED YICES_DEFINE
#define YICES_IMPORTED YICES_EXTERN
#else
#ifdef _WIN32
#ifdef libyices_EXPORTS
#define YICES_IMPORTED YICES_EXTERN __declspec(dllexport)
#else
#define YICES_IMPORTED YICES_EXTERN __declspec(dllimport)
#endif
#define YICES_EXPORTED YICES_DEFINE __declspec(dllexport)
#else
#define YICES_EXPORTED YICES_DEFINE __attribute__((visibility("default")))
#define YICES_IMPORTED YICES_EXTERN __attribute__((visibility("default")))
#endif
#endif

#if defined(PER_THREAD_STATE) && !defined(THREAD_SAFE)
#define THREAD_SAFE
#endif

#ifdef THREAD_SAFE
#ifdef __cplusplus
#define YICES_THREAD_LOCAL thread_local
#else
#ifdef _MSC_VER
#define YICES_THREAD_LOCAL __declspec(thread)
#else
#define YICES_THREAD_LOCAL __thread
#endif
#endif
#else
#define YICES_THREAD_LOCAL
#endif

#ifdef PER_THREAD_STATE
#define YICES_PTS_LOCAL YICES_THREAD_LOCAL
#else
#define YICES_PTS_LOCAL
#endif



#if defined(_WIN32) && !defined(_WIN32_WINNT)
// Use the oldest version of the Windows API.
#define _WIN32_WINNT 0x0500
#endif

#ifdef _MSC_VER
#define ATTRIBUTE_ALWAYS_INLINE
#define ATTRIBUTE_FORMAT(...)
#define ATTRIBUTE_MALLOC
#define ATTRIBUTE_NORETURN __declspec(noreturn)
#define ATTRIBUTE_UNUSED
#else
#define ATTRIBUTE_ALWAYS_INLINE __attribute__((always_inline))
#define ATTRIBUTE_FORMAT(...) __attribute__((format(__VA_ARGS__)))
#define ATTRIBUTE_MALLOC __attribute__((malloc))
#define ATTRIBUTE_NORETURN __attribute__((noreturn))
#define ATTRIBUTE_UNUSED __attribute__((unused))
#endif

/*
 * To enable general trace, set TRACE to 1
 * To enable the debugging code, set DEBUG to 1
 * To dump the initial tableau, set DUMP to 1
 *
 * To trace simplifications and tableau initialization set TRACE_INIT to 1
 * To trace the theory propagation set TRACE_PROPAGATION to 1 (in
 * To trace the branch&bound algorithm set TRACE_BB to 1
 * To get a summary of general solution + bounds: TRACE_INTFEAS to 1
 */

#ifndef YICES_EF_VERBOSE
#define YICES_EF_VERBOSE 0
#endif
#ifndef YICES_EM_VERBOSE
#define YICES_EM_VERBOSE 0
#endif
#ifndef YICES_TRACE
#define YICES_TRACE 0
#endif
#ifndef YICES_TRACE_LIGHT
#define YICES_TRACE_LIGHT 0
#endif
#ifndef YICES_TRACE_FCHECK
#define YICES_TRACE_FCHECK 0
#endif
#ifndef YICES_TRACE_SUBST
#define YICES_TRACE_SUBST 0
#endif
#ifndef YICES_TRACE_INIT
#define YICES_TRACE_INIT 0
#endif
#ifndef YICES_TRACE_PROPAGATION
#define YICES_TRACE_PROPAGATION 0
#endif
#ifndef YICES_TRACE_BB
#define YICES_TRACE_BB 0
#endif
#ifndef YICES_TRACE_INTFEAS
#define YICES_TRACE_INTFEAS 0
#endif
#ifndef YICES_TRACE_DL
#define YICES_TRACE_DL 0
#endif
#ifndef YICES_TRACE_SYM_BREAKING
#define YICES_TRACE_SYM_BREAKING 0
#endif
#ifndef YICES_DUMP
#define YICES_DUMP 0
#endif
#ifndef YICES_DEBUG
#define YICES_DEBUG 0
#endif
#ifndef YICES_DATA
#define YICES_DATA 0
#endif
