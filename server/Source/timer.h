//
//  timer.h
//  porting
//
//  Created by nidhi on 06/07/13.
//  Copyright (c) 2013 abc. All rights reserved.
//

#ifndef porting_timer_h
#define porting_timer_h
#include <stdio.h>
#include <signal.h>
#include <stdint.h>

#include <sys/_structs.h>

#define CLOCK_REALTIME 1

typedef uint64_t clockid_t ;
typedef uint64_t timer_t ;

#ifndef _STRUCT_TIMESPEC
#define _STRUCT_TIMESPEC
struct timespec {
    __darwin_time_t tv_sec;                /* Seconds */
    long   tv_nsec;               /* Nanoseconds */
};
#endif

struct itimerspec {
    struct timespec it_interval;  /* Timer interval */
    struct timespec it_value;     /* Initial expiration */
};

int timer_create(clockid_t clockid, struct sigevent *sevp, timer_t *timerid);
int timer_settime(timer_t timerid, int flags,
                  const struct itimerspec *new_value,
                  struct itimerspec * old_value);
int timer_gettime(timer_t timerid, struct itimerspec *curr_value);
int timer_delete(timer_t timerid);

#endif
