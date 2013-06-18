/**************************************************************************************************
 * Filename:       interface_devicelist.c
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

/*********************************************************************
 * INCLUDES
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "interface_srpcserver.h"
#include "interface_devicelist.h"

/*********************************************************************
 * DEFINES
 */


/*********************************************************************
 * TYPEDEFS
 */
 
typedef struct
{
  void   *next;
  epInfo_t epInfo;  
}deviceRecord_t;

deviceRecord_t *deviceRecordHead = NULL;

/*********************************************************************
 * LOCAL FUNCTION PROTOTYPES
 */ 
static deviceRecord_t* createDeviceRec( epInfo_t epInfo );
static deviceRecord_t* findDeviceRec( uint16_t nwkAddr, uint8_t endpoint);
static char* findDeviceInFileString( uint16_t nwkAddr, uint8_t endpoint, char* fileBuf, uint32_t bufLen );
static void removeDeviceFromFile( uint16_t nwkAddr, uint8_t endpoint );
static void writeDeviceToFile( deviceRecord_t *device );
static void readDeviceListFromFile( void );

/*********************************************************************
 * FUNCTIONS
 *********************************************************************/

/*********************************************************************
 * @fn      createDeviceRec
 *
 * @brief   create a device and add a rec fto the list.
 *
 * @param   table
 * @param   rmTimer
 *
 * @return  none
 */
static deviceRecord_t* createDeviceRec( epInfo_t epInfo )
{
  deviceRecord_t *srchRec;
  
  //printf("createDeviceRec++\n");
  
  //does it already exist  
  if( findDeviceRec( epInfo.nwkAddr, epInfo.endpoint ) )
  {
    //printf("createDeviceRec: Device already exists\n");
    return NULL;
  }
      
  deviceRecord_t *newDevice = malloc( sizeof( deviceRecord_t ) );
  
  //Store the epInfo
  memcpy( &(newDevice->epInfo), &epInfo, sizeof(epInfo_t));
  
  newDevice->epInfo.deviceName = NULL;
  newDevice->epInfo.status = DEVLIST_STATE_ACTIVE;
  
  //printf("New Device added (%x) - ADDR:%x, DEVICE:%x, PROFILE:%x, EP:%x\n", newDevice, newDevice->epInfo.nwkAddr, newDevice->epInfo.deviceID, newDevice->epInfo.profileID, newDevice->epInfo.endpoint);
   
  newDevice->next = NULL;   
   
  if(deviceRecordHead)
  {
    //find the end of the list and add the record
    srchRec = deviceRecordHead;
    // Stop at the last record
    while ( srchRec->next )
      srchRec = srchRec->next;

    // Add to the list
    srchRec->next = newDevice; 
    
    //printf("New Device added to end of list (%x)\n", srchRec->next);
  }
  else
  {
    //printf("createDeviceRec: adding new device to head of the list\n");
    deviceRecordHead = newDevice;
  }
      
  return newDevice;
  //printf("createDeviceRec--\n");
}

/*********************************************************************
 * @fn      findDeviceRec
 *
 * @brief   find a device and in the list.
 *
 * @param   nwkAddr
 * @param   endpoint
 *
 * @return  deviceRecord_t for the device foundm null if not found
 */
static deviceRecord_t* findDeviceRec( uint16_t nwkAddr, uint8_t endpoint)
{
  deviceRecord_t *srchRec = deviceRecordHead;

  //printf("findDeviceRec++: nwkAddr:%x, ep:%x\n", nwkAddr, endpoint);
  
  // find record
  while ( (srchRec) && !((srchRec->epInfo.nwkAddr == nwkAddr) && (srchRec->epInfo.endpoint == endpoint)) )
  {
    //printf("findDeviceRec++: searching nwkAddr:%x, ep:%x\n", srchRec->epInfo.nwkAddr, srchRec->epInfo.endpoint);
    srchRec = srchRec->next;  
  }
  
  //printf("findDeviceRec[%x]--\n", srchRec);
   
  return srchRec;
}

