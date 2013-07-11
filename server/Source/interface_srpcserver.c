/**************************************************************************************************
 * Filename:       interface_srpcserver.c
 * Description:    Socket Remote Procedure Call Interface - sample device application.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <sys/ioctl.h>
#include <poll.h>

#include "interface_srpcserver.h"
#include "socket_server.h"
#include "interface_devicelist.h"
#include "interface_grouplist.h"
#include "interface_scenelist.h"

#include "hal_defs.h"

#include "zbSocCmd.h"

void SRPC_RxCB( int clientFd );
void SRPC_ConnectCB( int status ); 

static uint8_t SRPC_setDeviceState(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_setDeviceLevel(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_setDeviceColor(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_getDeviceState(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_getDeviceLevel(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_getDeviceHue(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_getDeviceSat(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_bindDevices(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_getGroups(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_addGroup(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_getScenes(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_storeScene(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_recallScene(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_identifyDevice(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_removeDevice(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_close(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_getDevices(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_getDeviceTemp(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_getDevicePower(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_getDeviceHumid(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_notSupported(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_sblDownloadImage(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_sblAbort(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_changeDeviceName(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_installCertificate(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_getLastMessage(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_getCurrentPrice(uint8_t *pBuf, uint32_t clientFd);

//SRPC Interface call back functions
static void SRPC_CallBack_addGroupRsp(uint16_t groupId, char *nameStr, uint32_t clientFd);
static void SRPC_CallBack_addSceneRsp(uint16_t groupId, uint8_t sceneId, char *nameStr, uint32_t clientFd);

static void srpcSend(uint8_t* srpcMsg, int fdClient);
static void srpcSendAll(uint8_t* srpcMsg);


//local definitions

#define SOCKET_BOOTLOADING_STATE_IDLE 0
#define SOCKET_BOOTLOADING_STATE_ACTIVE 1


//type definitions

typedef uint8_t (*srpcProcessMsg_t)(uint8_t *pBuf, uint32_t clientFd);

//global constants

const srpcProcessMsg_t rpcsProcessIncoming[] =
{
  SRPC_close,           //SRPC_CLOSE
  SRPC_getDevices,      //SRPC_GET_DEVICES     
  SRPC_setDeviceState,  //SRPC_SET_DEV_STATE     
  SRPC_setDeviceLevel,  //SRPC_SET_DEV_LEVEL     
  SRPC_setDeviceColor,  //SRPC_SET_DEV_COLOR
  SRPC_getDeviceState,  //SRPC_GET_DEV_STATE     
  SRPC_getDeviceLevel,  //SRPC_GET_DEV_LEVEL     
  SRPC_getDeviceHue,    //SRPC_GET_DEV_HUE  
  SRPC_getDeviceSat,    //SRPC_GET_DEV_SAT        
  SRPC_bindDevices,     //SRPC_BIND_DEVICES      
  SRPC_getDeviceTemp,   //SRPC_GET_THERM_READING 
  SRPC_getDevicePower,  //SRPC_GET_POWER_READING 
  SRPC_notSupported,    //SRPC_DISCOVER_DEVICES  
  SRPC_notSupported,    //SRPC_SEND_ZCL          
  SRPC_getGroups,       //SRPC_GET_GROUPS    
  SRPC_addGroup,        //SRPC_ADD_GROUP     
  SRPC_getScenes,       //SRPC_GET_SCENES    
  SRPC_storeScene,      //SRPC_STORE_SCENE       
  SRPC_recallScene,     //SRPC_RECALL_SCENE      
  SRPC_identifyDevice,  //SRPC_IDENTIFY_DEVICE   
  SRPC_changeDeviceName,//RPCS_CHANGE_DEVICE_NAME  
  SRPC_removeDevice,    //SRPC_REMOVE_DEVICE    
  SRPC_getDeviceHumid,  //SRPC_GET_HUMID_READING            
  SRPC_sblDownloadImage, //SRPC_SBL_DOWNLOAD_IMAGE
  SRPC_sblAbort, //SRPC_SBL_ABORT
  SRPC_installCertificate,  //SRPC_INSTALL_CERTIFICATE
  SRPC_getLastMessage,  //SRPC_GET_LAST_MESSAGE
  SRPC_getCurrentPrice, //SRPC_GET_CURRENT_PRICE
};

//global variables

uint32_t bootloader_initiator_clientFd;
uint32 cert_install_clientFd = 0;
uint32 get_last_message_clientFd = 0;
uint32 get_current_price_clientFd = 0;
uint16_t SocketBootloadingState = SOCKET_BOOTLOADING_STATE_IDLE;


/***************************************************************************************************
 * @fn      srpcParseEpInfp - Parse epInfo and prepare the SRPC message.
 *
 * @brief   Parse epInfo and prepare the SRPC message.
 * @param   epInfo_t* epInfo
 *
 * @return  pSrpcMessage
 ***************************************************************************************************/
static uint8_t* srpcParseEpInfo(epInfoExtended_t* epInfoEx)
{
  uint8_t i;
  uint8_t *pSrpcMessage, *pTmp, devNameLen = 1, pSrpcMessageLen;  

  //printf("srpcParseEpInfo++\n");   
   
  //RpcMessage contains function ID param Data Len and param data
  if( epInfoEx->epInfo->deviceName )
  {
    devNameLen = strlen(epInfoEx->epInfo->deviceName);
  }
  
  //sizre of EP infor - the name char* + num bytes of device name
  pSrpcMessageLen = sizeof(epInfo_t) - sizeof(char*) + devNameLen;
  pSrpcMessage = malloc(pSrpcMessageLen + 2);
  
  pTmp = pSrpcMessage;
  
  if( pSrpcMessage )
  {
    //Set func ID in RPCS buffer
    *pTmp++ = SRPC_NEW_DEVICE;
    //param size
    *pTmp++ = pSrpcMessageLen;
    
    *pTmp++ = LO_UINT16(epInfoEx->epInfo->nwkAddr);
    *pTmp++ = HI_UINT16(epInfoEx->epInfo->nwkAddr);
    *pTmp++ = epInfoEx->epInfo->endpoint;
    *pTmp++ = LO_UINT16(epInfoEx->epInfo->profileID);
    *pTmp++ = HI_UINT16(epInfoEx->epInfo->profileID);
    *pTmp++ = LO_UINT16(epInfoEx->epInfo->deviceID);
    *pTmp++ = HI_UINT16(epInfoEx->epInfo->deviceID);
    *pTmp++ = epInfoEx->epInfo->version;  
    
    if( epInfoEx->epInfo->deviceName )
    {    
	  *pTmp++=devNameLen;
      for(i = 0; i < devNameLen; i++)
      {
        *pTmp++ = epInfoEx->epInfo->deviceName[i];
      }
    }
    else
    {
      *pTmp++=0;
    }
    *pTmp++ = epInfoEx->epInfo->status;    
    
    for(i = 0; i < 8; i++)
    {
      //printf("srpcParseEpInfp: IEEEAddr[%d] = %x\n", i, epInfo->IEEEAddr[i]);
      *pTmp++ = epInfoEx->epInfo->IEEEAddr[i];
    }
	*pTmp++ = epInfoEx->type;
	*pTmp++ = LO_UINT16(epInfoEx->prevNwkAddr);
	*pTmp++ = HI_UINT16(epInfoEx->prevNwkAddr);
	*pTmp++ = epInfoEx->epInfo->flags; //bit 0 : start, bit 1: end

  }
  //printf("srpcParseEpInfp--\n");

//  printf("srpcParseEpInfo %0x:%0x\n", epInfo->nwkAddr, epInfo->endpoint);   

  printf("srpcParseEpInfo: %s device, nwkAddr=0x%04X, endpoint=0x%X, profileID=0x%04X, deviceID=0x%04X, flags=0x%02X", 
  	epInfoEx->type == EP_INFO_TYPE_EXISTING ? "EXISTING" : 
	  epInfoEx->type == EP_INFO_TYPE_NEW ? "NEW" : 
	  "UPDATED",
  	epInfoEx->epInfo->nwkAddr,
	epInfoEx->epInfo->endpoint,
	epInfoEx->epInfo->profileID,
	epInfoEx->epInfo->deviceID,
	epInfoEx->epInfo->flags
  	);  

	if (epInfoEx->type == EP_INFO_TYPE_UPDATED)
	{
		printf(", prevNwkAddr=0x%04X\n", epInfoEx->prevNwkAddr);
	}
	
	printf("\n");

  return pSrpcMessage;
}    

/***************************************************************************************************
 * @fn      srpcSend
 *
 * @brief   Send a message over SRPC to a clients.
 * @param   uint8_t* srpcMsg - message to be sent
 *
 * @return  Status
 ***************************************************************************************************/
