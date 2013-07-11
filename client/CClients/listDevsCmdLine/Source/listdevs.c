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
 
#define CONSOLEDEVICE "/dev/console"

void socketClientCb( msgData_t *msg ); 
uint8_t SRPC_NewDevice(uint8_t *msg);
static void srpcSendGetDevices( void );

typedef uint8_t (*srpcProcessMsg_t)(uint8_t *msg);

typedef struct
{
  char * str;
  uint16_t id;
} device_id_strings_t;

device_id_strings_t device_id_strings[] = 
{
  {"On/Off Switch",                        0x0000},        
  {"Level Control Switch",                 0x0001},        
  {"On/Off Output",                        0x0002},        
  {"Level Controllable Output",            0x0003},        
  {"Scene Selector",                       0x0004},        
  {"Configuration Tool",                   0x0005},        
  {"Remote Control",                       0x0006},        
  {"Combined Interface",                   0x0007},        
  {"Range Extender",                       0x0008},        
  {"Mains Power Outlet",                   0x0009},        
  {"Door Lock",                            0x000A},        
  {"Door Lock Controller",                 0x000B},        
  {"Simple Sensor",                        0x000C},        
  {"On/Off Light",                         0x0100},        
  {"Dimmable Light",                       0x0101},        
  {"Color Dimmable Light",                 0x0102},        
  {"On/Off Light Switch",                  0x0103},        
  {"Dimmer Switch",                        0x0104},        
  {"Color Dimmer Switch",                  0x0105},        
  {"Light Sensor",                         0x0106},        
  {"Occupancy Sensor",                     0x0107},        
  {"Shade",                                0x0200},        
  {"Shade Controller",                     0x0201},        
  {"Window Covering Device",               0x0202},        
  {"Window Covering Controller",           0x0203},        
  {"Heating/Cooling Unit",                 0x0300},        
  {"Thermostat",                           0x0301},        
  {"Temperature Sensor",                   0x0302},        
  {"Pump",                                 0x0303},        
  {"Pump Controller",                      0x0304},        
  {"Pressure Sensor",                      0x0305},        
  {"Flow Sensor",                          0x0306},        
  {"IAS Control and Indicating Equipment", 0x0400},        
  {"IAS Ancillary Control Equipment",      0x0401},        
  {"IAS Zone",                             0x0402},        
  {"IAS Warning Device",                   0x0403},        
};

srpcProcessMsg_t srpcProcessIncoming[] =
{
  NULL,
  SRPC_NewDevice, 
};

int keyFd;

/*********************************************************************
 * @fn          main
 *
 * @brief      
 *
 * @param     
 *
 * @return     
 */
int main(int argc, char *argv[])
{
  int ret;
      
  socketClientInit("127.0.0.1:11235", socketClientCb);
      

  printf("--- List of devices ------------------------------------------------------------------------------------------------------------\n");
  printf("type     addr   ep   profID devID  IEEEAddr                flgs prvadr deviceIdString                       deviceGivenName     \n");
  printf("-------- ------ ---- ------ ------ ----------------------- ---- ------ ------------------------------------ --------------------\n");
  
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
 * @fn          srpcSendGetDevices
 *
 * @brief      
 *
 * @param     
 *
 * @return     
 */
static void srpcSendGetDevices( void )
{ 
  msgData_t srpcCmd;
  
  srpcCmd.cmdId = SRPC_GET_DEVICES;
  srpcCmd.len = 0;
  
  socketClientSendData (&srpcCmd);
    
  return; 
}


/*********************************************************************
 * @fn          get_device_id_string
 *
 * @brief      
 *
 * @param     
 *
 * @return     
 */
char * get_device_id_string(uint16_t id)
{
	char * device_id_string = "Unknown device";
	int x;

	for (x = 0; x < (sizeof(device_id_strings) / sizeof(device_id_strings[0])); x++)
	{
		if (id == device_id_strings[x].id)
		{
			device_id_string = device_id_strings[x].str;
			break;
		}
	}

	return device_id_string;
}


/*********************************************************************
 * @fn          SRPC_NewDevice
 *
 * @brief       This function proccesses the NewDevice message from the ZLL Gateway.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
uint8_t SRPC_NewDevice(uint8_t *pMsg)
{    
  epInfo_t epInfo;
  epInfoExtended_t epInfoEx;
  uint8_t devNameStrLen;
  int i;
  char * devNameStr;

  epInfoEx.epInfo = &epInfo;

  epInfo.nwkAddr = BUILD_UINT16(pMsg[0], pMsg[1]);
  pMsg+=2;

  epInfo.endpoint = *pMsg++;
  
  epInfo.profileID = BUILD_UINT16(pMsg[0], pMsg[1]);
  pMsg+=2;
  
  epInfo.deviceID = BUILD_UINT16(pMsg[0], pMsg[1]);
  pMsg+=2;

  epInfo.version = *pMsg++;
  
  //skip name for now
  devNameStrLen = *pMsg++;
  devNameStr = (char *)pMsg;
  pMsg += devNameStrLen;

  epInfo.status = *pMsg++;

    for(i = 0; i < 8; i++)
    {
      //printf("srpcParseEpInfp: IEEEAddr[%d] = %x\n", i, epInfo->IEEEAddr[i]);
      epInfo.IEEEAddr[i] = *pMsg++;
    }
	epInfoEx.type = *pMsg++;
	epInfoEx.prevNwkAddr = BUILD_UINT16(pMsg[0], pMsg[1]);
	pMsg+=2;
	epInfo.flags = *pMsg++;
   
  
  printf("%-8s", 
  	epInfoEx.type == EP_INFO_TYPE_EXISTING ? "EXISTING" : 
  	epInfoEx.type == EP_INFO_TYPE_NEW ? "NEW" : 
  	epInfoEx.type == EP_INFO_TYPE_REMOVED ? "REMOVED" : 
  	"UPDATED"
  );
  
  printf(" 0x%04X 0x%02X 0x%04X 0x%04X ",
    epInfo.nwkAddr,
    epInfo.endpoint,
    epInfo.profileID,
    epInfo.deviceID
    );  

  for(i = 0; i < 8; i++)
  {
	printf("%s%02X", i > 0 ? ":" : "", epInfo.IEEEAddr[7-i]);
  }
  
  printf(" 0x%02X ", epInfo.flags);
  
  if (epInfoEx.type == EP_INFO_TYPE_UPDATED)
  {
	  printf("0x%04X", epInfoEx.prevNwkAddr);
  }
  else
  {
  	printf("------");
  }

  printf(" %-*s \"%.*s\"\n", 36, get_device_id_string(epInfo.deviceID), devNameStrLen, devNameStr);
  
  return 0;  
}
