//
//  timerOSX.h
//  porting
//
//  Created by abc on 07/07/13.
//  Copyright (c) 2013 abc. All rights reserved.
//


#import <Foundation/Foundation.h>

@interface timerOSX : NSObject
{
    int timerExpired  ;
    NSTimer *t ;
    BOOL isStopped ;
}
-(int)create:(uint64_t)time ;
-(BOOL)expired ;
-(int)destroyTimer ;
- (void)threadProc ;
@end
