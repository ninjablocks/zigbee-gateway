/**************************************************************************************************
 * Filename:       interface_devicelist.h
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

#ifndef INTERFACE_DEVICELIST_H
#define INTERFACE_DEVICELIST_H

#ifdef __cplusplus
extern "C"
{
#endif


/*********************************************************************
 * INCLUDES
 */
#include <stdint.h>
#include "zbSocCmd.h"
#include "hal_types.h"

//device states
#define DEVLIST_STATE_NOT_ACTIVE    0
#define DEVLIST_STATE_ACTIVE        1

#define MAX_SUPPORTED_DEVICE_NAME_LENGTH 32

typedef struct
{
  void   *next;
  epInfo_t epInfo;  
}deviceRecord_t;

/*
 * devListAddDevice - create a device and add a rec to the list.
 */
void devListAddDevice( epInfo_t *epInfo);

/*
 * devListRemoveDeviceByNaEp - remove a device rec from the list.
 */
epInfo_t * devListRemoveDeviceByNaEp( uint16_t nwkAddr, uint8_t endpoint );

/*
 * devListNumDevices - get the number of devices in the list.
 */
uint32_t devListNumDevices( void );

/*
 * devListInitDatabase - restore device list from file.
 */
void devListInitDatabase( char * dbFilename );

epInfo_t * devListGetNextDev(uint32 *context);

epInfo_t * devListGetDeviceByIeeeEp( uint8_t ieeeAddr[8], uint8_t endpoint );

epInfo_t * devListGetDeviceByNaEp( uint16_t nwkAddr, uint8_t endpoint );

epInfo_t * devListRemoveDeviceByIeee( uint8_t ieeeAddr[8] );

#ifdef __cplusplus
}
#endif

#endif /* INTERFACE_DEVICELIST_H */
