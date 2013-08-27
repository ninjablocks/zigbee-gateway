//
//  timer.c
//  porting
//
//  Created by nidhi on 06/07/13.
//  Copyright (c) 2013 abc. All rights reserved.
//

#include "timer.h"
#include <mach/boolean.h>
#include <mach/mach.h>

#import "timerOSX.h"
#import <Foundation/NSTimer.h>
#import <Foundation/NSMethodSignature.h>
#import <Foundation/NSInvocation.h>


int timer_create(clockid_t clockid, struct sigevent *sevp, timer_t *timerid)
{
    
    int rt = -1;
    timerOSX * timerIDOSX = [[ timerOSX alloc] init ];
    if (timerIDOSX != NULL) {
        *timerid = timerIDOSX ;
         if (timerid != NULL) {
             rt = 0 ;
             printf("\n Timer Created - ID : %lld \n" ,*timerid );
         }
    }
    return rt ;
    
}
int timer_settime(timer_t timerid, int flags,
                  const struct itimerspec *new_value,
                  struct itimerspec * old_value)
{
    int rt = 0;
    timerOSX *timerIDOSX  = (timerOSX *)timerid;
    // old_value is always 0
    uint64_t time = (new_value->it_value.tv_sec * 1000000000 +  new_value->it_value.tv_nsec ) / 1000000000 ;
     printf("\n Timer Enabled - ID : %lld" , timerid );
    rt = [ timerIDOSX create:time];
    return rt ;
}

int timer_gettime(timer_t timerid, struct itimerspec *curr_value)
{
    int rt = 0;
    BOOL timerExpired ;
    
    timerOSX *timerIDOSX  = (timerOSX *)timerid;
    timerExpired = [ timerIDOSX expired];
    
    if(timerExpired == true)
    {
        curr_value->it_value.tv_sec = 0;
        curr_value->it_value.tv_nsec = 0;
        printf("\n Timer Expired - ID : %lld" , timerid );
    }
    else
    {
        curr_value->it_value.tv_sec = 0;
        curr_value->it_value.tv_nsec = 10000000;
        printf("\n Timer NOT Expired - ID : %lld" , timerid );
    }
    
    
    return rt ;
}
int timer_delete(timer_t timerid)
{
    
    int rt = 0;
    timerOSX *timerIDOSX  = (timerOSX *)timerid;
    rt = [ timerIDOSX destroyTimer];
    printf("\n Timer Destroyed - ID : %lld" , timerid );
    return rt ;
}



