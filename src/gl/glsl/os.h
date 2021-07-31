    #ifndef _OS_H
    #define _OS_H

    #if defined(__linux__) || defined(__linux)
    #  define OS_LINUX
    #endif

    #if !defined(SAG_COM) && (!defined(WINAPI_FAMILY) || WINAPI_FAMILY==WINAPI_FAMILY_DESKTOP_APP) && (defined(WIN64) || defined(_WIN64) || defined(__WIN64__))
    #  define OS_WIN32
    #  define OS_WIN64
    #elif !defined(SAG_COM) && (defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__))
    #  if defined(WINCE) || defined(_WIN32_WCE)
    #    define OS_WINCE
    #  elif defined(WINAPI_FAMILY)
    #    if defined(WINAPI_FAMILY_PHONE_APP) && WINAPI_FAMILY==WINAPI_FAMILY_PHONE_APP
    #      define OS_WINPHONE
    #      define OS_WINRT
    #    elif WINAPI_FAMILY==WINAPI_FAMILY_APP
    #      define OS_WINRT
    #    else
    #      define OS_WIN32
    #    endif
    #  else
    #    define OS_WIN32
    #  endif
    #endif

    #if defined(OS_WIN32) || defined(OS_WIN64) || defined(OS_WINCE) || defined(OS_WINRT)
    #  define OS_WIN
    #endif

#endif