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

void socketClientCb( msgData_t *msg ); 
uint8_t SRPC_NewDevice(uint8_t *msg);
static void srpcSendGetDevices( void );

typedef uint8_t (*rpcsProcessMsg_t)(uint8_t *msg);

srpcProcessMsg_t srpcProcessIncoming[] =
{
  NULL,
  SRPC_NewDevice, 
};


int main(int argc, char *argv[])
{
  int ret;
      
  socketClientInit("127.0.0.1:11235", socketClientCb);
  
  //send get devices command
  srpcSendGetDevices();
  
  while(1)
  {
    struct pollfd pollFds[1];
    
		pollFds[0].fd = keyFd;	
	  pollFds[0].events = POLLIN;
	  
	  ret = poll(pollFds, 1, -1);
	
    if (ret > 0) 
    {
      if( ((pollFds[0].revents) & POLLIN) )
      {  
        //Any key ppress will exit
        socketClientClose();
        exit(0);
      }
    }    
  }
   
  socketClientClose();
  
  return 0;
}

//Process the message from SE-Interface
void socketClienCb( msgData_t *msg )
{
  rpcsProcessMsg_t func;

  func = srpcProcessIncoming[(msg->cmdId)];
  if (func)
  {
    (*func)(msg->pData);
  }
  else
  {
    printf("Error: no processing function for CMD 0x%x\n", msg->cmdId); 
  }
      
}

static void srpcSendGetDevices( void )
{ 
  msgData_t srpcCmd;
  
  srpcCmd.cmdId = SRPC_GET_DEVICES;
  srpcCmd.len = 0;
  
  socketClientSendData (&srpcCmd);
    
  return; 
}

/*********************************************************************
 * @fn          SRPC_NewDevice
 *
 * @brief       This function proccesses the NewDevice message from the zbGateway.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
uint8_t SRPC_NewDevice(uint8_t *pMsg)
{    
  epInfo_t epInfo;
  uint8_t devNameStrLen;
  static int i=0;
  uint8_t* pTmpMsg; 
   
  printf("SRPC_NewDevice++\n");  
  
  pTmpMsg = pMsg;
  
  epInfo.nwkAddr = BUILD_UINT16(pTmpMsg[0], pTmpMsg[1]);
  pTmpMsg+=2;

  epInfo.endpoint = *pTmpMsg++;
  
  epInfo.profileID = BUILD_UINT16(pTmpMsg[0], pTmpMsg[1]);
  pTmpMsg+=2;
  
  epInfo.deviceID = BUILD_UINT16(pTmpMsg[0], pTmpMsg[1]);
  pTmpMsg+=2;

  epInfo.version = *pTmpMsg++;
  
  //skip name for now
  devNameStrLen = *pTmpMsg;
  pTmpMsg += devNameStrLen + 1;

  epInfo.status = *pTmpMsg++;
  
  epInfo.IEEEAddr[0] = *pTmpMsg++;
  epInfo.IEEEAddr[1] = *pTmpMsg++;
  epInfo.IEEEAddr[2] = *pTmpMsg++;
  epInfo.IEEEAddr[3] = *pTmpMsg++;
  epInfo.IEEEAddr[4] = *pTmpMsg++;
  epInfo.IEEEAddr[5] = *pTmpMsg++;
  epInfo.IEEEAddr[6] = *pTmpMsg++;
  epInfo.IEEEAddr[7] = *pTmpMsg++;
    
  printf("SRPC_NewDevice[%d]: %x:%x\n", i++, epInfo.nwkAddr, epInfo.endpoint);  
      
  return 0;  
}