static void srpcSend(uint8_t* srpcMsg, int fdClient)
{ 
  int rtn;
 
  rtn = socketSeverSend(srpcMsg, (srpcMsg[SRPC_MSG_LEN] + 2), fdClient);
  if (rtn < 0) 
  {
    printf("ERROR writing to socket\n");
  }
    
  return; 
}

/***************************************************************************************************
 * @fn      srpcSendAll
 *
 * @brief   Send a message over SRPC to all clients.
 * @param   uint8_t* srpcMsg - message to be sent
 *
 * @return  Status
 ***************************************************************************************************/
static void srpcSendAll(uint8_t* srpcMsg)
{ 
  int rtn;
 
  rtn = socketSeverSendAllclients(srpcMsg, (srpcMsg[SRPC_MSG_LEN] + 2));
  if (rtn < 0) 
  {
    printf("ERROR writing to socket\n");
  }
    
  return; 
}


/***************************************************************************************************
 * @fn      SRPC_CallBack_loadImageRsp
 *
 * @brief   
  *
 * @return  
 ***************************************************************************************************/
void SRPC_CallBack_loadImageRsp(uint8_t result, uint32_t clientFd)
{
  uint8_t * pBuf;  
  uint8_t pSrpcMessage[2 + 1];
  
  pBuf = pSrpcMessage;
  
  //Set func ID in RPCS buffer
  *pBuf++ = SRPC_SBL_RSP;

  //param size
  *pBuf++ = 1;
  
  *pBuf++ = result;
        
  printf("SRPC_CallBack_loadImageRsp: result=%d\n", result);

  //Send SRPC
  srpcSend(pSrpcMessage, clientFd);  

  if ((result != SBL_PENDING) && (clientFd == bootloader_initiator_clientFd))
  {
  	SocketBootloadingState = SOCKET_BOOTLOADING_STATE_IDLE;
  }
                  
  return;              
}


/***************************************************************************************************
 * @fn      SRPC_CallBack_SendProgressReport
 *
 * @brief   
  *
 * @return  
 ***************************************************************************************************/
void SRPC_CallBack_SendProgressReport(uint8_t phase, uint32_t location, uint32_t clientFd)
{
  uint8_t * pBuf;  
    
  uint8_t pSrpcMessage[2 + 5];
  
  pBuf = pSrpcMessage;
  
  //Set func ID in RPCS buffer
  *pBuf++ = SRPC_SBL_PROGRESS;

  //param size
  *pBuf++ = 5;
  
  *pBuf++ = phase;
        
  *pBuf++ = (location >> 0) % 0xFF;
  *pBuf++ = (location >> 8) % 0xFF;
  *pBuf++ = (location >> 16) % 0xFF;
  *pBuf++ = (location >> 24) % 0xFF;

  printf("SRPC_CallBack_loadImageProgress: phase=%d, location=0x%08X\n", phase, location);

  //Send SRPC
  srpcSend(pSrpcMessage, clientFd);  

  return;              
}


/*********************************************************************
 * @fn          SRPC_sblDownloadImage
 *
 * @brief       This function loads an existing image from the Host filesystem to the ZigBee device.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
static uint8_t SRPC_sblDownloadImage(uint8_t *pBuf, uint32_t clientFd)
{
  uint16_t filenameLength;
  uint8_t progressReportingInterval;
  char * filename;

  pBuf+=2; //increment past SRPC header
  filenameLength = BUILD_UINT16(pBuf[0], pBuf[1]);
  pBuf+=2;
  progressReportingInterval = *pBuf++;
  //todo: verify that the actual received packet is long enough to contain the filename of the length reported.
  filename = (char *)pBuf;
  filename[filenameLength] = '\0'; //todo: make sure pBuf is long enough to contain this extra character

  if (SocketBootloadingState != SOCKET_BOOTLOADING_STATE_IDLE)
  {
	SRPC_CallBack_loadImageRsp(SBL_BUSY, clientFd);
  } 
  else if (zbSocSblInitiateImageDownload(filename, progressReportingInterval) == SUCCESS)
  {
    SocketBootloadingState = SOCKET_BOOTLOADING_STATE_ACTIVE;
	bootloader_initiator_clientFd = clientFd;

    SRPC_CallBack_loadImageRsp(SBL_PENDING, clientFd);
  }
  else
  {
    SRPC_CallBack_loadImageRsp(SBL_ERROR_OPENING_FILE, clientFd);
  }

  return 0;
}


/***************************************************************************************************
 * @fn      SRPC_killLoadingImage
 *
 * @brief   
  *
 * @return  
 ***************************************************************************************************/
void SRPC_killLoadingImage(void)
{
	zbSocFinishLoadingImage();
	SocketBootloadingState = SOCKET_BOOTLOADING_STATE_IDLE;
}


/*********************************************************************
 * @fn          SRPC_sblAbort
 *
 * @brief
 *
 * @return
 */
static uint8_t SRPC_sblAbort(uint8_t *pBuf, uint32_t clientFd)
{
	if (SocketBootloadingState != SOCKET_BOOTLOADING_STATE_IDLE)
	{
		SRPC_killLoadingImage( );
		if (clientFd == bootloader_initiator_clientFd)
		{
			SRPC_CallBack_loadImageRsp(SBL_ABORTED_BY_USER, clientFd);
		}
		else
		{
			SRPC_CallBack_loadImageRsp(SBL_REMOTE_ABORTED_BY_USER, clientFd);
			SRPC_CallBack_loadImageRsp(SBL_ABORTED_BY_ANOTHER_USER , bootloader_initiator_clientFd);
		}
	}
	else
	{
		SRPC_CallBack_loadImageRsp(SBL_NO_ACTIVE_DOWNLOAD, clientFd);
	}

	return 0;
}


/*********************************************************************
 * @fn          SRPC_ProcessIncoming
 *
 * @brief       This function processes incoming messages.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
void SRPC_ProcessIncoming(uint8_t *pBuf, uint32_t clientFd)
{
  srpcProcessMsg_t func;
  
//  printf("SRPC_ProcessIncoming++[%x]\n", pBuf[SRPC_FUNC_ID]);
  /* look up and call processing function */
  func = rpcsProcessIncoming[(pBuf[SRPC_FUNC_ID] & ~(0x80))];
  if (func)
  {
    (*func)(pBuf, clientFd);
  }
  else
  {
    //printf("Error: no processing function for CMD 0x%x\n", pBuf[SRPC_FUNC_ID]); 
  }
  
  //printf("SRPC_ProcessIncoming--\n");
}

