

#ifndef BEAST_CONFIG_STANDARDCONFIG_H_INCLUDED
#define BEAST_CONFIG_STANDARDCONFIG_H_INCLUDED

#if BEAST_MSVC
#pragma warning (push)
#pragma warning (disable: 4514 4245 4100)
#endif

#if BEAST_USE_INTRINSICS
#include <intrin.h>
#endif

#if BEAST_MAC || BEAST_IOS
#include <libkern/OSAtomic.h>
#endif

#if BEAST_LINUX
#include <signal.h>
# if __INTEL_COMPILER
#  if __ia64__
#include <ia64intrin.h>
#  else
#include <ia32intrin.h>
#  endif
# endif
#endif

#if BEAST_MSVC && BEAST_DEBUG
#include <crtdbg.h>
#include <stdlib.h>
#include <malloc.h>
#endif

#if BEAST_MSVC
#pragma warning (pop)
#endif

#if BEAST_ANDROID
 #include <sys/atomics.h>
 #include <byteswap.h>
#endif

#undef check
#undef TYPE_BOOL
#undef max
#undef min

#endif








