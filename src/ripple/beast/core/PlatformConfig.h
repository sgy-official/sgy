

#ifndef BEAST_CONFIG_PLATFORMCONFIG_H_INCLUDED
#define BEAST_CONFIG_PLATFORMCONFIG_H_INCLUDED



#if (defined (_WIN32) || defined (_WIN64))
  #define       BEAST_WIN32 1
  #define       BEAST_WINDOWS 1
#elif defined (BEAST_ANDROID)
  #undef        BEAST_ANDROID
  #define       BEAST_ANDROID 1
#elif defined (LINUX) || defined (__linux__)
  #define     BEAST_LINUX 1
#elif defined (__APPLE_CPP__) || defined(__APPLE_CC__)
  #define Point CarbonDummyPointName 
  #define Component CarbonDummyCompName
  #include <CoreFoundation/CoreFoundation.h> 
  #undef Point
  #undef Component

  #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
    #define     BEAST_IPHONE 1
    #define     BEAST_IOS 1
  #else
    #define     BEAST_MAC 1
  #endif
#elif defined (__FreeBSD__)
  #define BEAST_BSD 1
#else
  #error "Unknown platform!"
#endif

#if BEAST_WINDOWS
  #ifdef _MSC_VER
    #ifdef _WIN64
      #define BEAST_64BIT 1
    #else
      #define BEAST_32BIT 1
    #endif
  #endif

  #ifdef _DEBUG
    #define BEAST_DEBUG 1
  #endif

  #ifdef __MINGW32__
    #define BEAST_MINGW 1
    #ifdef __MINGW64__
      #define BEAST_64BIT 1
    #else
      #define BEAST_32BIT 1
    #endif
  #endif

  
  #define BEAST_LITTLE_ENDIAN 1

  #define BEAST_INTEL 1
#endif

#if BEAST_MAC || BEAST_IOS

  #if defined (DEBUG) || defined (_DEBUG) || ! (defined (NDEBUG) || defined (_NDEBUG))
    #define BEAST_DEBUG 1
  #endif

  #ifdef __LITTLE_ENDIAN__
    #define BEAST_LITTLE_ENDIAN 1
  #else
    #define BEAST_BIG_ENDIAN 1
  #endif
#endif

#if BEAST_MAC

  #if defined (__ppc__) || defined (__ppc64__)
    #define BEAST_PPC 1
  #elif defined (__arm__)
    #define BEAST_ARM 1
  #else
    #define BEAST_INTEL 1
  #endif

  #ifdef __LP64__
    #define BEAST_64BIT 1
  #else
    #define BEAST_32BIT 1
  #endif

  #if MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_4
    #error "Building for OSX 10.3 is no longer supported!"
  #endif

  #ifndef MAC_OS_X_VERSION_10_5
    #error "To build with 10.4 compatibility, use a 10.5 or 10.6 SDK and set the deployment target to 10.4"
  #endif

#endif

#if BEAST_LINUX || BEAST_ANDROID || BEAST_BSD

  #ifdef _DEBUG
    #define BEAST_DEBUG 1
  #endif

  #if defined (__LITTLE_ENDIAN__) || ! defined (BEAST_BIG_ENDIAN)
    #define BEAST_LITTLE_ENDIAN 1
    #undef BEAST_BIG_ENDIAN
  #else
    #undef BEAST_LITTLE_ENDIAN
    #define BEAST_BIG_ENDIAN 1
  #endif

  #if defined (__LP64__) || defined (_LP64)
    #define BEAST_64BIT 1
  #else
    #define BEAST_32BIT 1
  #endif

  #if __MMX__ || __SSE__ || __amd64__
    #ifdef __arm__
      #define BEAST_ARM 1
    #else
      #define BEAST_INTEL 1
    #endif
  #endif
#endif


#ifdef __clang__
 #define BEAST_CLANG 1
#elif defined (__GNUC__)
  #define BEAST_GCC 1
#elif defined (_MSC_VER)
  #define BEAST_MSVC 1

  #if _MSC_VER < 1500
    #define BEAST_VC8_OR_EARLIER 1

    #if _MSC_VER < 1400
      #define BEAST_VC7_OR_EARLIER 1

      #if _MSC_VER < 1300
        #warning "MSVC 6.0 is no longer supported!"
      #endif
    #endif
  #endif

  #if BEAST_64BIT || ! BEAST_VC7_OR_EARLIER
    #define BEAST_USE_INTRINSICS 1
  #endif
#else
  #error unknown compiler
#endif


#define BEAST_PP_STR2_(x) #x
#define BEAST_PP_STR1_(x) BEAST_PP_STR2_(x)
#define BEAST_FILEANDLINE_ __FILE__ "(" BEAST_PP_STR1_(__LINE__) "): warning:"

#endif








