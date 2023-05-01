/*
This file is part of "Ostinato"

These macros are copied from BSD sys/time.h
*/

#ifndef _TIME_VAL_OPS
#define _TIME_VAL_OPS

/* Operations on timeval - for platforms where some are not already defined*/
#if defined(Q_OS_WIN32)

#ifndef timerclear
#define timerclear(tvp) ((tvp)->tv_sec = (tvp)->tv_usec = 0)
#endif

#ifndef timerisset
#define timerisset(tvp) ((tvp)->tv_sec || (tvp)->tv_usec)
#endif

#ifndef timercmp
#define timercmp(tvp, uvp, cmp)         \
    (((tvp)->tv_sec == (uvp)->tv_sec) ? \
g((tvp)->tv_usec cmp (uvp)->tv_usec) :  \
g((tvp)->tv_sec cmp (uvp)->tv_sec))
#endif

#ifndef timeradd
#define timeradd(tvp, uvp, vvp)                             \
    do {                                                    \
        (vvp)->tv_sec = (tvp)->tv_sec + (uvp)->tv_sec;      \
        (vvp)->tv_usec = (tvp)->tv_usec + (uvp)->tv_usec;   \
        if ((vvp)->tv_usec >= 1000000) {                    \
            (vvp)->tv_sec++;                                \
            (vvp)->tv_usec -= 1000000;                      \
        }                                                   \
    } while (0)
#endif

#ifndef timersub
#define timersub(tvp, uvp, vvp)                             \
    do {                                                    \
        (vvp)->tv_sec = (tvp)->tv_sec - (uvp)->tv_sec;      \
        (vvp)->tv_usec = (tvp)->tv_usec - (uvp)->tv_usec;   \
        if ((vvp)->tv_usec < 0) {                           \
            (vvp)->tv_sec--;                                \
            (vvp)->tv_usec += 1000000;                      \
        }                                                   \
    } while (0)
#endif

#endif /* Q_OS_WIN32 */

#endif