/*********************************************************************
 * @fn          SRPC_addGroup
 *
 * @brief       This function exposes an interface to add a devices to a group.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
static uint8_t SRPC_addGroup(uint8_t *pBuf, uint32_t clientFd)
{
  uint16_t dstAddr;
  uint8_t addrMode;
  uint8_t endpoint;
  char *nameStr;
  uint8_t nameLen;
  uint16_t groupId;
  
  //printf("SRPC_addGroup++\n");
       
  //increment past SRPC header
  pBuf+=2;

  addrMode = (afAddrMode_t)*pBuf++;   
  dstAddr = BUILD_UINT16(pBuf[0], pBuf[1]);
  pBuf += Z_EXTADDR_LEN;
  endpoint = *pBuf++;
  // index past panId
  pBuf += 2;
  
  nameLen = *pBuf++;
  nameStr = malloc(nameLen + 2);
  nameStr[0] = nameLen;
  int i;
  for(i = 0; i < nameLen; i++)
  {
    nameStr[i+1] = *pBuf++;
  } 
  nameStr[nameLen + 1] = '\0';
  
  //printf("SRPC_addGroup++: %x:%x:%x name[%d] %s \n", dstAddr, addrMode, endpoint, nameLen, nameStr  +1);
          
  groupId = groupListAddDeviceToGroup( nameStr, dstAddr, endpoint);        
  zbSocAddGroup(groupId, dstAddr, endpoint, addrMode);

  SRPC_CallBack_addGroupRsp(groupId, nameStr, clientFd);

  free(nameStr);
  
  //printf("SRPC_addGroup--\n");
  
  return 0;
}

/*********************************************************************
 * @fn          uint8_t SRPC_storeScene(uint8_t *pBuf, uint32_t clientFd)
 *
 * @brief       This function exposes an interface to store a scene.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
static uint8_t SRPC_storeScene(uint8_t *pBuf, uint32_t clientFd)
{
  uint16_t dstAddr;
  uint8_t addrMode;
  uint8_t endpoint;
  char *nameStr;
  uint8_t nameLen;
  uint16_t groupId;
  uint8_t sceneId;
  
  //printf("SRPC_storeScene++\n");
       
  //increment past SRPC header
  pBuf+=2;

  addrMode = (afAddrMode_t)*pBuf++;   
  dstAddr = BUILD_UINT16(pBuf[0], pBuf[1]);
  pBuf += Z_EXTADDR_LEN;
  endpoint = *pBuf++;
  // index past panId
  pBuf += 2;
  
  groupId = BUILD_UINT16(pBuf[0], pBuf[1]);
  pBuf += 2;
  
  nameLen = *pBuf++;
  nameStr = malloc(nameLen + 2);
  nameStr[0] = nameLen;
  int i;
  for(i = 0; i < nameLen; i++)
  {
    nameStr[i+1] = *pBuf++;
  } 
  nameStr[nameLen + 1] = '\0';
  
  sceneId = sceneListAddScene( nameStr, groupId );

//  printf("SRPC_storeScene++: name[%d] %s, group %d, scene %d \n", nameLen, nameStr + 1, groupId, sceneId);

  zbSocStoreScene(groupId, sceneId, dstAddr, endpoint, addrMode);
  SRPC_CallBack_addSceneRsp(groupId, sceneId, nameStr, clientFd);

  free(nameStr);
  
  //printf("SRPC_storeScene--\n");
  
  return 0;
}

/*********************************************************************
 * @fn          uint8_t SRPC_recallScene(uint8_t *pBuf, uint32_t clientFd)
 *
 * @brief       This function exposes an interface to recall a scene.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
static uint8_t SRPC_recallScene(uint8_t *pBuf, uint32_t clientFd)
{
  uint16_t dstAddr;
  uint8_t addrMode;
  uint8_t endpoint;
  char *nameStr;
  uint8_t nameLen;
  uint16_t groupId;
  uint8_t sceneId;
  
  //printf("SRPC_recallScene++\n");
       
  //increment past SRPC header
  pBuf+=2;

  addrMode = (afAddrMode_t)*pBuf++;   
  dstAddr = BUILD_UINT16(pBuf[0], pBuf[1]);
  pBuf += Z_EXTADDR_LEN;
  endpoint = *pBuf++;
  // index past panId
  pBuf += 2;
  
  groupId = BUILD_UINT16(pBuf[0], pBuf[1]);
  pBuf += 2;
    
  nameLen = *pBuf++;
  nameStr = malloc(nameLen + 2);
  nameStr[0] = nameLen;
  int i;
  for(i = 0; i < nameLen; i++)
  {
    nameStr[i+1] = *pBuf++;
  } 
  nameStr[nameLen+1] = '\0';
  
  sceneId = sceneListGetSceneId( nameStr, groupId );

//  printf("SRPC_recallScene++: name[%d] %s, group %d, scene %d \n", nameLen + 1, nameStr + 1, groupId, sceneId);

  zbSocRecallScene(groupId, sceneId, dstAddr, endpoint, addrMode);

  free(nameStr);
  
  //printf("SRPC_recallScene--\n");
  
  return 0;
}

/*********************************************************************
 * @fn          uint8_t SRPC_identifyDevice(uint8_t *pBuf, uint32_t clientFd)
 *
 * @brief       This function exposes an interface to make a device identify.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
static uint8_t SRPC_identifyDevice(uint8_t *pBuf, uint32_t clientFd)
{
  afAddrType_t dstAddr;
  uint16_t identifyTime;
  
  //printf("SRPC_identifyDevice++\n");
       
  //increment past SRPC header
  pBuf+=2;

  dstAddr.addrMode = (afAddrMode_t)*pBuf++;   
  if (dstAddr.addrMode == afAddr64Bit)
  {
    memcpy(dstAddr.addr.extAddr, pBuf, Z_EXTADDR_LEN);
  }
  else
  {
    dstAddr.addr.shortAddr = BUILD_UINT16(pBuf[0], pBuf[1]);
  }
  pBuf += Z_EXTADDR_LEN;

  dstAddr.endPoint = *pBuf++;
  dstAddr.panId = BUILD_UINT16(pBuf[0], pBuf[1]);
  pBuf += 2;
  
  identifyTime = BUILD_UINT16(pBuf[0], pBuf[1]);
  
  //TODO: implement zbSocIdentify
         
                            
  return 0;
}


/*********************************************************************
 * @fn          SRPC_changeDeviceName
 *
 * @brief       This function exposes an interface to set a bind devices.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
static uint8_t SRPC_changeDeviceName(uint8_t *pBuf, uint32_t clientFd)
{  
  uint16_t devNwkAddr;
  uint8_t devEndpoint;
  uint8_t nameLen;
  epInfo_t * epInfo;
   
  printf("RSPC_ZLL_changeDeviceName++\n");   
        
  //increment past SRPC header
  pBuf+=2;  
  
  // Src Address 
  devNwkAddr = BUILD_UINT16( pBuf[0], pBuf[1] );
  pBuf += 2;  

  devEndpoint = *pBuf++;
  
  nameLen = MIN(*pBuf++, MAX_SUPPORTED_DEVICE_NAME_LENGTH);

  epInfo = devListRemoveDeviceByNaEp(devNwkAddr, devEndpoint);
  if (epInfo != NULL)
  {
    strncpy(epInfo->deviceName, (char *)pBuf, nameLen);
	epInfo->deviceName[nameLen] = '\0';
	devListAddDevice(epInfo);
  }       
        
  return 0;
}


/*********************************************************************
 * @fn          uint8_t SRPC_removeDevice(uint8_t *pBuf, uint32_t clientFd)
 *
 * @brief       This function exposes an interface to remove a device.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
static uint8_t SRPC_removeDevice(uint8_t *pBuf, uint32_t clientFd)
{
  uint8_t devIEEE[8];
  uint8_t found = FALSE;
  uint8_t *pSrpcMessage;  
  epInfoExtended_t epInfoEx;

  printf("SRPC_removeDevice\n");   
        
  //increment past SRPC header
  pBuf+=2;  
  
  memcpy(devIEEE, pBuf, Z_EXTADDR_LEN);
  pBuf += Z_EXTADDR_LEN;       
    
  while ((epInfoEx.epInfo = devListRemoveDeviceByIeee(devIEEE)) != NULL)
  {
  	if (!found)
  	{
      found = TRUE;
  zbSocRemoveDevice(devIEEE); //make the device leave network (if not already gone)
  	}

  	epInfoEx.type = EP_INFO_TYPE_REMOVED;
	epInfoEx.prevNwkAddr = 0xFFFF;
                                    
    //Send epInfo
    pSrpcMessage = srpcParseEpInfo(&epInfoEx);  
    printf("SRPC_getDevices: %x:%x:%x:%x\n", epInfoEx.epInfo->nwkAddr, epInfoEx.epInfo->endpoint, epInfoEx.epInfo->profileID, epInfoEx.epInfo->deviceID);
    //Send SRPC
    srpcSendAll(pSrpcMessage);  
    free(pSrpcMessage); 
  }
  
  //todo: send a response to the originator: success / failure
  return 0;
} 

/*********************************************************************
 * @fn          SRPC_bindDevices
 *
 * @brief       This function exposes an interface to set a bind devices.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
uint8_t SRPC_bindDevices(uint8_t *pBuf, uint32_t clientFd)
{  
  uint16_t srcNwkAddr;
  uint8_t srcEndpoint;
  uint8 srcIEEE[8];
  uint8_t dstEndpoint;
  uint8 dstIEEE[8];
  uint16 clusterId;
   
  //printf("SRPC_bindDevices++\n");   
        
  //increment past SRPC header
  pBuf+=2;  
  
  /* Src Address */
  srcNwkAddr = BUILD_UINT16( pBuf[0], pBuf[1] );
  pBuf += 2;  

  srcEndpoint = *pBuf++;
  
  memcpy(srcIEEE, pBuf, Z_EXTADDR_LEN);
  pBuf += Z_EXTADDR_LEN;  

  dstEndpoint = *pBuf++;

  memcpy(dstIEEE, pBuf, Z_EXTADDR_LEN);
  pBuf += Z_EXTADDR_LEN;   
  
  clusterId = BUILD_UINT16(pBuf[0], pBuf[1]);
  pBuf += 2;     
  
  zbSocBind(srcNwkAddr, srcEndpoint, srcIEEE, dstEndpoint, dstIEEE, clusterId);
  
  return 0;
}

