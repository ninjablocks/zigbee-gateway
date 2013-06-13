/*
 * zbSocCmd.c
 *
 * This module contains the API for the ZigBee SoC Host Interface.
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


/*********************************************************************
 * INCLUDES
 */
#include <termios.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "zbSocCmd.h"


/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */
#define MAX_CONSOLE_CMD_LEN 128

/************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */
static uint16_t savedNwkAddr;    
static uint8_t savedAddrMode;    
static uint8_t savedEp;    
static uint8_t savedValue;    
static uint16_t savedTransitionTime; 
static uint16_t savedDeviceId;
static  uint16_t savedProfileId;  

/*********************************************************************
 * LOCAL VARIABLES
 */
zbSocCallbacks_t zbSocCb;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
void commandUsage( void );
void processConsoleCommand( void );
void getConsoleCommandParams(char* cmdBuff, uint16_t *nwkAddr, uint16_t *deviceId, uint16_t *profileId, uint8_t *addrMode, uint8_t *ep, uint8_t *value, uint16_t *transitionTime);
uint32_t getParam( char *cmdBuff, char *paramId, uint32_t *paramInt);

/*********************************************************************
 * API FUNCTIONS
 */
 
/*********************************************************************
 * @fn      zbSocOpen
 *
 * @brief   opens the serial port to the CC253x.
 *
 * @param   devicePath - path to the UART device
 *
 * @return  status
 */
int32_t zbSocOpen( char *devicePath  )
{
  //do nothing, TH is going to read command from stdin to simulate a ZigBee device  
  return 0;
}

void zbSocClose( void )
{
  return;
}

/*********************************************************************
 * @fn      zbSocOpen
 *
 * @brief   opens the serial port to the CC253x.
 *
 * @param   devicePath - path to the UART device
 *
 * @return  status
 */
void zbSocRegisterCallbacks( zbSocCallbacks_t zbSocCallbacks)
{
  //copy the callback function pointers
  memcpy(&zbSocCb, &zbSocCallbacks, sizeof(zbSocCallbacks_t));
  return;  
}


/*********************************************************************
 * @fn      zbSocTouchLink
 *
 * @brief   Send the touchLink command to the CC253x.
 *
 * @param   none
 *
 * @return  none
 */
void zbSocTouchLink(void)
{  
}


/*********************************************************************
 * @fn      zbSocResetToFn
 *
 * @brief   Send the reset to factory new command to the CC253x.
 *
 * @param   none
 *
 * @return  none
 */
void zbSocResetToFn(void)
{  
}

/*********************************************************************
 * @fn      zbSocSendResetToFn
 *
 * @brief   Send the reset to factory new command to a ZigBee device.
 *
 * @param   none
 *
 * @return  none
 */
void zbSocSendResetToFn(void)
{  
}

/*********************************************************************
 * @fn      zbSocSetState
 *
 * @brief   Send the on/off command to a ZigBee light.
 *
 * @param   state - 0: Off, 1: On.
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be controled.
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast.
 *
 * @return  none
 */
void zbSocSetState(uint8_t state, uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode)
{
}

/*********************************************************************
 * @fn      zbSocSetLevel
 *
 * @brief   Send the level command to a ZigBee light.
 *
 * @param   level - 0-128 = 0-100%
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be controled.
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast.
 *
 * @return  none
 */
void zbSocSetLevel(uint8_t level, uint16_t time, uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode)
{
}

/*********************************************************************
 * @fn      zbSocSetHue
 *
 * @brief   Send the hue command to a ZigBee light.
 *
 * @param   hue - 0-128 represent the 360Deg hue color wheel : 0=red, 42=blue, 85=green  
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be controled.
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast.
 *
 * @return  none
 */
void zbSocSetHue(uint8_t hue, uint16_t time, uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode)
{
}

/*********************************************************************
 * @fn      zbSocSetSat
 *
 * @brief   Send the satuartion command to a ZigBee light.
 *
 * @param   sat - 0-128 : 0=white, 128: fully saturated color  
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be controled.
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast.
 *
 * @return  none
 */
void zbSocSetSat(uint8_t sat, uint16_t time, uint16_t dstAddr, uint8_t  endpoint, uint8_t addrMode)
{
}

/*********************************************************************
 * @fn      zbSocSetHueSat
 *
 * @brief   Send the hue and satuartion command to a ZigBee light.
 *
 * @param   hue - 0-128 represent the 360Deg hue color wheel : 0=red, 42=blue, 85=green  
 * @param   sat - 0-128 : 0=white, 128: fully saturated color  
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be controled.
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast.
 *
 * @return  none
 */
void zbSocSetHueSat(uint8_t hue, uint8_t sat, uint16_t time, uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode)
{
}

/*********************************************************************
 * @fn      zbSocGetState
 *
 * @brief   Send the get state command to a ZigBee light.
 *
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be sent the command.
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast.
 *
 * @return  none
 */
void zbSocGetState(uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode)
{  	  
} 
 
/*********************************************************************
 * @fn      zbSocGetLevel
 *
 * @brief   Send the get level command to a ZigBee light.
 *
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be sent the command.
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast.
 *
 * @return  none
 */
void zbSocGetLevel(uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode)
{
} 

/*********************************************************************
 * @fn      zbSocGetHue
 *
 * @brief   Send the get hue command to a ZigBee light.
 *
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be sent the command.
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast.
 *
 * @return  none
 */
void zbSocGetHue(uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode)
{
} 