/***************************************************************************************************
 * @fn      findDeviceInFileString - remove device from file.
 *
 * @brief   
 * @param   
 *
 * @return 
 ***************************************************************************************************/
	static char* findDeviceInFileString( uint16_t nwkAddr, uint8_t endpoint, char* fileBuf, uint32_t bufLen )
	{
	  char *deviceIdx = NULL, *deviceStartIdx, *deviceEndIdx;
	  uint32_t remainingBytes;
	  
	  printf("findDeviceInFile++: bufLen=%d, fileBuf:%p\n", bufLen, fileBuf);
	  
	  deviceStartIdx = fileBuf;
	  remainingBytes = bufLen;
	  //set to a non NULL value
	  deviceEndIdx = fileBuf;
	  
	  while( ((deviceStartIdx - fileBuf) < bufLen) && (deviceEndIdx != 0) )
	  {
		printf("findDeviceInFile++: bufLen=%d\n", bufLen);
		
		//is this device the correct device (start + IEEE Addr of 8 bytes)
		if( *((uint16_t*)(deviceStartIdx+8)) == nwkAddr )
		{
		  //device found
		  printf("findDeviceInFileString: device:%x found\n", nwkAddr);
		  deviceIdx = deviceStartIdx;
		  break;
		}
		else
		{
		  printf("findDeviceInFileString: device:%x found, looking for %x\n", *((uint16_t*)(deviceStartIdx+8)), nwkAddr);	
		}	 
		 
		//find end of current device by finding the delimiter
		deviceEndIdx = deviceStartIdx;
		while((*deviceEndIdx != ';') && ((fileBuf - deviceEndIdx) < bufLen))
		{
		  deviceEndIdx++;
		}
		
		if( deviceEndIdx > (fileBuf + bufLen) )
		{
		  //past end of file string
		  //printf("findDeviceInFile++\n");
		   deviceEndIdx = NULL;
		}
		
		if( deviceEndIdx )
		{
		  remainingBytes = bufLen - (fileBuf - deviceEndIdx);
		  deviceStartIdx = &(deviceEndIdx[1]);	  
		}
	  }
	  
	  //printf("findDeviceInFile-- [%x]\n", (uint32_t) deviceIdx);
	  
	  return deviceIdx;
	}

/***************************************************************************************************
 * @fn      removeDeviceFromFile - remove device from file.
 *
 * @brief   
 * @param   
 *
 * @return 
 ***************************************************************************************************/
	static void removeDeviceFromFile( uint16_t nwkAddr, uint8_t endpoint )
	{
	  FILE *fpDevFile;
	  uint32_t fileSize;
	  char *fileBuf, *deviceStr, *devStrEnd;
	  
	  //printf("removeDeviceFromFile++\n");
	  
	  fpDevFile = fopen("devicelistfile.dat", "rwb");
	  
	  if(fpDevFile)
	  {
		//read the file into a buffer  
		fseek(fpDevFile, 0, SEEK_END);
		fileSize = ftell(fpDevFile);
		rewind(fpDevFile);	
		fileBuf = (char*) calloc(sizeof(char), fileSize);  
		fread(fileBuf, 1, fileSize, fpDevFile);
	
		//printf("removeDeviceFromFile: number of bytes in file = %d\n", fileSize);
		//printf("removeDeviceFromFile: Searching for device string\n");
		//find the device
		deviceStr = findDeviceInFileString( nwkAddr, endpoint, fileBuf, fileSize );
		
		if( deviceStr )
		{
		  printf("removeDeviceFromFile: device string:%x\n", (uint32_t) deviceStr);
		
		  //find end of current device by finding the delimiter
		  devStrEnd = deviceStr;
		  while((*devStrEnd != ';') && ((devStrEnd - fileBuf) < fileSize))
		  {
			devStrEnd++;
			printf("removeDeviceFromFile: finding delimiter:%c:%x\n", *devStrEnd, (uint32_t) devStrEnd);
		  } 	 
		  
		  if((devStrEnd - fileBuf) < fileSize)
		  { 		
			//copy start of file to bigenning of device
			fwrite((const void *) fileBuf, fileBuf-deviceStr, 1, fpDevFile);
			//copy end of device to end of file
			fwrite((const void *) devStrEnd, fileSize - (fileBuf - devStrEnd), 1, fpDevFile);	   
		  }
		  else
		  {
			printf("removeDeviceFromFile: device delimiter not found\n");
		  }
		}
		else
		{
		  printf("removeDeviceFromFile: device not found in file\n");
		}
	  }
		
	  fflush(fpDevFile);
	  fclose(fpDevFile);
	  free(fileBuf);
	}

