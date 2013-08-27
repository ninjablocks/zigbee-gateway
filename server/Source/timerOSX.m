//
//  timerOSX.m
//  porting
//
//  Created by abc on 07/07/13.
//  Copyright (c) 2013 abc. All rights reserved.
//

#import "timerOSX.h"
#import  <Foundation/NSThread.h>

@implementation timerOSX

- (id) init
{
    //initialize the superclass and assign it to self
    if (self = [super init]) {
        timerExpired = false ;
    }
    isStopped = NO ;
   
    return self;
}

- (void)threadProc
{

    NSRunLoop *runloop = [NSRunLoop currentRunLoop];
    
    t = [NSTimer timerWithTimeInterval: 3
                                target: self
                              selector:@selector(onTick:)
                              userInfo: nil repeats:NO];
    [runloop addTimer:t forMode:NSDefaultRunLoopMode];
    while (!isStopped)
    {
        {
            [runloop runMode:NSDefaultRunLoopMode
                  beforeDate:[NSDate distantFuture]];
        }
    }

}


-(int)create:(uint64_t)time {
    
    NSThread * wthread = [[NSThread alloc] initWithTarget:self selector:@selector(threadProc) object:nil];
    [wthread start];
  
    return 0;   

}


-(void)onTick:(NSTimer *)timer {
    printf("\nTimer Fired - ID %lld" , self);
    timerExpired = true ;
     isStopped = TRUE ;
}

-(BOOL)expired
{
    return timerExpired ;
}

-(int)destroyTimer
{
   
    if( t != nil )
        [t invalidate];
    t = nil ;
    return 0 ;
}
@end
