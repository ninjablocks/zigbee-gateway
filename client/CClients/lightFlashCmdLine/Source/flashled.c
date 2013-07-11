 /**************************************************************************************************
  Filename:       flashled.c
  Revised:        $Date: 2012-03-21 17:37:33 -0700 (Wed, 21 Mar 2012) $
  Revision:       $Revision: 246 $

  Description:    This file contains an example client for the zbGateway sever

  Copyright (C) {2012} Texas Instruments Incorporated - http://www.ti.com/


   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

     Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.

     Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the
     distribution.

     Neither the name of Texas Instruments Incorporated nor the names of
     its contributors may be used to endorse or promote products derived
     from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 
**************************************************************************************************/
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <poll.h>

#include "socket_client.h"
#include "interface_srpcserver.h"
#include "hal_defs.h"
 
int keyFd;

#define CONSOLEDEVICE "/dev/console"

void sendLightState(uint16_t addr, uint16_t addrMode, uint16_t ep, uint8_t state);
void socketClientCb( msgData_t *msg );

typedef uint8_t (*srpcProcessMsg_t)(msgData_t *msg);

srpcProcessMsg_t rpcsProcessSeIncoming[] =
{  
};

void keyInit(void)
{ 
  keyFd = open(CONSOLEDEVICE, O_RDONLY | O_NOCTTY | O_NONBLOCK );
  tcflush(keyFd, TCIFLUSH);    
}

int main(int argc, char *argv[])
{
  uint16_t  addr, period, loops;
  uint8_t addrMode, ep, cnt=0;
  uint32 start_with = 0;
      
  if(argc < 6)
  {
    printf("Expected 4 and got %d params Usage: %s <device/group addr> <addr mode> <ep> <periodms> <loops>\n", argc, argv[0] );
    printf("Example - Unicast command of nwk addr 0xb85a ep 0xb every 1s: %s 0xb85a 2 0xb 1000 0\n", argv[0] );
    exit(0);
  }
  else
  {
    uint32_t tmpInt;
    sscanf(argv[1], "0x%x", &tmpInt);
    addr = (uint16_t) tmpInt;
    
    sscanf(argv[2], "%d", &tmpInt);
    addrMode = (uint8_t) tmpInt;
    
    sscanf(argv[3], "0x%x", &tmpInt);
    ep = (uint8_t) tmpInt;
        
    sscanf(argv[4], "%d", &tmpInt);
    period = (uint16_t) tmpInt;
    
    sscanf(argv[5], "%d", &tmpInt);    
    loops = (uint16_t) tmpInt;

    if (argc > 6)
	{
      start_with = atoi(argv[6]);
    }
		
  }
      
  socketClientInit("127.0.0.1:11235", socketClientCb);
  
  if(loops != 0)
    loops+=2;
  
  //loop for ever if loops = 0
  while(loops != 2)
  {    
    printf("Toggling Light %x:%x - %d\n", addr, ep, cnt++ );
    if((loops % 2) == start_with) 
    {
      sendLightState(addr, addrMode, ep, 1);
    }
    else
    {
      sendLightState(addr, addrMode, ep, 0);
    }
          
    usleep(period * 1000);
    
    if(loops > 1)
      loops--;
    else
      loops^=1;
  }
   
  socketClientClose();
  
  return 0;
}

//Process the message from HA-Interface
void socketClientCb( msgData_t *msg )
{
  //for now we are not interested in messages from HA server
}

void sendLightState(uint16_t addr, uint16_t addrMode, uint16_t ep, uint8_t state)
{     
  msgData_t msg;
  uint8_t* pRpcCmd = msg.pData;		 
  		
  msg.cmdId = SRPC_SET_DEV_STATE;
  msg.len = 15;
  //Addr Mode
  *pRpcCmd++ = (afAddrMode_t)addrMode;
  //Addr
  *pRpcCmd++ = addr & 0xFF;
  *pRpcCmd++ = (addr & 0xFF00) >> 8;    
  //index past 8byte addr
  pRpcCmd+=6;
  //Ep
  *pRpcCmd++ = ep;
  //Pad out Pan ID
  pRpcCmd+=2;        
  //State
  *pRpcCmd++ = state;

  socketClientSendData (&msg);
}