/*********************************************************************
 * @fn      zbSocGetSat
 *
 * @brief   Send the get saturation command to a ZigBee light.
 *
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be sent the command.
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast.
 *
 * @return  none
 */
void zbSocGetSat(uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode)
{
} 

/*************************************************************************************************
 * @fn      zbSocProcessRpc()
 *
 * @brief   read and process the RPC from the ZigBee controller
 *
 * @param   none
 *
 * @return  length of current Rx Buffer
 *************************************************************************************************/
void zbSocProcessRpc (void)
{      
  char cmdBuff[MAX_CONSOLE_CMD_LEN];
  uint32_t bytesRead;      
  uint16_t nwkAddr;
  uint16_t deviceId;
  uint16_t profileId;    
  uint8_t addrMode;
  uint8_t endpoint;
  uint8_t value;
  uint16_t transitionTime;
    
  //read stdin
  bytesRead = read(0, cmdBuff, (MAX_CONSOLE_CMD_LEN-1));    
  cmdBuff[bytesRead] = '\0';  
  
  getConsoleCommandParams(cmdBuff, &nwkAddr, &deviceId, &profileId, &addrMode, &endpoint, &value, &transitionTime);   
     
  if((strstr(cmdBuff, "touchlink")) != 0)
  {      
    printf("processRpcSysAppTlInd: %x:%x:%x:%x\n",  nwkAddr, endpoint, deviceId, profileId);
    if(zbSocCb.pfnTlIndicationCb)
    {
      epInfo_t epInfo;
      epInfo.nwkAddr = nwkAddr;
      epInfo.endpoint = endpoint;
      epInfo.deviceID = deviceId;
      epInfo.profileID = profileId;      
      zbSocCb.pfnTlIndicationCb(&epInfo);
    }    
  }    
  return; 
}

void getConsoleCommandParams(char* cmdBuff, uint16_t *nwkAddr, uint16_t *deviceId, uint16_t *profileId, uint8_t *addrMode, uint8_t *ep, uint8_t *value, uint16_t *transitionTime)
{ 
  //set some default values
  uint32_t tmpInt;     
  
  if( getParam( cmdBuff, "-n", &tmpInt) )
  {
    savedNwkAddr = (uint16_t) tmpInt;
  }
  if( getParam( cmdBuff, "-d", &tmpInt) )
  {
    savedDeviceId = (uint16_t) tmpInt;
  }
  if( getParam( cmdBuff, "-p", &tmpInt) )
  {
    savedProfileId = (uint16_t) tmpInt;
  }    
  if( getParam( cmdBuff, "-m", &tmpInt) )
  {
    savedAddrMode = (uint8_t) tmpInt;
  }
  if( getParam( cmdBuff, "-e", &tmpInt) )
  {
    savedEp = (uint8_t) tmpInt;
  }
  if( getParam( cmdBuff, "-v", &tmpInt) )
  {
    savedValue = (uint8_t) tmpInt;
  }
  if( getParam( cmdBuff, "-t", &tmpInt) )
  {
    savedTransitionTime = (uint16_t) tmpInt;
  }        
        
  *nwkAddr = savedNwkAddr;    
  *addrMode = savedAddrMode;    
  *ep = savedEp;    
  *value = savedValue;    
  *transitionTime = savedTransitionTime;
  *deviceId = savedDeviceId;
  *profileId = savedProfileId;

  return;         
}

uint32_t getParam( char *cmdBuff, char *paramId, uint32_t *paramInt)
{
  char* paramStrStart;
  char* paramStrEnd;
  //0x1234+null termination
  char paramStr[7];    
  uint32_t rtn = 0;
  
  memset(paramStr, 0, sizeof(paramStr));  
  paramStrStart = strstr(cmdBuff, paramId);
  
  if( paramStrStart )
  {
    //index past the param idenentifier "-?"
    paramStrStart+=2;
    //find the the end of the param text
    paramStrEnd = strstr(paramStrStart, " ");
    if( paramStrEnd )
    {
      if(paramStrEnd-paramStrStart > (sizeof(paramStr)-1))
      {
        //we are not on the last param, but the param str is too long
        strncpy( paramStr, paramStrStart, (sizeof(paramStr)-1));
        paramStr[sizeof(paramStr)-1] = '\0';  
      }
      else
      {
        //we are not on the last param so use the " " as the delimiter
        strncpy( paramStr, paramStrStart, paramStrEnd-paramStrStart);
        paramStr[paramStrEnd-paramStrStart] = '\0'; 
      }
    }
    
    else
    {
      //we are on the last param so use the just go the the end of the string 
      //(which will be null terminate). But make sure that it is not bigger
      //than our paramStr buffer. 
      if(strlen(paramStrStart) > (sizeof(paramStr)-1))
      {
        //command was entered wrong and paramStrStart will over flow the 
        //paramStr buffer.
        strncpy( paramStr, paramStrStart, (sizeof(paramStr)-1));
        paramStr[sizeof(paramStr)-1] = '\0';
      }
      else
      {
        //Param is the correct size so just copy it.
        strcpy( paramStr, paramStrStart);
        paramStr[strlen(paramStrStart)-1] = '\0';
      }
    }
    
    //was the param in hex or dec?
    if(strstr(paramStr, "0x"))   
    {
      //convert the hex value to an int.
      sscanf(paramStr, "0x%x", paramInt);
    }
    else
    {
      //assume that it ust be dec and convert to int.
      sscanf(paramStr, "%d", paramInt);
    }         
    
    //paramInt was set
    rtn = 1;
         
  }  
    
  return rtn;
}

