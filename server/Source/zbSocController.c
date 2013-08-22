/**************************************************************************************************
 * Filename:       zll_controller.c
 * Description:    This file contains the interface to the UART.
 *
 *
 * Copyright (C) 2013 Texas Instruments Incorporated - http://www.ti.com/ 
 * 
 * 
 *  Redistribution and use in source and binary forms, with or without 
 *  modification, are permitted provided that the following conditions 
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the   
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <time.h>
#include <unistd.h>
#include <sys/timerfd.h>

#include "zbSocCmd.h"
#include "interface_devicelist.h"
#include "interface_grouplist.h"
#include "interface_scenelist.h"
#include "interface_srpcserver.h"
#include "socket_server.h"

#define MAX_DB_FILENAMR_LEN 255

uint8_t zclTlIndicationCb(epInfo_t *epInfo);
uint8_t zclNewDevIndicationCb(epInfo_t *epInfo);
uint8_t zclGetStateCb(uint8_t state, uint16_t nwkAddr, uint8_t endpoint);
uint8_t zclGetLevelCb(uint8_t level, uint16_t nwkAddr, uint8_t endpoint);
uint8_t zclGetHueCb(uint8_t hue, uint16_t nwkAddr, uint8_t endpoint);
uint8_t zclGetSatCb(uint8_t sat, uint16_t nwkAddr, uint8_t endpoint);
uint8_t zclGetTempCb(uint16_t temp, uint16_t nwkAddr, uint8_t endpoint);
uint8_t zclReadPowerRspCb(uint32_t power, uint16_t nwkAddr, uint8_t endpoint);
uint8_t zclReadEnergyRspCb(uint32_t energy_lo, uint32_t energy_hi, uint16_t nwkAddr, uint8_t endpoint);
uint8_t zclGetHumidCb(uint16_t temp, uint16_t nwkAddr, uint8_t endpoint);
uint8_t zclZoneSateChangeCb(uint32_t zoneState, uint16_t nwkAddr, uint8_t endpoint);
uint8_t SblDoneCb(uint8_t status);
uint8_t SblReportingCb(uint8_t phase, uint32_t location);
uint8_t certInstallResultIndCb(uint8_t result);
uint8_t keyEstablishmentStateIndCb(uint8_t state);
uint8_t zclDisplayMessageIndCb(uint8_t *zclPayload, uint8_t len);
uint8_t zclPublishPriceIndCb(uint8_t *zclPayload, uint8_t len);
uint8_t zclOnOffCb(uint8_t commandID, uint16_t nwkAddr, uint8_t endpoint);
uint8_t zclModelNameCb(uint8_t *model_name, uint8_t len, uint16_t nwkAddr, uint8_t endpoint);
uint8_t zclGenericReadAttrCb(uint8_t *data, uint16_t nwkAddr, uint8_t endpoint,
                             uint16_t clusterID, uint16_t attrID, uint8_t dataType);
uint8_t zclDiscoverAttrCb(uint16_t nwkAddr, uint8_t endpoint,
                          uint16_t clusterID, uint16_t attrID, uint8_t dataType);

static zbSocCallbacks_t zbSocCbs =
{
  zclTlIndicationCb,      // pfnTlIndicationCb - TouchLink Indication callback  
  zclNewDevIndicationCb,  // pfnNewDevIndicationCb - New Device Indication callback
  zclGetStateCb,          //pfnZclGetStateCb - ZCL response callback for get State
  zclGetLevelCb,          //pfnZclGetLevelCb_t - ZCL response callback for get Level
  zclGetHueCb,            // pfnZclGetHueCb - ZCL response callback for get Hue
  zclGetSatCb,            //pfnZclGetSatCb - ZCL response callback for get Sat
  zclGetTempCb,           //pfnZclGetTempCb - ZCL response callback for get Temp
  zclReadPowerRspCb,          //pfnZclGetPowerCb - ZCL response callback for get Power
  zclGetHumidCb,           //pfnZclGetTempCb - ZCL response callback for get Temp 
  zclZoneSateChangeCb,     //pfnZclZoneSateChangeCb - ZCL Command indicating Alarm Zone State Change
  SblDoneCb, 	   //pfnBootloadingDoneCb - Bootloader processing ended
  SblReportingCb, 	   //pfnBootloadingProgressReportingCb - Bootloader progress reporting
  certInstallResultIndCb,  // pfnCertInstallResultIndCb - Certificate Installation result reporting
  keyEstablishmentStateIndCb,  //pfnkeyEstablishmentStateIndCb - Key Establishment state change reporting
  zclDisplayMessageIndCb, //pfnZclDisplayMessageIndCb - ZCL response callback for DisplayMessage or request callback for unsolicited message
  zclPublishPriceIndCb, //pfnZclPublishPriceIndCb - ZCL response callback for GetCurrentMessage or request callback for unsolicited message
  zclOnOffCb, // pfnZclOnOffCb - ZCL cluster command callback for on/off
  zclModelNameCb,         // pfnZclModelNameCb - ZCL response callback for GetModelName
  zclGenericReadAttrCb, // pfnZclGenericReadAttributeCb - ZCL response callback for an otherwise unknown attribute
  zclDiscoverAttrCb,   // pfnZclDiscoverAttributeCb - ZCL response callback for a Discover command.
  zclReadEnergyRspCb,  // pfnZclReadEnergyRspCb - ZCL response callback for get Energy
};

uint8_t uartDebugPrintsEnabled = 0;
int current_poll_timeout = -1;


void usage( char* exeName )
{
    printf("Usage: ./%s <port>\n", exeName);
    printf("Eample: ./%s /dev/ttyACM0\n", exeName);
}


int main(int argc, char* argv[])
{
  int retval = 0;
  char * selected_serial_port;
  int numTimerFDs = NUM_OF_TIMERS;
  timerFDs_t *timer_fds = malloc(  NUM_OF_TIMERS * sizeof( timerFDs_t ) );
  char dbFilename[MAX_DB_FILENAMR_LEN];
 
  printf("%s -- %s %s\n", argv[0], __DATE__, __TIME__ );
 
  // accept only 1
  if( argc < 2 )
  {
    usage(argv[0]);
    printf("attempting to use /dev/ttyACM0\n\n");
	selected_serial_port = "/dev/ttyACM0";
  }
  else
  {
  	selected_serial_port = argv[1];
  }
  
  zbSocOpen( selected_serial_port );
  
  if( serialPortFd == -1 )
  {
    exit(-1);
  }

  if (argc > 2)
  {
  	uartDebugPrintsEnabled = atoi(argv[2]);
  }

  if (argc > 3)
  {
    if (atoi(argv[3]) == 1)
    {
      zbSocResetToFn();
      printf("Sent Reset to Factory New\n");
    }
      else if (atoi(argv[3])  > 1)
    {
      //...
    }
  }
  
  zbSocGetTimerFds(timer_fds);
  
  sprintf(dbFilename, "%.*s/devicelistfile.dat",strrchr(argv[0],'/') - argv[0] , argv[0]);
  devListInitDatabase(dbFilename);
  sprintf(dbFilename, "%.*s/grouplistfile.dat",strrchr(argv[0],'/') - argv[0] , argv[0]);
  groupListInitDatabase(dbFilename);  
  sceneListRestorScenes();
  
  zbSocRegisterCallbacks( zbSocCbs );    
  SRPC_Init();
  
  printf("resetting CC2530");
  zbSocResetLocalDevice();

  //wait for reset (need a better way to do this)
  usleep(30000);

  //exit bootloader
  zbSocForceRun();

  while(1)
  {          
    int numClientFds = socketSeverGetNumClients(); 
    
	  //poll on client socket fd's and the zbSoC serial port for any activity
		if(numClientFds)
		{
		  int pollFdIdx;  		   
      int timerFdIdx;
		  int *client_fds = malloc(  numClientFds * sizeof( int ) );

		  //socket client FD's + serialPortFd serial port FD
      struct pollfd *pollFds = malloc(  ((numClientFds + 1 + numTimerFDs) * sizeof( struct pollfd )) );
		  
		  if(client_fds && pollFds)	
		  {
		    //set the serialPortFd serial port FD in the poll file descriptors
		    pollFds[0].fd = serialPortFd;
#if (!HAL_UART_SPI)
          //Fd will be a characture driver
  			pollFds[0].events = POLLIN;
#else	      
          //Fd Will be GPIO
    	  pollFds[0].events = POLLPRI;
  			
          //try to read any messages that might already be wating
          if(zbSocTransportPoll())
          {
            zbSocProcessRpc();
          }
    	 
          //flush events
          int buf[1];
          lseek(pollFds[0].fd, SEEK_SET, 0);
          read(pollFds[0].fd, buf, 1);
            	  
#endif  			
		    //Set the socket file descriptors  		    
	  	  socketSeverGetClientFds(client_fds, numClientFds);  			    	  	    	  	 
		  	for(pollFdIdx=0; pollFdIdx < numClientFds; pollFdIdx++)
  	  	{
  			  pollFds[pollFdIdx+1].fd = client_fds[pollFdIdx];
  			  pollFds[pollFdIdx+1].events = POLLIN | POLLRDHUP;
  			  //printf("%s: adding fd %d to poll()\n", argv[0], pollFds[pollFdIdx].fd); 	  				
  		  }	
        for(timerFdIdx=0; timerFdIdx < numTimerFDs; timerFdIdx++)
        {
          pollFds[numClientFds+1+timerFdIdx].fd = timer_fds[timerFdIdx].fd;
          pollFds[numClientFds+1+timerFdIdx].events =POLLIN;
          //printf("%s: adding fd %d to poll()\n", argv[0], pollFds[pollFdIdx].fd); 					
        } 

//        printf("%s: waiting for poll()\n", argv[0]);

        poll(pollFds, (numClientFds+1+numTimerFDs), current_poll_timeout);

        //printf("%s: got poll()\n", argv[0]);
        
        //did the poll unblock because of the zllSoC serial?
        if(pollFds[0].revents)
        {
          printf("Message from the ZigBee SoC\n");
          zbSocProcessRpc();
        }

        //did the poll unblock because of activity on the socket interface?
        for(pollFdIdx=1; pollFdIdx < (numClientFds+1); pollFdIdx++)
        {
          if ( (pollFds[pollFdIdx].revents) )
          {
            printf("Message from the client\n");
            socketSeverPoll(pollFds[pollFdIdx].fd, pollFds[pollFdIdx].revents);
          }
        }          
		
        //did the poll unblock because of timer expiration?
        for(timerFdIdx=0; timerFdIdx < numTimerFDs; timerFdIdx++)
        {
          if (pollFds[numClientFds+1+timerFdIdx].revents)
        {
            printf("Timer expired: #%d\n", timerFdIdx);
            timer_fds[timerFdIdx].callback();
        }
        }
        	  
        free( client_fds );	  
        free( pollFds );	  		
        }
      }  		           
  }    

  return retval;
}


uint8_t zclTlIndicationCb(epInfo_t *epInfo)
{
  zclNewDevIndicationCb(epInfo);
  return 0;  
}

uint8_t zclNewDevIndicationCb(epInfo_t *epInfo)
{
	epInfo_t* oldRec;
	epInfoExtended_t epInfoEx;

	
	oldRec = devListGetDeviceByIeeeEp(epInfo->IEEEAddr, epInfo->endpoint);

	if (oldRec != NULL)
	{
		if (epInfo->nwkAddr != oldRec->nwkAddr)
		{
			epInfoEx.type = EP_INFO_TYPE_UPDATED;
			epInfoEx.prevNwkAddr = oldRec->nwkAddr;
			devListRemoveDeviceByNaEp(oldRec->nwkAddr, oldRec->endpoint); //theoretically, update the database record in place is possible, but this other approach is selected to provide change logging. Records that are marked as deleted soes not have to be phisically deleted (e.g. by avoiding consilidation) and thus the database can be used as connection log
		}
		else
		{
			//not checking if any of the records has changed. assuming that for a given device (ieee_addr+endpoint_number) nothing will change except the network address.
			epInfoEx.type = EP_INFO_TYPE_EXISTING;
		}
	}
	else
	{
		epInfoEx.type = EP_INFO_TYPE_NEW;
	}

	if (epInfoEx.type != EP_INFO_TYPE_EXISTING)
	{
  devListAddDevice(epInfo);
		epInfoEx.epInfo = epInfo;
		RSPC_SendEpInfo(&epInfoEx);
	}
  return 0;  
}

uint8_t zclGetStateCb(uint8_t state, uint16_t nwkAddr, uint8_t endpoint)
{
  SRPC_CallBack_getStateRsp(state, nwkAddr, endpoint, 0);
  return 0;  
}

uint8_t zclGetLevelCb(uint8_t level, uint16_t nwkAddr, uint8_t endpoint)
{
  SRPC_CallBack_getLevelRsp(level, nwkAddr, endpoint, 0);
  return 0;  
}

uint8_t zclGetHueCb(uint8_t hue, uint16_t nwkAddr, uint8_t endpoint)
{
  SRPC_CallBack_getHueRsp(hue, nwkAddr, endpoint, 0);
  return 0;  
}

uint8_t zclGetSatCb(uint8_t sat, uint16_t nwkAddr, uint8_t endpoint)
{
  SRPC_CallBack_getSatRsp(sat, nwkAddr, endpoint, 0);
  return 0;  
}

uint8_t zclGetTempCb(uint16_t temp, uint16_t nwkAddr, uint8_t endpoint)
{
  SRPC_CallBack_getTempRsp(temp, nwkAddr, endpoint, 0);
  
  printf("\nzclGetTempCb:\n    Network Addr : 0x%04x\n    End Point    : 0x%02x\n    temp   : %02x\n\n", 
    nwkAddr, endpoint, temp); 
  
  return 0;  
}

uint8_t zclReadPowerRspCb(uint32_t power, uint16_t nwkAddr, uint8_t endpoint)
{
  SRPC_CallBack_readPowerRsp(power, nwkAddr, endpoint, 0);

  printf("\nzclReadPowerRspCb:\n    Network Addr : 0x%04x\n    End Point    : 0x%02x\n    power   : %02x\n\n", 
    nwkAddr, endpoint, power); 
  
  return 0;  
}

uint8_t zclReadEnergyRspCb(uint32_t energy_lo, uint32_t energy_hi, uint16_t nwkAddr, uint8_t endpoint)
{
  SRPC_CallBack_readEnergyRsp(energy_lo, energy_hi, nwkAddr, endpoint, 0);

  printf("\nzclReadEnergyRspCb:\n    Network Addr : 0x%04x\n    End Point    : 0x%02x\n"
         "    energy  : %04x%08x\n\n",
         nwkAddr, endpoint, energy_hi, energy_lo);

  return 0;
}

uint8_t zclGetHumidCb(uint16_t Humid, uint16_t nwkAddr, uint8_t endpoint)
{
  SRPC_CallBack_getHumidRsp(Humid, nwkAddr, endpoint, 0);
  
  printf("\nzclGetTempCb:\n    Network Addr : 0x%04x\n    End Point    : 0x%02x\n    Humid   : %02x\n\n", 
    nwkAddr, endpoint, Humid); 
  
  return 0; 
} 

uint8_t zclZoneSateChangeCb(uint32_t zoneState, uint16_t nwkAddr, uint8_t endpoint)
{
  SRPC_CallBack_zoneSateInd(zoneState, nwkAddr, endpoint, 0);
  
  printf("\nzclZoneSateChangeCb:\n    Network Addr : 0x%04x\n    End Point    : 0x%02x\n    zoneState   : %02x\n\n", 
    nwkAddr, endpoint, zoneState); 
  
  return 0; 
}

uint8_t zclOnOffCb(uint8_t commandID, uint16_t nwkAddr, uint8_t endpoint)
{

  SRPC_CallBack_OnOffCmd(commandID, nwkAddr, endpoint, 0);

  printf("zclOnOffCb:\n    Network Addr : 0x%04x\n    End Point    : 0x%02x\n    commandID   : %02x\n\n", nwkAddr, endpoint, commandID );

  return 0;
}

uint8_t zclModelNameCb(uint8_t *model_name, uint8_t len, uint16_t nwkAddr, uint8_t endpoint)
{
    int i;

    SRPC_CallBack_ModelName(model_name, len, nwkAddr, endpoint, 0);

    printf("\nzclModelNameCb:\n    Network Addr : 0x%04x\n    End Point    : 0x%02x\n    Name        : ",
           nwkAddr, endpoint);
    for (i = 0; i < len; i++)
        putchar(model_name[i]);
    putchar('\n');

    return 0;
}

uint8_t zclGenericReadAttrCb(uint8_t *data, uint16_t nwkAddr, uint8_t endpoint,
                             uint16_t clusterID, uint16_t attrID, uint8_t dataType)
{
    int i;
    uint8_t len;

    if (dataType == 0x00)
        len = 0;
    else if (dataType < 0x38)
        len = (dataType & 0x07) + 1;
    else if (dataType < 0x40)
        len = ((dataType & 0x07) + 1) * 2;
    else if (dataType == ZCL_DATATYPE_CHAR_STRING)
        len = *data++;
    else
    {
        /* Don't know how to handle this, so skip out silently */
        printf("zclGenericReadAttrCb: unknown data type %02x\n", dataType);
        return 0;
    }

    SRPC_CallBack_ReadAttribute(data, len, nwkAddr, endpoint, clusterID, attrID, dataType, 0);

    printf("\nzclGenericReadAttrCb:\n"
           "    Network Addr : 0x%04x\n"
           "    End Point    : 0x%02x\n"
           "    Cluster ID   : 0x%04x\n"
           "    Attribute ID : 0x%04x\n"
           "    Data type    : %d\n"
           "    Data         :",
           nwkAddr, endpoint, clusterID, attrID, dataType);
    for (i = 0; i < len; i++)
        printf(" %02x", data[i]);
    putchar('\n');

    return 0;
}

