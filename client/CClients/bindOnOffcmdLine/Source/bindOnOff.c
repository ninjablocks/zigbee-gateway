 /**************************************************************************************************
  Filename:       bindOnOff.c
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
int printDevHeader = FALSE;
int printTempHeader = TRUE;

#define CONSOLEDEVICE "/dev/console"

void socketClientCb( msgData_t *msg );
void sendBindOnOff(uint16_t srcNwkAddr, uint8_t srcEp, uint8_t srcIeee[8], uint8_t dstEp, uint8_t dstIeee[8], uint16_t clusterId );
uint8_t SRPC_OnOffCb(uint8_t *pMsg);

typedef uint8_t (*srpcProcessMsg_t)(uint8_t *msg);

srpcProcessMsg_t srpcProcessIncoming[] =
{  
  NULL, //Reserved
  NULL, //SRPC_NEW_DEVICE     0x0001
  NULL, //SRPC_DEV_ANNCE		      0x0002
  NULL, //SRPC_SIMPLE_DESC	      0x0003
  NULL, //SRPC_TEMP_READING       0x0004
  NULL, //SRPC_READ_POWER_RSP               0x0005
  NULL, //SRPC_PING               0x0006
  NULL, //SRPC_GET_DEV_STATE_RSP  0x0007	
  NULL, //SRPC_GET_DEV_LEVEL_RSP  0x0008
  NULL, //SRPC_GET_DEV_HUE_RSP    0x0009
  NULL, //SRPC_GET_DEV_SAT_RSP    0x000a
  NULL, //SRPC_ADD_GROUP_RSP      0x000b
  NULL, //SRPC_GET_GROUP_RSP      0x000c
  NULL, //SRPC_ADD_SCENE_RSP      0x000d
  NULL, //SRPC_GET_SCENE_RSP      0x000e
  NULL, //SRPC_HUMID_READING      0x000f
  NULL, //SRPC_ZONESTATE_CHANGE   0x0010
  NULL, //SRPC_SBL_RSP     0x0011
  NULL, //SRPC_SBL_PROGRESS 0x0012
  NULL, //SRPC_CERT_INSTALL_RESULT_IND      0x0013
  NULL, //SRPC_KEY_ESTABLISHMENT_STATE_IND  0x0014
  NULL, //SRPC_DISPLAY_MESSAGE_IND          0x0015
  NULL, //SRPC_PUBLISH_PRICE_IND            0x0016
  NULL, //SRPC_DEVICE_REMOVED               0x0017
  SRPC_OnOffCb, //SRPC_ONOFF_CMD                    0x0018
};

void keyInit(void)
{ 
  keyFd = open(CONSOLEDEVICE, O_RDONLY | O_NOCTTY | O_NONBLOCK );
  tcflush(keyFd, TCIFLUSH);    
}

int main(int argc, char *argv[])
{
  uint16_t srcAddr, dstAddr;
  uint8_t srcEp, dstEp, srcIeee[8], dstIeee[8], i;
  uint32 ret;
      
  if(argc < 6)
  {
    printf("Expected 4 and got %d params Usage: %s <source nwk Addr> <src ep> <source ieee> <dst ep> <dst ieee> \n", argc, argv[0] );
    printf("Example - bind cluster on/off cluster of device addr:0x0000, ep:0x0b IEEE:00:12:4B:00:01:23:61:2C\n");
    printf("Example - to device ep:0x01 IEEE:00:12:4B:00:01:23:61:2C: %s 0x0000 0x0b 00:12:4B:00:01:23:61:2C 0x01 00:12:4B:00:01:23:61:2C \n", argv[0] );
    exit(0);
  }
  else
  {
    uint32_t tmpInt;
    uint32_t tmpIntArray[8];

    sscanf(argv[1], "0x%x", &tmpInt);
    srcAddr = (uint16_t) tmpInt;    
    
    sscanf(argv[2], "0x%x", &tmpInt);
    srcEp = (uint8_t) tmpInt;
        
    sscanf(argv[3], "%x:%x:%x:%x:%x:%x:%x:%x", &tmpIntArray[0], &tmpIntArray[1], &tmpIntArray[2], 
                         &tmpIntArray[3], &tmpIntArray[4], &tmpIntArray[5], &tmpIntArray[6], &tmpIntArray[7]);

    for(i=0; i < 8; i++)
    {
      srcIeee[i] = (uint8_t) tmpIntArray[i];
    }

    sscanf(argv[4], "0x%x", &tmpInt);
    dstEp = (uint8_t) tmpInt;
        
    sscanf(argv[5], "%x:%x:%x:%x:%x:%x:%x:%x", &tmpIntArray[0], &tmpIntArray[1], &tmpIntArray[2], 
                         &tmpIntArray[3], &tmpIntArray[4], &tmpIntArray[5], &tmpIntArray[6], &tmpIntArray[7]);

    printf("%s: srcAddr:%x, srcEp=%x\n", argv[0], srcAddr, srcEp);

    for(i=0; i < 8; i++)
    {
      dstIeee[i] = (uint8_t) tmpIntArray[i];
    }  
		
  }
      
  socketClientInit("127.0.0.1:11235", socketClientCb);  

  //bind on/off cluster (0x0006)
  sendBindOnOff(srcAddr, srcEp, srcIeee, dstEp, dstIeee, 0x0006);

  printf("\n\n");
  printf("--- On Off ------\n");
  printf("addr   ep   On/Off Cmd\n");
  printf("------ ---- -----------\n");

  //poll and processes incoming msg's unitl key is presses
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

/*********************************************************************
 * @fn          socketClientCb
 *
 * @brief      
 *
 * @param     
 *
 * @return     
 */