/***************************************************************************************************
 * @fn      writeDeviceToFile - store device list.
 *
 * @brief   
 * @param   
 *
 * @return 
 ***************************************************************************************************/
static void writeDeviceToFile( deviceRecord_t *device )
{
  FILE *fpDevFile;
  
  //printf("writeDeviceToFile++\n");
  
  fpDevFile = fopen("devicelistfile.dat", "a+b");

  if(fpDevFile)
  {
    //printf("writeDeviceToFile: opened file\n");
    
    //printf("writeDeviceToFile: Store epInfo[%d - %d - %d - %d]\n", sizeof(epInfo_t), sizeof (char*), sizeof (uint8_t), ((sizeof(epInfo_t)) - (sizeof (char*) + sizeof (uint8_t))));
    //Store epInfo - device name pointer and status 
    fwrite((const void *) &(device->epInfo), (sizeof(epInfo_t) - (sizeof (char*) + sizeof (uint8_t))), 1, fpDevFile);
    
    //Store deviceName len
    if(device->epInfo.deviceName)
    {    
      uint8_t i;      
      printf("writeDeviceToFile: Store deviceName - %d: ", (device->epInfo.deviceName[0]));
      
      //first char of dev name is str length
      for(i = 0; i < (device->epInfo.deviceName[0] + 1) ; i++)
      {
        fwrite((const void *) (&(device->epInfo.deviceName[i])), 1, 1, fpDevFile);
        printf("%c", (device->epInfo.deviceName[i]));        
      }
      printf("\n");
    }
    else
    {
      //just store the len of 0
      uint8_t tmp = 0;
      fwrite(&tmp, 1, 1, fpDevFile);
    }
    
    //printf("writeDeviceToFile: Store status\n");
    //Store status
    fwrite((const void *) &(device->epInfo.status), 1, 1, fpDevFile);
    //socWrite delimter
    fwrite(";", 1, 1, fpDevFile);
    
    fflush(fpDevFile);
    fclose(fpDevFile); 
  }
}    

/***************************************************************************************************
 * @fn      readDeviceListFromFile - restore the device list.
 *
 * @brief   
 *
 * @return 
 ***************************************************************************************************/
static void readDeviceListFromFile( void )
{
  FILE *fpDevFile;
  deviceRecord_t *device;
  epInfo_t epInfo;
  char chTmp;
  
  //printf("readDeviceListFromFile++\n");
  fpDevFile = fopen("devicelistfile.dat", "a+b");

  if(fpDevFile)
  {
    //printf("readDeviceListFromFile: file opened\n");
    //read epInfo_t - device name pointer and status 
    while(fread(&(epInfo), (sizeof(epInfo_t) - (sizeof (char*) + sizeof (uint8_t))), 1, fpDevFile))
    {
      //printf("readDeviceListFromFile: epInfo[%d] read for device %x\n", (sizeof(epInfo_t) - (sizeof (char*) + sizeof (uint8_t))), epInfo.nwkAddr);
      
      device = createDeviceRec(epInfo);      
      
      if(device)
      {
        uint8_t strLen; 
        char *strName;
        fread(&(strLen), 1, 1, fpDevFile);
        //printf("readDeviceListFromFile: strLen %d\n", strLen); 
        
        if(strLen > 0)
        {
          strName = malloc(strLen + 1);
          if(strName)
          {          
            strName[0] = strLen;
            fread(&(strName[1]), 1, strLen, fpDevFile);
          }
          device->epInfo.deviceName = strName;
        }

        fread(&(device->epInfo.status), 1, 1, fpDevFile);
        //printf("readDeviceListFromFile: device->epInfo.status %x\n", device->epInfo.status); 
        
        //read ';' delimeter
        fread(&chTmp, 1, 1, fpDevFile);
        
        if(chTmp != ';')
        {
          //printf("readDeviceListFromFile: Error, device delimter not found\n");        
          return;
        }
      }
    }
    
    fflush(fpDevFile);
    fclose(fpDevFile); 
  }
  
  //printf("readDeviceListFromFile--\n");
}