/*********************************************************************
 * @fn          SRPC_setDeviceState
 *
 * @brief       This function exposes an interface to set a devices on/off attribute.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
static uint8_t SRPC_setDeviceState(uint8_t *pBuf, uint32_t clientFd)
{
  uint8_t endpoint, addrMode;
  uint16_t dstAddr;
  bool state;
 
  //printf("SRPC_setDeviceState++\n");
       
  //increment past SRPC header
  pBuf+=2;

  addrMode = (afAddrMode_t)*pBuf++;   
  dstAddr = BUILD_UINT16(pBuf[0], pBuf[1]);
  pBuf += Z_EXTADDR_LEN;
  endpoint = *pBuf++;
  // index past panId
  pBuf += 2;
  
  state = (bool)*pBuf;
  
//  printf("SRPC_setDeviceState: dstAddr.addr.shortAddr=%x, endpoint=%x dstAddr.mode=%x, state=%x\n", dstAddr, endpoint, addrMode, state); 
    
  // Set light state on/off
  zbSocSetState(state, dstAddr, endpoint, addrMode);

  //printf("SRPC_setDeviceState--\n");
  
  return 0;
}

/*********************************************************************
 * @fn          SRPC_setDeviceLevel
 *
 * @brief       This function exposes an interface to set a devices level attribute.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
static uint8_t SRPC_setDeviceLevel(uint8_t *pBuf, uint32_t clientFd)
{
  uint8_t endpoint, addrMode;
  uint16_t dstAddr;
  uint8_t level; 
  uint16_t transitionTime;
 
  //printf("SRPC_setDeviceLevel++\n");   
      
  //increment past SRPC header
  pBuf+=2;

  addrMode = (afAddrMode_t)*pBuf++;   
  dstAddr = BUILD_UINT16(pBuf[0], pBuf[1]);
  pBuf += Z_EXTADDR_LEN;
  endpoint = *pBuf++;
  // index past panId
  pBuf += 2;
  
  level = *pBuf++;
  
  transitionTime = BUILD_UINT16(pBuf[0], pBuf[1]);  
  pBuf += 2;
  
//  printf("SRPC_setDeviceLevel: dstAddr.addr.shortAddr=%x ,level=%x, tr=%x \n", dstAddr, level, transitionTime); 
    
  zbSocSetLevel(level, transitionTime, dstAddr, endpoint, addrMode);
  
  //printf("SRPC_setDeviceLevel--\n");
  
  return 0;
}

/*********************************************************************
 * @fn          SRPC_setDeviceColor
 *
 * @brief       This function exposes an interface to set a devices hue and saturation attribute.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
static uint8_t SRPC_setDeviceColor(uint8_t *pBuf, uint32_t clientFd)
{
  uint8_t endpoint, addrMode;
  uint16_t dstAddr;
  uint8_t hue, saturation; 
  uint16_t transitionTime;

  //printf("SRPC_setDeviceColor++\n");   
      
  //increment past SRPC header
  pBuf+=2;

  addrMode = (afAddrMode_t)*pBuf++;   
  dstAddr = BUILD_UINT16(pBuf[0], pBuf[1]);
  pBuf += Z_EXTADDR_LEN;
  endpoint = *pBuf++;
  // index past panId
  pBuf += 2;
  
  hue = *pBuf++;
  
  saturation = *pBuf++;
  
  transitionTime = BUILD_UINT16(pBuf[0], pBuf[1]);  
  pBuf += 2;
  
//  printf("SRPC_setDeviceColor: dstAddr=%x ,hue=%x, saturation=%x, tr=%x \n", dstAddr, hue, saturation, transitionTime); 
    
  zbSocSetHueSat(hue, saturation, transitionTime, dstAddr, endpoint, addrMode);
  
  //printf("SRPC_setDeviceColor--\n");
  
  return 0;
}

/*********************************************************************
 * @fn          SRPC_getDeviceState
 *
 * @brief       This function exposes an interface to get a devices on/off attribute.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
static uint8_t SRPC_getDeviceState(uint8_t *pBuf, uint32_t clientFd)
{
  uint8_t endpoint, addrMode;
  uint16_t dstAddr;
 
  //printf("SRPC_getDeviceState++\n");
       
  //increment past SRPC header
  pBuf+=2;

  addrMode = (afAddrMode_t)*pBuf++;   
  dstAddr = BUILD_UINT16(pBuf[0], pBuf[1]);
  pBuf += Z_EXTADDR_LEN;
  endpoint = *pBuf++;
  // index past panId
  pBuf += 2;
  
//  printf("SRPC_getDeviceState: dstAddr.addr.shortAddr=%x, endpoint=%x dstAddr.mode=%x", dstAddr, endpoint, addrMode); 
    
  // Get light state on/off
  zbSocGetState(dstAddr, endpoint, addrMode);

  //printf("SRPC_getDeviceState--\n");
  
  return 0;
}

/*********************************************************************
 * @fn          SRPC_getDeviceLevel
 *
 * @brief       This function exposes an interface to get a devices level attribute.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
static uint8_t SRPC_getDeviceLevel(uint8_t *pBuf, uint32_t clientFd)
{
  uint8_t endpoint, addrMode;
  uint16_t dstAddr;
 
  //printf("SRPC_getDeviceLevel++\n");
       
  //increment past SRPC header
  pBuf+=2;

  addrMode = (afAddrMode_t)*pBuf++;   
  dstAddr = BUILD_UINT16(pBuf[0], pBuf[1]);
  pBuf += Z_EXTADDR_LEN;
  endpoint = *pBuf++;
  // index past panId
  pBuf += 2;
  
//  printf("SRPC_getDeviceLevel: dstAddr.addr.shortAddr=%x, endpoint=%x dstAddr.mode=%x\n", dstAddr, endpoint, addrMode); 
    
  // Get light level
  zbSocGetLevel(dstAddr, endpoint, addrMode);

  //printf("SRPC_getDeviceLevel--\n");
  
  return 0;
}

/*********************************************************************
 * @fn          SRPC_getDeviceHue
 *
 * @brief       This function exposes an interface to get a devices hue attribute.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
static uint8_t SRPC_getDeviceHue(uint8_t *pBuf, uint32_t clientFd)
{
  uint8_t endpoint, addrMode;
  uint16_t dstAddr;
 
  //printf("SRPC_getDeviceHue++\n");
       
  //increment past SRPC header
  pBuf+=2;

  addrMode = (afAddrMode_t)*pBuf++;   
  dstAddr = BUILD_UINT16(pBuf[0], pBuf[1]);
  pBuf += Z_EXTADDR_LEN;
  endpoint = *pBuf++;
  // index past panId
  pBuf += 2;
  
//  printf("SRPC_getDeviceHue: dstAddr.addr.shortAddr=%x, endpoint=%x dstAddr.mode=%x\n", dstAddr, endpoint, addrMode); 
    
  // Get light hue
  zbSocGetHue(dstAddr, endpoint, addrMode);

  //printf("SRPC_getDeviceHue--\n");
  
  return 0;
}

/*********************************************************************
 * @fn          SRPC_getDeviceSat
 *
 * @brief       This function exposes an interface to get a devices sat attribute.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
static uint8_t SRPC_getDeviceSat(uint8_t *pBuf, uint32_t clientFd)
{
  uint8_t endpoint, addrMode;
  uint16_t dstAddr;
 
  //printf("SRPC_getDeviceSat++\n");
       
  //increment past SRPC header
  pBuf+=2;

  addrMode = (afAddrMode_t)*pBuf++;   
  dstAddr = BUILD_UINT16(pBuf[0], pBuf[1]);
  pBuf += Z_EXTADDR_LEN;
  endpoint = *pBuf++;
  // index past panId
  pBuf += 2;
  
//  printf("SRPC_getDeviceSat: dstAddr.addr.shortAddr=%x, endpoint=%x dstAddr.mode=%x\n", dstAddr, endpoint, addrMode); 
    
  // Get light sat
  zbSocGetSat(dstAddr, endpoint, addrMode);

  //printf("SRPC_getDeviceSat--\n");
  
  return 0;
}

/*********************************************************************
 * @fn          SRPC_getDeviceTemp
 *
 * @brief       This function exposes an interface to get a devices temp attribute.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
static uint8_t SRPC_getDeviceTemp(uint8_t *pBuf, uint32_t clientFd)
{
  uint8_t endpoint, addrMode;
  uint16_t dstAddr;
 
  //printf("SRPC_getDeviceTemp++\n");
       
  //increment past SRPC header
  pBuf+=2;

  addrMode = (afAddrMode_t)*pBuf++;   
  dstAddr = BUILD_UINT16(pBuf[0], pBuf[1]);
  pBuf += Z_EXTADDR_LEN;
  endpoint = *pBuf++;
  // index past panId
  pBuf += 2;
  
  printf("SRPC_getDeviceTemp: dstAddr.addr.shortAddr=%x, endpoint=%x dstAddr.mode=%x\n", dstAddr, endpoint, addrMode); 
    
  // Get light sat
  zbSocGetTemp(dstAddr, endpoint, addrMode);

  //printf("SRPC_getDeviceTemp--\n");
  
  return 0;
}


/*********************************************************************
 * @fn          SRPC_getDeviceHumid
 *
 * @brief       This function exposes an interface to get a devices humidity attribute.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
static uint8_t SRPC_getDeviceHumid(uint8_t *pBuf, uint32_t clientFd)
{
  uint8_t endpoint, addrMode;
  uint16_t dstAddr;
 
  //printf("SRPC_getDeviceHumid++\n");
       
  //increment past SRPC header
  pBuf+=2;

  addrMode = (afAddrMode_t)*pBuf++;   
  dstAddr = BUILD_UINT16(pBuf[0], pBuf[1]);
  pBuf += Z_EXTADDR_LEN;
  endpoint = *pBuf++;
  // index past panId
  pBuf += 2;
  
  printf("SRPC_getDeviceHumid: dstAddr.addr.shortAddr=%x, endpoint=%x dstAddr.mode=%x\n", dstAddr, endpoint, addrMode); 
    
  // Get light sat
  zbSocGetHumid(dstAddr, endpoint, addrMode);

  //printf("SRPC_getDeviceHumid--\n");
  
  return 0;
}

/*********************************************************************
 * @fn          SRPC_getDevicePower
 *
 * @brief       This function exposes an interface to get a devices sat attribute.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
static uint8_t SRPC_getDevicePower(uint8_t *pBuf, uint32_t clientFd)
{
  uint8_t endpoint, addrMode;
  uint16_t dstAddr;
 
  printf("SRPC_getDevicePower++\n");
       
  //increment past SRPC header
  pBuf+=2;

  addrMode = (afAddrMode_t)*pBuf++;   
  dstAddr = BUILD_UINT16(pBuf[0], pBuf[1]);
  pBuf += Z_EXTADDR_LEN;
  endpoint = *pBuf++;
  // index past panId
  pBuf += 2;
  
  printf("SRPC_getDevicePower: dstAddr.addr.shortAddr=%x, endpoint=%x dstAddr.mode=%x\n", dstAddr, endpoint, addrMode); 
    
  // Get light sat
  zbSocReadPower(dstAddr, endpoint, addrMode);

  printf("SRPC_getDevicePower--\n");
  
  return 0;
}

/*********************************************************************
 * @fn          SRPC_installCertificate
 *
 * @brief       This function installs the certificate on the IHD.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
static uint8_t SRPC_installCertificate(uint8_t *pBuf, uint32_t clientFd)
{
  uint16_t filenameLength;
  uint8_t status;
  uint8_t force2reset;
  char * filename;

  pBuf+=2; //increment past SRPC header
  filenameLength = BUILD_UINT16(pBuf[0], pBuf[1]);
  pBuf+=2;
  force2reset = *pBuf++;
  //todo: verify that the actual received packet is long enough to contain the filename of the length reported.
  filename = (char *)pBuf;
  filename[filenameLength] = '\0'; //todo: make sure pBuf is long enough to contain this extra character
  
//  printf("SRPC_installCertificate: filename = %s, force2reset = %d\n", filename, force2reset); 
    
  cert_install_clientFd = clientFd;
//  printf("SRPC_installCertificate: cert_install_clientFd = %d\n", cert_install_clientFd);

  if ((status = zbSocInitiateCertInstall(filename, force2reset)) != SUCCESS)
  {
    SRPC_CallBack_certInstallResultInd(status);
    cert_install_clientFd = 0;
  }

//  printf("SRPC_installCertificate--\n");
  
  return 0;
}

/*********************************************************************
 * @fn          SRPC_getLastMessage
 *
 * @brief       This function sends GetLastMessage to the ESI.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
static uint8_t SRPC_getLastMessage(uint8_t *pBuf, uint32_t clientFd)
{
  uint8_t endpoint, addrMode;
  uint16_t dstAddr;
 
//  printf("SRPC_getLastMessage++\n");
       
  //increment past SRPC header
  pBuf+=2;

  addrMode = (afAddrMode_t)*pBuf++;   
  dstAddr = BUILD_UINT16(pBuf[0], pBuf[1]);
  pBuf += Z_EXTADDR_LEN;
  endpoint = *pBuf++;
  // index past panId
  pBuf += 2;
  
//  printf("SRPC_getLastMessage: dstAddr.addr.shortAddr=%x, endpoint=%x dstAddr.mode=%x\n", dstAddr, endpoint, addrMode); 
    
  // Get light sat
  zbSocGetLastMessage(dstAddr, endpoint, addrMode);

  get_last_message_clientFd = clientFd;
    
  printf("Requested the ZigBee SoC to send GetLastMessage.\n");
  
  return 0;
}

/*********************************************************************
 * @fn          SRPC_getCurrentPrice
 *
 * @brief       This function sends GetCurrentPrice to the ESI.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
static uint8_t SRPC_getCurrentPrice(uint8_t *pBuf, uint32_t clientFd)
{
  uint8_t endpoint, addrMode, rxOnIdle;
  uint16_t dstAddr;
 
//  printf("SRPC_getCurrentPrice++\n");
       
  //increment past SRPC header
  pBuf+=2;

  addrMode = (afAddrMode_t)*pBuf++;   
  dstAddr = BUILD_UINT16(pBuf[0], pBuf[1]);
  pBuf += Z_EXTADDR_LEN;
  endpoint = *pBuf++;
  // index past panId
  pBuf += 2;

  rxOnIdle = 1; // The server should know what the devide type of the IHD is. If a router, rxOnIdle can be 1, otherwise 0.
  
//  printf("SRPC_getCurrentPrice: dstAddr.addr.shortAddr=%x, endpoint=%x dstAddr.mode=%x\n", dstAddr, endpoint, addrMode); 
    
  // Get light sat
  zbSocGetCurrentPrice(rxOnIdle, dstAddr, endpoint, addrMode);

  get_current_price_clientFd = clientFd;
    
  printf("Requested the ZigBee SoC to send GetCurrentPrice.\n");
  
  return 0;
}

/*********************************************************************
 * @fn          SRPC_close
 *
 * @brief       This function exposes an interface to allow an upper layer to close the interface.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
uint8_t SRPC_close(uint8_t *pBuf, uint32_t clientFd)
{ 
  uint16_t magicNumber;
    
  //printf("SRPC_close++\n");

  //increment past SRPC header
  pBuf+=2;
  
  magicNumber = BUILD_UINT16(pBuf[0], pBuf[1]);
    
  if(magicNumber == CLOSE_AUTH_NUM)
  {
    //Close the application  
    socketSeverClose();
    //TODO: Need to create API's and close other fd's
    
    exit(EXIT_SUCCESS );
  }
  
  return 0; 
} 

/*********************************************************************
 * @fn          uint8_t SRPC_getGroups(uint8_t *pBuf, uint32_t clientFd)
 *
 * @brief       This function exposes an interface to get the group list.
 *
 * @param       pBuf - incomin messages
 *
 * @return      none
 */
