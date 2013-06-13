 /**************************************************************************************************
  Filename:       socket_client.h
  Revised:        $Date: 2012-03-21 17:37:33 -0700 (Wed, 21 Mar 2012) $
  Revision:       $Revision: 246 $

  Description:    This file contains Linux platform specific RemoTI (RTI) API
                  Surrogate implementation

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

#ifndef SOCKET_CLIENT_H
#define SOCKET_CLIENT_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
 *                                           Includes
 **************************************************************************************************/
#include <stdint.h>

/**************************************************************************************************
 *                                        Externals
 **************************************************************************************************/

/**************************************************************************************************
 *                                           Constant
 **************************************************************************************************/
#define AP_MAX_BUF_LEN 128
#define SRPC_FRAME_HDR_SZ 2
#define DEFAULT_PORT "1234"

/**************************************************************************************************
 *                                        Type definitions
 **************************************************************************************************/
typedef struct ATTR_PACKED
{
  uint8_t cmdId;
  uint8_t len;
  uint8_t pData[AP_MAX_BUF_LEN];
} msgData_t;

typedef void (*socketClientCb_t)( msgData_t *msg ); 

/**************************************************************************************************
 *                                        Global Variables
 **************************************************************************************************/

/**************************************************************************************************
 *                                        Local Variables
 **************************************************************************************************/







/**************************************************************************************************
 *
 * @fn          RTIS_Init_Socket
 *
 * @brief       This function initializes RTI Surrogate
 *
 * input parameters
 *
 * @param       ipAddress - path to the NPI Server Socket
 *
 * output parameters
 *
 * None.
 *
 * @return      TRUE if the surrogate module started off successfully.
 *              FALSE, otherwise.
 *
 **************************************************************************************************/
int socketClientInit(const char *devPath, socketClientCb_t cb);


/**************************************************************************************************
 *
 * @fn          socketClientClose
 *
 * @brief       This function stops RTI surrogate module
 *
 * input parameters
 *
 * None.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 *
 **************************************************************************************************/
void socketClientClose(void);


/**************************************************************************************************
 *
 * @fn          socketClientSendData
 *
 * @brief       This function sends a message over the socket
 *
 * input parameters
 *
 * None.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 *
 **************************************************************************************************/
void socketClientSendData (msgData_t *pMsg);


#ifdef __cplusplus
}
#endif

#endif /* SOCKET_SERVER_H */