/*********************************************************************
 * @fn      devListAddDevice
 *
 * @brief   create a device and add a rec to the list.
 *
 * @param   epInfo
 *
 * @return  none
 */
void devListAddDevice( epInfo_t *epInfo)
{ 
  //printf("devListAddDevice++(%x:%x)\n", epInfo->nwkAddr, epInfo->endpoint);
   
  deviceRecord_t *device = createDeviceRec(*epInfo);
  if(device)
  {
    writeDeviceToFile(device);
  }
  
  //printf("\n\n\ndevListAddDevice: list=\n");
  //find the end of the list and add the record
  deviceRecord_t *srchRec = deviceRecordHead;
  //printf("%x", srchRec);
  
  while ( srchRec->next )
  {
      srchRec = srchRec->next;
      //printf("%x\n", srchRec);
  }
  //printf("\n\n\n");
     
  //printf("devListAddDevice--\n");
}

/*********************************************************************
 * @fn      devListRemoveDevice
 *
 * @brief   remove a device rec from the list.
 *
 * @param   nwkAddr - nwkAddr of device to be removed
 * @param   endpoint - endpoint of device to be removed 
 *
 * @return  none
 */
void devListRemoveDevice( uint16_t nwkAddr, uint8_t endpoint )
{
  deviceRecord_t *srchRec, *prevRec=NULL;

  printf("deleteDeviceRec: nwkAddr:%x, endpoint:%x \n", nwkAddr, endpoint);

  // find record and prev device record
  srchRec = deviceRecordHead;
  
  // find record
  while ( (srchRec) && !((srchRec->epInfo.nwkAddr == nwkAddr) && (srchRec->epInfo.endpoint == endpoint)) )
  {
    prevRec = srchRec;
    srchRec = srchRec->next;  
  }
     
  if (srchRec == NULL)
  {
      printf("deleteDeviceRec: record not found\n");
      return;    
  }
  else
  {               
    // delete the rec from the list
    if ( prevRec == NULL )      
    {
      //at head of the list
      //remove record from list 
      deviceRecordHead = srchRec->next;
    }
    else
    {
      //remove record from list    
      prevRec->next = srchRec->next;    
    }
    
    free(srchRec);        
    removeDeviceFromFile(nwkAddr, endpoint);
  }
}

/*********************************************************************
 * @fn      devListRestorDevices
 *
 * @brief   create a device list from file.
 *
 * @param   none
 *
 * @return  none
 */
void devListRestorDevices( void )
{
  //printf("devListRestorDevices++\n");
  if( deviceRecordHead == NULL)
  {
    readDeviceListFromFile();
  }
  //else do what, should we delete the list and recreate from the file?
}

/*********************************************************************
 * @fn      devListChangeDeviceName
 *
 * @brief   change the name of a device.
 *
 * @return 
 */