static uint8_t SRPC_getGroups(uint8_t *pBuf, uint32_t clientFd)
{  
	uint32 context = 0;
  groupRecord_t *group = groupListGetNextGroup(&context);
  size_t nameLen;
  
  //printf("SRPC_getGroups++\n");
  
  while(group != NULL)
  {  
    uint8_t *pSrpcMessage, *pBuf;            
    //printf("SRPC_getGroups: group != null\n");
    
    //printf("SRPC_getGroups: malloc'ing %d bytes\n", (2 + (sizeof(uint16_t)) + ((uint8_t) (group->groupNameStr[0]))));
    
    //RpcMessage contains function ID param Data Len and param data
    //2 (SRPC header) + sizeof(uint16_t) (GroupId) + sizeof(uint8_t) (GroupName Len) + group->groupNameStr[0] (string)
    pSrpcMessage = malloc(2 + (sizeof(uint16_t)) + sizeof(uint8_t) + ((uint8_t) (group->name[0])));
      
     pBuf = pSrpcMessage;
    
    //Set func ID in RPCS buffer
    *pBuf++ = SRPC_GET_GROUP_RSP;
    //param size
    //sizeof(uint16_t) (GroupId) + sizeof(uint8_t) (SceneId) + sizeof(uint8_t) (GroupName Len) + group->groupNameStr[0] (string)
    *pBuf++ = (sizeof(uint16_t) + sizeof(uint8_t) + group->name[0]);
    
    *pBuf++ = group->id & 0xFF;
    *pBuf++ = (group->id & 0xFF00) >> 8;
    
	nameLen = strlen(group->name);
    *pBuf++ = nameLen;
	memcpy(pBuf, group->name, nameLen);
    *pBuf += nameLen;
          
    //printf("SRPC_CallBack_addGroupRsp: groupName[%d] %s\n", group->groupNameStr[0], &(group->groupNameStr[1]));
    //Send SRPC
    srpcSend(pSrpcMessage, clientFd);  
    free(pSrpcMessage);    
    //get next group (NULL if all done)
    group = groupListGetNextGroup(&context);
  }
  //printf("SRPC_getGroups--\n");
    
  return 0;
}

/***************************************************************************************************
 * @fn      SRPC_CallBack_addGroupRsp
 *
 * @brief   Sends the groupId to the client after a groupAdd
  *
 * @return  Status
 ***************************************************************************************************/