void socketClientCb( msgData_t *msg )
{
  srpcProcessMsg_t func;
  
  func = srpcProcessIncoming[(msg->cmdId)];
  if (func)
  {
    (*func)(msg->pData);
  }      
}

/*********************************************************************
 * @fn          sendBindOnOff
 *
 * @brief      
 *
 * @param     
 *
 * @return     
 */
void sendBindOnOff(uint16_t srcNwkAddr, uint8_t srcEp, uint8_t srcIeee[8], uint8_t dstEp, uint8_t dstIeee[8], uint16_t clusterId )
{     
  msgData_t msg;
  uint8_t* pRpcCmd = msg.pData;		 
  		
  //printf("srcNwkAddr, srcEp, srcIeee[8], dstEp, dstIeee[8], clusterId ");
  printf("sendBindOnOff: srcNwkAddr:%x, srcEp:%x, dstEp:%x \n",srcNwkAddr, srcEp, dstEp );

  msg.cmdId = SRPC_BIND_DEVICES;
  msg.len = 22;

  //Src Addr
  *pRpcCmd++ = srcNwkAddr & 0xFF;
  *pRpcCmd++ = (srcNwkAddr & 0xFF00) >> 8;
    
  //Src Ep
  *pRpcCmd++ = srcEp;

  //Src IEEE Addr
  *pRpcCmd++ = srcIeee[7];
  *pRpcCmd++ = srcIeee[6];    
  *pRpcCmd++ = srcIeee[5];
  *pRpcCmd++ = srcIeee[4];
  *pRpcCmd++ = srcIeee[3];
  *pRpcCmd++ = srcIeee[2];
  *pRpcCmd++ = srcIeee[1];
  *pRpcCmd++ = srcIeee[0];

  //Dst Ep
  *pRpcCmd++ = dstEp;

  //Dst IEEE Addr
  *pRpcCmd++ = dstIeee[7];
  *pRpcCmd++ = dstIeee[6];    
  *pRpcCmd++ = dstIeee[5];
  *pRpcCmd++ = dstIeee[4];
  *pRpcCmd++ = dstIeee[3];
  *pRpcCmd++ = dstIeee[2];
  *pRpcCmd++ = dstIeee[1];
  *pRpcCmd++ = dstIeee[0];

  //Src Addr
  *pRpcCmd++ = clusterId & 0xFF;
  *pRpcCmd++ = (clusterId & 0xFF00) >> 8;

  socketClientSendData (&msg);
}

/*********************************************************************
 * @fn          SRPC_OnOffCb
 *
 * @brief       This function proccesses the NewDevice message from the ZLL Gateway.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
uint8_t SRPC_OnOffCb(uint8_t *pMsg)
{
  uint16_t nwkAddr;
  uint8_t ep, commandID;

  //printf("SRPC_OnOffCb++\n");
	
  nwkAddr = BUILD_UINT16(pMsg[0], pMsg[1]);
  pMsg+=2;

  ep = *pMsg++;
  commandID = *pMsg++; 
	
  printf("0x%04X 0x%02X 0x%02X\n", nwkAddr, ep, commandID);
	
  return 0;
}