uint8_t zclDiscoverAttrCb(uint16_t nwkAddr, uint8_t endpoint,
                          uint16_t clusterID, uint16_t attrID, uint8_t dataType)
{
    SRPC_CallBack_DiscoverAttribute(nwkAddr, endpoint, clusterID, attrID, dataType, 0);

    printf("\nzclGenericReadAttrCb:\n"
           "    Network Addr : 0x%04x\n"
           "    End Point    : 0x%02x\n"
           "    Cluster ID   : 0x%04x\n"
           "    Attribute ID : 0x%04x\n"
           "    Data type    : 0x%02x\n",
           nwkAddr, endpoint, clusterID, attrID, dataType);

    return 0;
}

uint8_t SblDoneCb(uint8_t status)
{
	SRPC_CallBack_bootloadingDone(status);
	return 0;
}

uint8_t SblReportingCb(uint8_t phase, uint32_t location)
{
	SRPC_CallBack_loadImageProgress(phase, location);
	return 0;
}

uint8_t certInstallResultIndCb(uint8_t result)
{
  SRPC_CallBack_certInstallResultInd(result);

  return 0;
}

uint8_t keyEstablishmentStateIndCb(uint8_t state)
{
  SRPC_CallBack_keyEstablishmentStateInd(state);

  return 0;
}

uint8_t zclDisplayMessageIndCb(uint8_t *zclPayload, uint8_t len)
{
  SRPC_CallBack_displayMessageInd(zclPayload, len);

  return 0;
}

uint8_t zclPublishPriceIndCb(uint8_t *zclPayload, uint8_t len)
{
  SRPC_CallBack_publishPriceInd(zclPayload, len);

  return 0;
}