void SRPC_CallBack_addGroupRsp(uint16_t groupId, char *nameStr, uint32_t clientFd)
{
  uint8_t *pSrpcMessage, *pBuf;  
    
  //printf("SRPC_CallBack_addGroupRsp++\n");
  
  //printf("SRPC_CallBack_addGroupRsp: malloc'ing %d bytes\n", 2 + 3 + nameStr[0]);
  //RpcMessage contains function ID param Data Len and param data
  pSrpcMessage = malloc(2 + 3 + nameStr[0]);
  
  pBuf = pSrpcMessage;
  
  //Set func ID in RPCS buffer
  *pBuf++ = SRPC_ADD_GROUP_RSP;
  //param size
  *pBuf++ = 3 + nameStr[0];
  
  *pBuf++ = groupId & 0xFF;
  *pBuf++ = (groupId & 0xFF00) >> 8;
  
  *pBuf++ = nameStr[0];
  int i;
  for(i = 0; i < nameStr[0]; i++)
  {
    *pBuf++ = nameStr[i+1];
  }
        
  //printf("SRPC_CallBack_addGroupRsp: groupName[%d] %s\n", nameStr[0], nameStr);
  //Send SRPC
  srpcSend(pSrpcMessage, clientFd);  

  free(pSrpcMessage);
  
  //printf("SRPC_CallBack_addGroupRsp--\n");
                    
  return;              
}

/*********************************************************************
 * @fn          uint8_t SRPC_getScenes(uint8_t *pBuf, uint32_t clientFd)
 *
 * @brief       This function exposes an interface to get the scenes defined for a group.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
static uint8_t SRPC_getScenes(uint8_t *pBuf, uint32_t clientFd)
{  
  sceneListItem_t *scene = sceneListGetNextScene(NULL, 0);
  
  //printf("SRPC_getScenes++\n");
  
  while(scene != NULL)
  {  
    uint8_t *pSrpcMessage, *pBuf;            
    //printf("SRPC_getScenes: scene != null\n");
    
    //printf("SRPC_getScenes: malloc'ing %d bytes\n", (2 + (sizeof(uint16_t)) + (sizeof(uint8_t)) + (sizeof(uint8_t)) + ((uint8_t) (scene->sceneNameStr[0]))));
    
    //RpcMessage contains function ID param Data Len and param data
    //2 (SRPC header) + sizeof(uint16_t) (GroupId) + sizeof(uint8_t) (SceneId) + sizeof(uint8_t) (SceneName Len) + scene->sceneNameStr[0] (string)
    pSrpcMessage = malloc(2 + (sizeof(uint16_t)) + (sizeof(uint8_t)) + (sizeof(uint8_t)) + ((uint8_t) (scene->sceneNameStr[0])));
      
     pBuf = pSrpcMessage;
    
    //Set func ID in RPCS buffer
    *pBuf++ = SRPC_GET_SCENE_RSP;
    //param size
    //sizeof(uint16_t) (GroupId) + sizeof(uint8_t) (SceneId) + sizeof(uint8_t) (SceneName Len) + scene->sceneNameStr[0] (string)
    *pBuf++ = (sizeof(uint16_t) + (sizeof(uint8_t)) + (sizeof(uint8_t)) + scene->sceneNameStr[0]);
          
    *pBuf++ = scene->groupId & 0xFF;
    *pBuf++ = (scene->groupId & 0xFF00) >> 8;
    
    *pBuf++ = scene->sceneId;   
    
    *pBuf++ = scene->sceneNameStr[0];
    int i;
  for(i = 0; i < scene->sceneNameStr[0]; i++)
    {
      *pBuf++ = scene->sceneNameStr[i+1];
    }
          
    //printf("SRPC_getScenes: sceneName[%d] %s, groupId %x\n", scene->sceneNameStr[0], &(scene->sceneNameStr[1]), scene->groupId);
    //Send SRPC
    srpcSend(pSrpcMessage, clientFd);  
    free(pSrpcMessage);    
    //get next scene (NULL if all done)
    scene = sceneListGetNextScene(scene->sceneNameStr, scene->groupId);
  }
  //printf("SRPC_getScenes--\n");
    
  return 0;
}

/***************************************************************************************************
 * @fn      SRPC_CallBack_addSceneRsp
 *
 * @brief   Sends the sceneId to the client after a storeScene
  *
 * @return  Status
 ***************************************************************************************************/
void SRPC_CallBack_addSceneRsp(uint16_t groupId, uint8_t sceneId, char *nameStr, uint32_t clientFd)
{
  uint8_t *pSrpcMessage, *pBuf;  
    
  //printf("SRPC_CallBack_addSceneRsp++\n");
  
  //printf("SRPC_CallBack_addSceneRsp: malloc'ing %d bytes\n", 2 + 4 + nameStr[0]);
  //RpcMessage contains function ID param Data Len and param data
  pSrpcMessage = malloc(2 + 4 + nameStr[0]);
  
  pBuf = pSrpcMessage;
  
  //Set func ID in RPCS buffer
  *pBuf++ = SRPC_ADD_SCENE_RSP;
  //param size
  *pBuf++ = 4 + nameStr[0];
  
  *pBuf++ = groupId & 0xFF;
  *pBuf++ = (groupId & 0xFF00) >> 8;
  
  *pBuf++ = sceneId & 0xFF;  
  
  *pBuf++ = nameStr[0];
  int i;
  for(i = 0; i < nameStr[0]; i++)
  {
    *pBuf++ = nameStr[i+1];
  }
        
  //printf("SRPC_CallBack_addSceneRsp: groupName[%d] %s\n", nameStr[0], nameStr);
  //Send SRPC
  srpcSend(pSrpcMessage, clientFd);  

  free(pSrpcMessage);
  
  //printf("SRPC_CallBack_addSceneRsp--\n");
                    
  return;              
}

/***************************************************************************************************
 * @fn      SRPC_CallBack_getStateRsp
 *
 * @brief   Sends the get State Rsp to the client that sent a get state
  *
 * @return  Status
 ***************************************************************************************************/
void SRPC_CallBack_getStateRsp(uint8_t state, uint16_t srcAddr, uint8_t endpoint, uint32_t clientFd)
{
  uint8_t * pBuf;
  uint8_t pSrpcMessage[2 + 4];
    
  //printf("SRPC_CallBack_getStateRsp++\n");
  
  //RpcMessage contains function ID param Data Len and param data
  
  pBuf = pSrpcMessage;
  
  //Set func ID in RPCS buffer
  *pBuf++ = SRPC_GET_DEV_STATE_RSP;
  //param size
  *pBuf++ = 4;
    
  *pBuf++ = srcAddr & 0xFF;
  *pBuf++ = (srcAddr & 0xFF00) >> 8;
  *pBuf++ = endpoint; 
  *pBuf++ = state & 0xFF;  
        
  //printf("SRPC_CallBack_getStateRsp: state=%x\n", state);
  
  //Store the device that sent the request, for now send to all clients
  srpcSendAll(pSrpcMessage);  

  //printf("SRPC_CallBack_addSceneRsp--\n");
                    
  return;              
}

/***************************************************************************************************
 * @fn      SRPC_CallBack_getLevelRsp
 *
 * @brief   Sends the get Level Rsp to the client that sent a get level
  *
 * @return  Status
 ***************************************************************************************************/
void SRPC_CallBack_getLevelRsp(uint8_t level, uint16_t srcAddr, uint8_t endpoint, uint32_t clientFd)
{
  uint8_t * pBuf;  
  uint8_t pSrpcMessage[2 + 4];
    
  //printf("SRPC_CallBack_getLevelRsp++\n");
  
  //RpcMessage contains function ID param Data Len and param data
  
  pBuf = pSrpcMessage;
  
  //Set func ID in RPCS buffer
  *pBuf++ = SRPC_GET_DEV_LEVEL_RSP;
  //param size
  *pBuf++ = 4;
  
  *pBuf++ = srcAddr & 0xFF;
  *pBuf++ = (srcAddr & 0xFF00) >> 8;
  *pBuf++ = endpoint;   
  *pBuf++ = level & 0xFF;  
        
  //printf("SRPC_CallBack_getLevelRsp: level=%x\n", level);
  
  //Store the device that sent the request, for now send to all clients
  srpcSendAll(pSrpcMessage);  

  //printf("SRPC_CallBack_getLevelRsp--\n");
                    
  return;              
}

/***************************************************************************************************
 * @fn      SRPC_CallBack_getHueRsp
 *
 * @brief   Sends the get Hue Rsp to the client that sent a get hue
  *
 * @return  Status
 ***************************************************************************************************/
void SRPC_CallBack_getHueRsp(uint8_t hue, uint16_t srcAddr, uint8_t endpoint, uint32_t clientFd)
{
  uint8_t * pBuf;  
  uint8_t pSrpcMessage[2 + 4];
    
  //printf("SRPC_CallBack_getLevelRsp++\n");
  
  //RpcMessage contains function ID param Data Len and param data
  
  pBuf = pSrpcMessage;
  
  //Set func ID in RPCS buffer
  *pBuf++ = SRPC_GET_DEV_HUE_RSP;
  //param size
  *pBuf++ = 4;
    
  *pBuf++ = srcAddr & 0xFF;
  *pBuf++ = (srcAddr & 0xFF00) >> 8;
  *pBuf++ = endpoint;   
  *pBuf++ = hue & 0xFF;  
        
  //printf("SRPC_CallBack_getHueRsp: hue=%x\n", hue);
  
  //Store the device that sent the request, for now send to all clients
  srpcSendAll(pSrpcMessage);  

  //printf("SRPC_CallBack_getHueRsp--\n");
                    
  return;              
}

