/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef NEONCOMMON_INCLUDE_NEON_COMMON_SHAREDLIB_H
#define NEONCOMMON_INCLUDE_NEON_COMMON_SHAREDLIB_H

#ifdef NEON_STATIC
    #define NEON_PUBLIC
    #define NEON_LOCAL
#else
    #if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
        #ifdef NEON_DO_EXPORT
            #ifdef __GNUC__
                #define NEON_PUBLIC __attribute__ ((dllexport))
            #else
                #define NEON_PUBLIC __declspec(dllexport)
            #endif
        #else
            #ifdef __GNUC__
                #define NEON_PUBLIC __attribute__ ((dllimport))
            #else
                #define NEON_PUBLIC __declspec(dllimport)
            #endif
        #endif

        #define NEON_LOCAL
    #else
        #if __GNUC__ >= 4
            #define NEON_PUBLIC __attribute__ ((visibility ("default")))
            #define NEON_LOCAL  __attribute__ ((visibility ("hidden")))
        #else
            #define NEON_PUBLIC
            #define NEON_LOCAL
        #endif
    #endif
#endif

#endif //NEONCOMMON_INCLUDE_NEON_COMMON_SHAREDLIB_H