void devListChangeDeviceName( uint16_t devNwkAddr, uint8_t devEndpoint, char *deviceNameStr)
{
  printf("devListChangeDeviceName++: devNwkAddr:%x, devEndpoint:%x\n", devNwkAddr, devEndpoint);   
    
  deviceRecord_t *device = findDeviceRec( devNwkAddr, devEndpoint );             
  
  if(device)
  {
    if(device->epInfo.deviceName)
    {
      free(device->epInfo.deviceName);
    }
    
    printf("devListChangeDeviceName: removing device from file\n");  
    removeDeviceFromFile(devNwkAddr, devEndpoint);
    
    printf("devListChangeDeviceName: Changing device name: %c\n", (deviceNameStr[0] + 1));      
    //fisrt byte of deviceNameStr is the size of the string
    device->epInfo.deviceName = malloc(deviceNameStr[0]);
    strncpy(device->epInfo.deviceName, &(deviceNameStr[0]), (deviceNameStr[0] + 1) );
        
    printf("devListChangeDeviceName: writing device to file\n");  
    writeDeviceToFile(device);           
  }
  else
  {
    printf("devListChangeDeviceName: Device not found");  
  }  
  
  printf("devListChangeDeviceName--\n");

  return;  
}

/*********************************************************************
 * @fn      devListNumDevices
 *
 * @brief   get the number of devices in the list.
 *
 * @param   none
 *
 * @return  none
 */
uint32_t devListNumDevices( void )
{  
  uint32_t recordCnt=0;
  deviceRecord_t *srchRec;
  
  //printf("devListNumDevices++\n");
  
  // Head of the list
  srchRec = deviceRecordHead;  
  
  if(srchRec==NULL)
  {
    //printf("devListNumDevices: deviceRecordHead NULL\n");
    return -1;
  }
    
  // Stop when rec found or at the end
  while ( srchRec )
  {  
    //printf("devListNumDevices: recordCnt=%d\n", recordCnt);
    srchRec = srchRec->next;  
    recordCnt++;      
  }
  
  //printf("devListNumDevices %d\n", recordCnt);
  return (recordCnt);
}

/*********************************************************************
 * @fn      devListGetNextDev
 *
 * @brief   Return the next device in the list.
 *
 * @param   nwkAddr - if 0xFFFF it will return head of the list
 *
 * @return  epInfo, return next epInfo from nwkAddr and ep supplied or 
 *          NULL if at end of the list
 */
epInfo_t* devListGetNextDev( uint16_t nwkAddr, uint8_t endpoint )
{  
  epInfo_t* epInfo = NULL;
  deviceRecord_t *srchRec;
  
  //printf("devListGetNextDev++: nwkAddr=%x, endpoint=%x\n", nwkAddr, endpoint);
  
  // Head of the list
  srchRec = deviceRecordHead;  
  
  if(nwkAddr != 0xFFFF)
  {
    //Find the record for nwkAddr
    srchRec = findDeviceRec(nwkAddr, endpoint);
    //get the next record (may be NULL if at end of list)
    srchRec = srchRec->next;
    
    //printf("devListGetNextDev: found device %x\n", srchRec);      
    
    //Store the epInfo
    if(srchRec)
    {
      //printf("devListGetNextDev: returning next device %x\n", srchRec);      
      epInfo = &(srchRec->epInfo);
    }
  }
  else
  {    
    //printf("\n\n\ndevListGetNextDev: list= [%x]\n", deviceRecordHead);
    //find the end of the list and add the record
    deviceRecord_t *srchRec1 = deviceRecordHead;
    //printf("%x", srchRec1);
    if(srchRec1)
    {
      while ( srchRec1->next )
      {
          srchRec1 = srchRec1->next;
          //printf("%x\n", srchRec1);
      }
    }
    //printf("\n\n\n");
        
    //printf("devListGetNextDev: returning head of list\n");
    //return fisrt record
    if(deviceRecordHead)
    {      
      epInfo = &(deviceRecordHead->epInfo);
    }
  }
  
  //printf("devListGetNextDev[%x]--\n", epInfo);
  
  return (epInfo);
}