/***************************************************************************************************
 * @fn      SRPC_CallBack_getSatRsp
 *
 * @brief   Sends the get Sat Rsp to the client that sent a get sat
  *
 * @return  Status
 ***************************************************************************************************/
void SRPC_CallBack_getSatRsp(uint8_t sat, uint16_t srcAddr, uint8_t endpoint, uint32_t clientFd)
{
  uint8_t * pBuf;  
  uint8_t pSrpcMessage[2 + 4];
    
  //printf("SRPC_CallBack_getSatRsp++\n");
  
  //RpcMessage contains function ID param Data Len and param data
  
  pBuf = pSrpcMessage;
  
  //Set func ID in RPCS buffer
  *pBuf++ = SRPC_GET_DEV_SAT_RSP;
  //param size
  *pBuf++ = 4;
  
  *pBuf++ = srcAddr & 0xFF;
  *pBuf++ = (srcAddr & 0xFF00) >> 8;
  *pBuf++ = endpoint;   
  *pBuf++ = sat & 0xFF;  
        
  //printf("SRPC_CallBack_getSatRsp: sat=%x\n", sat);
  
  //Store the device that sent the request, for now send to all clients
  srpcSendAll(pSrpcMessage);  

  //printf("SRPC_CallBack_getSatRsp--\n");
                    
  return;              
}

/***************************************************************************************************
 * @fn      SRPC_CallBack_getTempRsp
 *
 * @brief   Sends the get Temp Rsp to the client that sent a get sat
  *
 * @return  Status
 ***************************************************************************************************/
void SRPC_CallBack_getTempRsp(uint16_t temp, uint16_t srcAddr, uint8_t endpoint, uint32_t clientFd)
{
  uint8_t * pBuf;  
  uint8_t pSrpcMessage[2 + 5];
    
  //printf("SRPC_CallBack_getTempRsp++\n");
  
  //RpcMessage contains function ID param Data Len and param data
  
  pBuf = pSrpcMessage;
  
  //Set func ID in RPCS buffer
  *pBuf++ = SRPC_TEMP_READING;
  //param size
  *pBuf++ = 5;
  
  *pBuf++ = srcAddr & 0xFF;
  *pBuf++ = (srcAddr & 0xFF00) >> 8;
  *pBuf++ = endpoint;   
  *pBuf++ = temp & 0xFF;
  *pBuf++ = (temp & 0xFF00) >> 8;   
        
  //printf("SRPC_CallBack_getTempRsp: temp=%x\n", temp);
  
  //Store the device that sent the request, for now send to all clients
  srpcSendAll(pSrpcMessage);  

  //printf("SRPC_CallBack_getSatRsp--\n");
                    
  return;              
}

/***************************************************************************************************
 * @fn      SRPC_CallBack_getHumidRsp
 *
 * @brief   Sends the get Humid Rsp to the client that sent a get sat
  *
 * @return  Status
 ***************************************************************************************************/
void SRPC_CallBack_getHumidRsp(uint16_t humid, uint16_t srcAddr, uint8_t endpoint, uint32_t clientFd)
{
  uint8_t * pBuf;  
  uint8_t pSrpcMessage[2 + 5];
    
  //printf("SRPC_CallBack_getHumidRsp++\n");
  
  //RpcMessage contains function ID param Data Len and param data
  
  pBuf = pSrpcMessage;
  
  //Set func ID in RPCS buffer
  *pBuf++ = SRPC_HUMID_READING;
  //param size
  *pBuf++ = 5;
  
  *pBuf++ = srcAddr & 0xFF;
  *pBuf++ = (srcAddr & 0xFF00) >> 8;
  *pBuf++ = endpoint;   
  *pBuf++ = humid & 0xFF;
  *pBuf++ = (humid & 0xFF00) >> 8;   
        
  //printf("SRPC_CallBack_getHumidRsp: temp=%x\n", humid);
  
  //Store the device that sent the request, for now send to all clients
  srpcSendAll(pSrpcMessage);  

  //printf("SRPC_CallBack_getHumidRsp--\n");
                    
  return;              
}

/***************************************************************************************************
 * @fn      SRPC_CallBack_getPowerRsp
 *
 * @brief   Sends the get Power Rsp to the client that sent a get sat
  *
 * @return  Status
 ***************************************************************************************************/
void SRPC_CallBack_readPowerRsp(uint32_t power, uint16_t srcAddr, uint8_t endpoint, uint32_t clientFd)
{
  uint8_t * pBuf;  
  uint8_t pSrpcMessage[2 + 7];
    
  //printf("SRPC_CallBack_getPowerRsp++\n");
  
  //RpcMessage contains function ID param Data Len and param data
  
  pBuf = pSrpcMessage;
  
  //Set func ID in RPCS buffer
  *pBuf++ = SRPC_READ_POWER_RSP;
  //param size
  *pBuf++ = 7;
  
  *pBuf++ = srcAddr & 0xFF;
  *pBuf++ = (srcAddr & 0xFF00) >> 8;
  *pBuf++ = endpoint;   
  *pBuf++ = power & 0xFF;
  *pBuf++ = (power & 0xFF00) >> 8;   
  *pBuf++ = (power & 0xFF0000) >> 16;   
  *pBuf++ = (power & 0xFF000000) >> 24;   

        
  //printf("SRPC_CallBack_getPowerRsp: power=%x\n", power);
  
  //Store the device that sent the request, for now send to all clients
  srpcSendAll(pSrpcMessage);  

  //printf("SRPC_CallBack_getPowerRsp--\n");
                    
  return;              
}

void SRPC_CallBack_bootloadingDone(uint8_t result)
{
	SRPC_CallBack_loadImageRsp(result, bootloader_initiator_clientFd);
}


void SRPC_CallBack_loadImageProgress(uint8_t phase, uint32_t location)
{
	SRPC_CallBack_SendProgressReport(phase, location, bootloader_initiator_clientFd);
}

/***************************************************************************************************
 * @fn      SRPC_CallBack_zoneSateInd
 *
 * @brief   Sends the get Power Rsp to the client that sent a get sat
  *
 * @return  Status
 ***************************************************************************************************/
void SRPC_CallBack_zoneSateInd(uint32_t zoneState, uint16_t srcAddr, uint8_t endpoint, uint32_t clientFd)
{
  uint8_t * pBuf;  
  uint8_t pSrpcMessage[2 + 7];
    
  //printf("SRPC_CallBack_zoneSateInd++\n");
  
  //RpcMessage contains function ID param Data Len and param data
  
  pBuf = pSrpcMessage;
  
  //Set func ID in RPCS buffer
  *pBuf++ = SRPC_ZONESTATE_CHANGE;
  //param size
  *pBuf++ = 7;
  
  *pBuf++ = srcAddr & 0xFF;
  *pBuf++ = (srcAddr & 0xFF00) >> 8;
  *pBuf++ = endpoint;   
  *pBuf++ = zoneState & 0xFF;
  *pBuf++ = (zoneState & 0xFF00) >> 8;   
  *pBuf++ = (zoneState & 0xFF0000) >> 16;   
  *pBuf++ = (zoneState & 0xFF000000) >> 24;   

        
  printf("SRPC_CallBack_zoneSateInd: zoneState=%x\n", zoneState);
  
  //Store the device that sent the request, for now send to all clients
  srpcSendAll(pSrpcMessage);  

  //printf("SRPC_CallBack_zoneSateInd--\n");
                    
  return;              
}

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

/*********************************************************************
 * @fn          SRPC_getDevices
 *
 * @brief       This function exposes an interface to allow an upper layer to start device discovery.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
static uint8_t SRPC_getDevices(uint8_t *pBuf, uint32_t clientFd)
{ 
  epInfoExtended_t epInfoEx;
  uint32_t context = 0;
  uint8_t *pSrpcMessage;

  //printf("SRPC_getDevices++ \n");

  while((epInfoEx.epInfo = devListGetNextDev(&context)) != NULL)
  {  
  	epInfoEx.type = EP_INFO_TYPE_EXISTING;
	epInfoEx.prevNwkAddr = 0xFFFF;
	epInfoEx.epInfo->flags = MT_NEW_DEVICE_FLAGS_NONE;
  
    //Send epInfo
    pSrpcMessage = srpcParseEpInfo(&epInfoEx);  
    //printf("SRPC_getDevices: %x:%x:%x:%x\n", epInfo->nwkAddr, epInfo->endpoint, epInfo->profileID, epInfo->deviceID);
    //Send SRPC
    srpcSend(pSrpcMessage, clientFd);  
    free(pSrpcMessage); 
  
    usleep(1000);
    
    //get next device (NULL if all done)
//printf("--?--> 0x%04X, 0x%02X", (uint32_t)epInfoEx.epInfo->nwkAddr, (uint32_t)epInfoEx.epInfo->endpoint);
//printf(" --!--> 0x%08X\n", (uint32_t)epInfoEx.epInfo);
  }
  
  return 0;  
}

/*********************************************************************
 * @fn          SRPC_notSupported
 *
 * @brief       This function is called for unsupported commands.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
static uint8_t SRPC_notSupported(uint8_t *pBuf, uint32_t clientFd)
{   
  return 0;  
}

/***************************************************************************************************
 * @fn      SRPC_CallBack_certInstallResultInd
 *
 * @brief   Sends Certificate Installation Result indication to the client
 *
 * @return  Status
 ***************************************************************************************************/
void SRPC_CallBack_certInstallResultInd(uint8_t result)
{
  uint8_t * pBuf;  
  uint8_t pSrpcMessage[2 + 1];
    
//  printf("SRPC_CallBack_certInstallResultInd: result = %d\n", result);
  
  //RpcMessage contains function ID param Data Len and param data
  
  pBuf = pSrpcMessage;
  
  //Set func ID in RPCS buffer
  *pBuf++ = SRPC_CERT_INSTALL_RESULT_IND;
  //param size
  *pBuf++ = 1;
  
  *pBuf = result;
        
  srpcSend(pSrpcMessage, cert_install_clientFd);
}

/***************************************************************************************************
 * @fn      SRPC_CallBack_keyEstablishmentStateInd
 *
 * @brief   Sends Key Establishment State indication to the client
 *
 * @return  Status
 ***************************************************************************************************/
void SRPC_CallBack_keyEstablishmentStateInd(uint8_t state)
{
  uint8_t * pBuf;  
  uint8_t pSrpcMessage[2 + 1];
    
  printf("SRPC_CallBack_keyEstablishmentStateInd: state = %d\n", state);
  
  //RpcMessage contains function ID param Data Len and param data
  
  pBuf = pSrpcMessage;
  
  //Set func ID in RPCS buffer
  *pBuf++ = SRPC_KEY_ESTABLISHMENT_STATE_IND;
  //param size
  *pBuf++ = 1;
  
  *pBuf = state;
        
  srpcSendAll(pSrpcMessage);  
}

/***************************************************************************************************
 * @fn      SRPC_CallBack_displayMessageInd
 *
 * @brief   Sends DisplayMessage indication to the client that sent GetLastMessage
 *          This can be sent unsolicitedly as well
 *
 * @return  Status
 ***************************************************************************************************/
void SRPC_CallBack_displayMessageInd(uint8_t *zclPayload, uint8_t len)
{
  uint8_t *pSrpcMessage, *pBuf;  
    
//  printf("SRPC_CallBack_displayMessageInd: len = %d\n", len);
  
  //RpcMessage contains function ID param Data Len and param data
  pSrpcMessage = malloc(2 + len);
  
  pBuf = pSrpcMessage;
  
  //Set func ID in RPCS buffer
  *pBuf++ = SRPC_DISPLAY_MESSAGE_IND;
  //param size
  *pBuf++ = len;
  
  memcpy(pBuf, zclPayload, len);
        
  if (get_last_message_clientFd != 0)
  {
    srpcSend(pSrpcMessage, get_last_message_clientFd);
    get_last_message_clientFd = 0;
  }
  else
  {
    srpcSendAll(pSrpcMessage);
  }

  free(pSrpcMessage);

  printf("Sent SRPC_DISPLAY_MESSAGE_IND to the client.\n\n");
}

/***************************************************************************************************
 * @fn      SRPC_CallBack_publishPriceInd
 *
 * @brief   Sends PublishPrice indication to the client that sent GetCurrentPrice
 *          This can be sent unsolicitedly as well
 *
 * @return  Status
 ***************************************************************************************************/
void SRPC_CallBack_publishPriceInd(uint8_t *zclPayload, uint8_t len)
{
  uint8_t *pSrpcMessage, *pBuf;  
    
//  printf("SRPC_CallBack_publishPriceInd: len = %d\n", len);
  
  //RpcMessage contains function ID param Data Len and param data
  pSrpcMessage = malloc(2 + len);
  
  pBuf = pSrpcMessage;
  
  //Set func ID in RPCS buffer
  *pBuf++ = SRPC_PUBLISH_PRICE_IND;
  //param size
  *pBuf++ = len;
  
  memcpy(pBuf, zclPayload, len);
        
  if (get_current_price_clientFd != 0)
  {
    srpcSend(pSrpcMessage, get_current_price_clientFd);
    get_current_price_clientFd = 0;
  }
  else
  {
    srpcSendAll(pSrpcMessage);
  }
  
  free(pSrpcMessage);

  printf("Sent SRPC_PUBLISH_PRICE_IND to the client.\n\n");
}

/***************************************************************************************************
 * @fn      SRPC_Init
 *
 * @brief   initialises the RPC interface and waitsfor a client to connect.
 * @param   
 *
 * @return  Status
 ***************************************************************************************************/      
void SRPC_Init( void )
{
  if(socketSeverInit(SRPC_TCP_PORT) == -1)
  {
    //exit if the server does not start
    exit(-1);
  }
  
  serverSocketConfig(SRPC_RxCB, SRPC_ConnectCB);  
}

/*********************************************************************
 * @fn          RSPC_SendEpInfo
 *
 * @brief       This function exposes an interface to allow an upper layer to start send an ep indo to all devices.
 *
 * @param       epInfo - pointer to the epInfo to be sent
 *
 * @return      afStatus_t
 */
uint8_t RSPC_SendEpInfo(epInfoExtended_t *epInfoEx)
{ 
  uint8_t *pSrpcMessage = srpcParseEpInfo(epInfoEx);  
  
  printf("RSPC_SendEpInfo++ %x:%x:%x:%x\n", epInfoEx->epInfo->nwkAddr, epInfoEx->epInfo->endpoint, epInfoEx->epInfo->profileID, epInfoEx->epInfo->deviceID);
    
  //Send SRPC
  srpcSendAll(pSrpcMessage);  
  free(pSrpcMessage); 
  printf("RSPC_SendEpInfo--\n");
  
  return 0;  
}

/***************************************************************************************************
 * @fn      SRPC_ConnectCB
 *
 * @brief   Callback for connecting SRPC clients.
 *
 * @return  Status
 ***************************************************************************************************/
void SRPC_ConnectCB( int clientFd )
{
  //printf("SRPC_ConnectCB++ \n");
/*
  epInfo = devListGetNextDev(0xFFFF, 0);
  
  while(epInfo)
  {  
    printf("SRPC_ConnectCB: send epInfo\n");  
    //Send device Annce
    uint8_t *pSrpcMessage = srpcParseEpInfo(epInfo);  
    //Send SRPC
    srpcSend(pSrpcMessage, clientFd);  
    free(pSrpcMessage); 
  
    usleep(1000);
    
    //get next device (NULL if all done)
    epInfo = devListGetNextDev(epInfo->nwkAddr, epInfo->endpoint);
  }
*/     
  //printf("SRPC_ConnectCB--\n");
}
  
/***************************************************************************************************
 * @fn      SRPC_RxCB
 *
 * @brief   Callback for Rx'ing SRPC messages.
 *
 * @return  Status
 ***************************************************************************************************/
void SRPC_RxCB( int clientFd )
{
  char buffer[256]; 
  int byteToRead;
  int byteRead;
  int rtn;
      
  //printf("SRPC_RxCB++[%x]\n", clientFd);
    
  rtn = ioctl(clientFd, FIONREAD, &byteToRead);
  
  if(rtn !=0)
  {
    printf("SRPC_RxCB: Socket error\n");
  }      

  while(byteToRead)
  {
    bzero(buffer,256);
    byteRead = 0;
    //Get the CMD-ID and Len
    byteRead += read(clientFd,buffer,2);   
    //Get the rest of the message
    //printf("SRPC: reading %x byte message\n",buffer[SRPC_MSG_LEN]);
    byteRead += read(clientFd,&buffer[2],buffer[SRPC_MSG_LEN]);
    byteToRead -= byteRead;
    if (byteRead < 0) error("SRPC ERROR: error reading from socket\n");
    if (byteRead < buffer[SRPC_MSG_LEN]) error("SRPC ERROR: full message not read\n");         
    //printf("Read the message[%x]\n",byteRead);
    SRPC_ProcessIncoming((uint8_t*)buffer, clientFd);
  }
  
  //printf("SRPC_RxCB--\n");
    
  return; 
}



/***************************************************************************************************
 * @fn      Closes the TCP port
 *
 * @brief   Send a message over SRPC.
 *
 * @return  Status
 ***************************************************************************************************/
void SRPC_Close(void)
{
  socketSeverClose();    
}

