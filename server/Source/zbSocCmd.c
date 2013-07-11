
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
#include <ctype.h>

#include <errno.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <sys/timerfd.h>

#include "zbSocCmd.h"
#include "zbSocTransport.h"
#if (!HAL_UART_SPI)
#include "zbSocTransportUart.c"
#else
#include "zbSocTransportSpi.c"
#endif

/*********************************************************************
 * MACROS
 */

#define FALSE 0
#define TRUE (!FALSE)

#define APPCMDHEADER(len) \
0xFE,                                                                             \
len,   /*RPC payload Len                                      */     \
0x29, /*MT_RPC_CMD_AREQ + MT_RPC_SYS_APP        */     \
0x00, /*MT_APP_MSG                                                   */     \
0x0B, /*Application Endpoint                                  */     \
0x02, /*short Addr 0x0002                                     */     \
0x00, /*short Addr 0x0002                                     */     \
0x0B, /*Dst EP                                                             */     \
0xFF, /*Cluster ID 0xFFFF invalid, used for key */     \
0xFF, /*Cluster ID 0xFFFF invalid, used for key */     \

#define BUILD_UINT16(loByte, hiByte) \
          ((uint16_t)(((loByte) & 0x00FF) + (((hiByte) & 0x00FF) << 8)))
          
#define BUILD_UINT32(Byte0, Byte1, Byte2, Byte3) \
          ((uint32_t)((uint32_t)((Byte0) & 0x00FF) \
          + ((uint32_t)((Byte1) & 0x00FF) << 8) \
          + ((uint32_t)((Byte2) & 0x00FF) << 16) \
          + ((uint32_t)((Byte3) & 0x00FF) << 24)))

			
#define TIMEOUT_TIMER 0 //(&timers[0])
#define REPORTING_TIMER 1 //(&timers[1])
			      
/*********************************************************************
 * CONSTANTS
 */
#define MT_SYS_OSAL_NV_WRITE                 0x09

#define MT_APP_RPC_CMD_TOUCHLINK          0x01
#define MT_APP_RPC_CMD_RESET_TO_FN        0x02
#define MT_APP_RPC_CMD_CH_CHANNEL         0x03
#define MT_APP_RPC_CMD_JOIN_HA            0x04
#define MT_APP_RPC_CMD_PERMIT_JOIN        0x05
#define MT_APP_RPC_CMD_SEND_RESET_TO_FN   0x06
#define MT_APP_RPC_CMD_INSTALL_CERTIFICATE   0x07

#define MT_APP_RSP                           0x80
#define MT_APP_ZLL_TL_IND                    0x81
#define MT_APP_NEW_DEV_IND               0x82
#define MT_APP_KEY_ESTABLISHMENT_STATE_IND   0x90

#define MT_DEBUG_MSG                         0x80

#define COMMAND_LIGHTING_MOVE_TO_HUE  0x00
#define COMMAND_LIGHTING_MOVE_TO_SATURATION 0x03
#define COMMAND_LEVEL_MOVE_TO_LEVEL 0x00

/*** Foundation Command IDs ***/
#define ZCL_CMD_READ                                    0x00
#define ZCL_CMD_READ_RSP                                0x01
#define ZCL_CMD_WRITE                                   0x02
#define ZCL_CMD_WRITE_UNDIVIDED                         0x03
#define ZCL_CMD_WRITE_RSP                               0x04

// General Clusters
#define ZCL_CLUSTER_ID_GEN_IDENTIFY                    0x0003
#define ZCL_CLUSTER_ID_GEN_GROUPS                      0x0004
#define ZCL_CLUSTER_ID_GEN_SCENES                      0x0005
#define ZCL_CLUSTER_ID_GEN_ON_OFF                      0x0006
#define ZCL_CLUSTER_ID_GEN_LEVEL_CONTROL               0x0008
#define ZCL_CLUSTER_ID_GEN_KEY_ESTABLISHMENT                 0x0800
// Lighting Clusters
#define ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL          0x0300
// Mettering and Sensing Clusters
#define ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT			 0x0402
#define ZCL_CLUSTER_ID_MS_REL_HUMIDITY_MEASUREMENT		 0x0405
// Pricing
#define ZCL_CLUSTER_ID_SE_PRICING                            0x0700
//Metering
#define ZCL_CLUSTER_ID_SE_SIMPLE_METERING              0x0702
// Messaging
#define ZCL_CLUSTER_ID_SE_MESSAGE                            0x0703
// Security and Safety (SS) Clusters
#define ZCL_CLUSTER_ID_SS_IAS_ZONE                     0x0500

// Data Types
#define ZCL_DATATYPE_BOOLEAN                            0x10
#define ZCL_DATATYPE_UINT8                              0x20
#define ZCL_DATATYPE_UINT16                             0x21
#define ZCL_DATATYPE_INT16                              0x29
#define ZCL_DATATYPE_INT24                              0x2a

/*******************************/
/*** Generic Cluster ATTR's  ***/
/*******************************/
#define ATTRID_ON_OFF                                     0x0000
#define ATTRID_LEVEL_CURRENT_LEVEL                        0x0000

/*******************************/
/*** Lighting Cluster ATTR's  ***/
/*******************************/
#define ATTRID_LIGHTING_COLOR_CONTROL_CURRENT_HUE         0x0000
#define ATTRID_LIGHTING_COLOR_CONTROL_CURRENT_SATURATION  0x0001


/*******************************/
/*** Mettering and Sensing Cluster ATTR's  ***/
/*******************************/
#define ATTRID_MS_TEMPERATURE_MEASURED_VALUE              0x0000
#define ATTRID_MS_RELATIVE_HUMIDITY_MEASURED_VALUE        0x0000

/*******************************/
/*** SE SIMPLE METERING Cluster ATTR's  ***/
/*******************************/
#define ATTRID_MASK_SE_HISTORICAL_CONSUMPTION             0x0400
#define ATTRID_SE_INSTANTANEOUS_DEMAND               ( 0x0000 | ATTRID_MASK_SE_HISTORICAL_CONSUMPTION )

/*******************************/
/*** Scenes Cluster Commands ***/
/*******************************/
#define COMMAND_SCENE_STORE                               0x04
#define COMMAND_SCENE_RECALL                              0x05

/*******************************/
/*** Groups Cluster Commands ***/
/*******************************/
#define COMMAND_GROUP_ADD                                 0x00

/********************************************/
/*** Safety and Security Cluster Commands ***/
/********************************************/
#define COMMAND_SS_IAS_ZONE_STATUS_CHANGE_NOTIFICATION                   0x00
#define COMMAND_SS_IAS_ZONE_STATUS_ENROLL_REQUEST                        0x01
#define COMMAND_SS_IAS_ZONE_STATUS_ENROLL_RESPONSE                       0x00
   // permitted values for Enroll Response Code field
#define SS_IAS_ZONE_STATUS_ENROLL_RESPONSE_CODE_SUCCESS                  0x00
#define SS_IAS_ZONE_STATUS_ENROLL_RESPONSE_CODE_NOT_SUPPORTED            0x01
#define SS_IAS_ZONE_STATUS_ENROLL_RESPONSE_CODE_NO_ENROLL_PERMIT         0x02
#define SS_IAS_ZONE_STATUS_ENROLL_RESPONSE_CODE_TOO_MANY_ZONES           0x03

/**********************************/
/*** Messaging Cluster Commands ***/
/**********************************/
#define COMMAND_SE_GET_LAST_MESSAGE                   0x0000  // Messaging client command
#define COMMAND_SE_DISPLAY_MESSAGE                    0x0000  // Messaging server command

/********************************/
/*** Pricing Cluster Commands ***/
/********************************/
#define COMMAND_SE_GET_CURRENT_PRICE                  0x0000  // Pricing client command
#define COMMAND_SE_PUBLISH_PRICE                      0x0000  // Pricing server command


/* The 3 MSB's of the 1st command field byte are for command type. */
#define MT_RPC_CMD_TYPE_MASK  0xE0

/* The 5 LSB's of the 1st command field byte are for the subsystem. */
#define MT_RPC_SUBSYSTEM_MASK 0x1F

#define MT_RPC_SOF         0xFE

#define MAX_TIMEOUT_RETRIES 10

#define RPC_BUF_CMD0 0
#define RPC_BUF_CMD1 1
#define RPC_BUF_INCOMING_RESULT 2


#define ZBSOC_SBL_STATE_IDLE 0
#define ZBSOC_SBL_STATE_HANDSHAKING 1
#define ZBSOC_SBL_STATE_PROGRAMMING 2
#define ZBSOC_SBL_STATE_VERIFYING 3
#define ZBSOC_SBL_STATE_EXECUTING 4

#define ZBSOC_SBL_MAX_RETRY_ON_ERROR_REPORTED 5
#define ZBSOC_SBL_IMAGE_BLOCK_SIZE 64

#define SB_WRITE_CMD                0x01
#define SB_READ_CMD                 0x02
#define SB_ENABLE_CMD               0x03
#define SB_HANDSHAKE_CMD            0x04

#define SB_TGT_BOOTLOAD             0x10
#define SB_RESPONSE					0x80

#define SB_SUCCESS                  0
#define SB_FAILURE                  1

//#define SB_MB_BOOT_APP              1
#define SB_MB_WAIT_HS               2

//#define SB_RPC_SYS_BOOT             13
#define SB_RPC_CMD_AREQ             0x40

#define MT_SYS_RESET_REQ            0x00

#define SB_FORCE_BOOT               0xF8
#define SB_FORCE_RUN               (SB_FORCE_BOOT ^ 0xFF)

#define ZBSOC_SBL_MAX_FRAME_SIZE			71

#define ZBSOC_CERT_STATE_IMPLICIT_CERT   0
#define ZBSOC_CERT_STATE_DEV_PRIVATE_KEY 1
#define ZBSOC_CERT_STATE_CA_PUBLIC_KEY   2
#define ZBSOC_CERT_STATE_IEEE_ADDRESS    3
#define ZBSOC_CERT_STATE_ERASE_NWK_INFO  4
#define ZBSOC_CERT_STATE_COMPLETE        5
#define ZBSOC_CERT_STATE_IDLE            6

#define MT_NLME_LEAVE_REQ         0x05

#define Z_EXTADDR_LEN    8

#define ZCL_KE_IMPLICIT_CERTIFICATE_LEN                  48
#define ZCL_KE_DEVICE_PRIVATE_KEY_LEN                    21
#define ZCL_KE_CA_PUBLIC_KEY_LEN                         22

#define MAX_CERT_LINE_LEN 256

#define ZBSOC_CERT_OPER_START       0
#define ZBSOC_CERT_OPER_CONT        1
#define ZBSOC_CERT_OPER_ERROR       2
#define ZBSOC_CERT_OPER_STOP        3

#define Z_EXTADDR_LEN 8

typedef enum
{
  afAddrNotPresent = 0,
  afAddrGroup      = 1,
  afAddr16Bit      = 2,
  afAddr64Bit      = 3,  
  afAddrBroadcast  = 15
} afAddrMode_t;

typedef enum {
  MT_RPC_CMD_POLL = 0x00,
  MT_RPC_CMD_SREQ = 0x20,
  MT_RPC_CMD_AREQ = 0x40,
  MT_RPC_CMD_SRSP = 0x60,
  MT_RPC_CMD_RES4 = 0x80,
  MT_RPC_CMD_RES5 = 0xA0,
  MT_RPC_CMD_RES6 = 0xC0,
  MT_RPC_CMD_RES7 = 0xE0
} mtRpcCmdType_t;

typedef enum {
  MT_RPC_SYS_RES0,   /* Reserved. */
  MT_RPC_SYS_SYS,
  MT_RPC_SYS_MAC,
  MT_RPC_SYS_NWK,
  MT_RPC_SYS_AF,
  MT_RPC_SYS_ZDO,
  MT_RPC_SYS_SAPI,   /* Simple API. */
  MT_RPC_SYS_UTIL,
  MT_RPC_SYS_DBG,
  MT_RPC_SYS_APP,
  MT_RPC_SYS_OTA,
  MT_RPC_SYS_ZNP,
  MT_RPC_SYS_SPARE_12,
  MT_RPC_SYS_SBL = 13,  // 13 to be compatible with existing RemoTI. // AKA MT_RPC_SYS_UBL
  MT_RPC_SYS_MAX        // Maximum value, must be last (so 14-32 available, not yet assigned).
} mtRpcSysType_t;

const char * BOOTLOADER_RESULT_STRINGS[] = 
{
	"SBL_SUCCESS",
	"SBL_INTERNAL_ERROR",
	"SBL_BUSY",
	"SBL_OUT_OF_MEMORY",
	"SBL_PENDING",
	"SBL_ABORTED_BY_USER",
	"SBL_NO_ACTIVE_DOWNLOAD",
	"SBL_ERROR_OPENING_FILE",
	"SBL_TARGET_WRITE_FAILED",
	"SBL_EXECUTION_FAILED",
	"SBL_HANDSHAKE_FAILED",
	"SBL_LOCAL_READ_FAILED",
	"SBL_VERIFICATION_FAILED",
	"SBL_COMMUNICATION_FAILED",
	"SBL_ABORTED_BY_ANOTHER_USER",
	"SBL_REMOTE_ABORTED_BY_USER",
	"SBL_TARGET_READ_FAILED",
//	"SBL_TARGET_STILL_WORKING", not an actual return code. Also - it corresponds to the value 0xFF, so irrelevant in this table.
};


/************************************************************
 * TYPEDEFS
 */

typedef struct {
  uint8_t ieeeAddr[Z_EXTADDR_LEN];
  uint8_t implicitCert[ZCL_KE_IMPLICIT_CERTIFICATE_LEN];
  uint8_t caPublicKey[ZCL_KE_CA_PUBLIC_KEY_LEN];
  uint8_t devPrivateKey[ZCL_KE_DEVICE_PRIVATE_KEY_LEN];
} certInfo_t;

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
int serialPortFd = 0;
uint8_t transSeqNumber = 0;

zbSocCallbacks_t zbSocCb;
uint32_t zbSocSblProgressReportingInterval;
uint8_t zbSocSblReportingPending = FALSE;
FILE * zbSocSblImageFile;
certInfo_t zbSocCertInfo;
uint8_t zbSocCertForce2Reset;
	
int timeout_retries;
uint16_t zbSocSblState = ZBSOC_SBL_STATE_IDLE;
uint16_t zbSocCertState = ZBSOC_CERT_STATE_IDLE;

zllTimer timers[2] = {
	{NULL, 0, FALSE},
	{NULL, 0, FALSE},
};

timerFDs_t * timerFDs = NULL;

/*********************************************************************
 * EXTERNAL VARIABLES
 */
extern uint8_t uartDebugPrintsEnabled;


/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void calcFcs(uint8_t *msg, int size);
static void processRpcSysAppTlInd(uint8_t *TlIndBuff);
static void processRpcSysAppNewDevInd(uint8_t *TlIndBuff);
static void processRpcSysAppZcl(uint8_t *zclRspBuff);
static void processRpcSysAppZclFoundation(uint8_t *zclRspBuff, uint8_t zclFrameLen, uint16_t clusterID, uint16_t nwkAddr, uint8_t endpoint);
static void processRpcSysAppZclCluster(uint8_t *zclRspBuff, uint8_t zclFrameLen, uint16_t clusterID, uint16_t nwkAddr, uint8_t endpoint);
static void processRpcSysSys(uint8_t *rpcBuff);
static void processRpcSysApp(uint8_t *rpcBuff);
static void processRpcSysDbg(uint8_t *rpcBuff, uint8_t len);
void zbSocSblEnableBootloader();
void zbSocTimeoutCallback(void);
void zbSocSblReportingCallback(void);
static void processRpcSysSbl(uint8_t *rpcBuff);
static void processCertInstall(uint8_t operation);
void zbSocSblExecuteImage(void);



/*********************************************************************
 * @fn      calcFcs
 *
 * @brief   populates the Frame Check Sequence of the RPC payload.
 *
 * @param   msg - pointer to the RPC message
 *
 * @return  none
 */
static void calcFcs(uint8_t *msg, int size)
{
	uint8_t result = 0;
	int idx = 1; //skip SOF
	int len = (size - 2);  // skip FCS

	while ((len--) != 0) {
		result ^= msg[idx++];
	}
	
	msg[(size-1)] = result;
}

/*********************************************************************
 * @fn      hexStr2Array
 *
 * @brief   converts hexadecimal char string to byte array.
 *
 * @param   str - input string
 *          len - size of array
 *          array - output byte array
 *
 * @return  none
 */
static uint8_t hexStr2Array(char *str, int len, uint8_t *array)
{
  int i, j;
  uint8_t tempNibble;

  if (strlen(str) > len * 2)
  {
    return CERT_RESULT_WRONG_FORMAT;
  }

  for (i = 0; i < strlen(str); i += 2)
  {
    array[i / 2] = 0;

    for (j = 0; j < 2; j++)
    {
      if (!isxdigit(str[i + j]))
      {
        return CERT_RESULT_WRONG_FORMAT;
      }

      tempNibble = (uint8_t) toupper(str[i + j]) - '0';
      if (tempNibble > 9)
      {
        tempNibble -= ('A' - ('9' + 1));
      }

      array[i / 2] = array[i / 2] * 16 + tempNibble;
    }
  }

  return SUCCESS;
}

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
int32_t zbSocOpen( char *_devicePath  )
{
  serialPortFd = zbSocTransportOpen(_devicePath);
  if (serialPortFd <0) 
  {
    perror(_devicePath); 
    printf("%s open failed\n",_devicePath);
    return(-1);
  }
  
  return serialPortFd;
}


void zbSocForceRun(void)
{
	uint8_t forceBoot = SB_FORCE_RUN;
	
	//Send the bootloader force boot incase we have a bootloader that waits
	zbSocTransportWrite(&forceBoot, 1);
	
}
	
void zbSocClose( void )
{
  zbSocTransportClose();

  return;
}

/*********************************************************************
 * @fn      zbSocRegisterCallbacks
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
	uint8_t tlCmd[] = {
		APPCMDHEADER(13)
		0x06, //Data Len
		0x02, //Address Mode
		0x00, //2dummy bytes
		0x00,
		MT_APP_RPC_CMD_TOUCHLINK,
		0x00,     //
		0x00,     //
		0x00       //FCS - fill in later
    };
	  
    calcFcs(tlCmd, sizeof(tlCmd));
    zbSocTransportWrite(tlCmd, sizeof(tlCmd));  
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
	uint8_t tlCmd[] = {
		APPCMDHEADER(13)
		0x06, //Data Len
		0x02, //Address Mode
		0x00, //2dummy bytes
		0x00,
		MT_APP_RPC_CMD_RESET_TO_FN,
		0x00,     //
		0x00,     //
		0x00       //FCS - fill in later
    };
	  
    calcFcs(tlCmd, sizeof(tlCmd));
    zbSocTransportWrite(tlCmd, sizeof(tlCmd));
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
	uint8_t tlCmd[] = {
		APPCMDHEADER(13)
		0x06, //Data Len
		0x02, //Address Mode
		0x00, //2dummy bytes
		0x00,
		MT_APP_RPC_CMD_SEND_RESET_TO_FN,
		0x00,     //
		0x00,     //
		0x00       //FCS - fill in later
    };
	  
    calcFcs(tlCmd, sizeof(tlCmd));
    zbSocTransportWrite(tlCmd, sizeof(tlCmd)); 
}

/*********************************************************************
 * @fn      zbSocOpenNwk
 *
 * @brief   Send the open network command to a ZigBee device.
 *
 * @param   none
 *
 * @return  none
 */
void zbSocOpenNwk(void)
{  
	uint8_t cmd[] = {
		APPCMDHEADER(13)
		0x06, //Data Len
		0x02, //Address Mode
		0x00, //2dummy bytes
		0x00,
		MT_APP_RPC_CMD_PERMIT_JOIN,
		60, // open for 60s
		1,  // open all devices
		0x00  //FCS - fill in later
    };
	  
    calcFcs(cmd, sizeof(cmd));
    zbSocTransportWrite(cmd, sizeof(cmd)); 
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
  	uint8_t cmd[] = {
  		0xFE,                                                                                      
  		11,   /*RPC payload Len */          
  		0x29, /*MT_RPC_CMD_AREQ + MT_RPC_SYS_APP */          
  		0x00, /*MT_APP_MSG  */          
  		0x0B, /*Application Endpoint */          
  		(dstAddr & 0x00ff),
  		(dstAddr & 0xff00) >> 8,
  		endpoint, /*Dst EP */          
  		(ZCL_CLUSTER_ID_GEN_ON_OFF & 0x00ff),
  		(ZCL_CLUSTER_ID_GEN_ON_OFF & 0xff00) >> 8,
  		0x04, //Data Len
  		addrMode, 
  		0x01, //0x01 ZCL frame control field.  (send to the light cluster only)
  		transSeqNumber++,
		(state ? 1:0),
  		0x00       //FCS - fill in later
  	};

  	calcFcs(cmd, sizeof(cmd));
    zbSocTransportWrite(cmd,sizeof(cmd));
}


/*********************************************************************
 * @fn      zbSocGetTimerFds
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
void zbSocGetTimerFds(timerFDs_t *fds)
{
  int i;
  
  timerFDs = fds;
  
  for (i = 0; i < NUM_OF_TIMERS; i++)
  {
    fds[i].fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    
    if (fds[i].fd == -1)
    {
      printf("Error creating timer\n");
      exit(-3);
    }
    else
    {
      printf("Timer created OK\n");
    }
  }
  
  fds[TIMEOUT_TIMER].callback = zbSocTimeoutCallback;
  fds[REPORTING_TIMER].callback = zbSocSblReportingCallback;
}


/*********************************************************************
 * @fn      zbSocNVWrite
 *
 * @brief   Send data for an NV item to the ZigBee SoC.
 *
 * @param   id - NV item id
 * @param   offset - Offset in the item to write at
 * @param   len - length of the data in byte
 * @param   data - data to write
 *
 * @return  none
 */
void zbSocNVWrite(uint16_t id, uint8_t offset, uint8_t len, uint8_t *data)
{
  uint8_t *cmd;

//  printf("zbSocNVWrite: id = %d, offset = %d, len = %d\n", id, offset, len);

  cmd = malloc(len + 9);

  cmd[0] = 0xFE;
  cmd[1] = len + 4;
  cmd[2] = MT_RPC_CMD_SREQ | MT_RPC_SYS_SYS;
  cmd[3] = MT_SYS_OSAL_NV_WRITE;
  cmd[4] = (id & 0x00ff); // LSB of id
  cmd[5] = (id & 0xff00) >> 8;  // MSG of id
  cmd[6] = offset;
  cmd[7] = len;
  memcpy(&cmd[8], data, len);
  cmd[8 + len] = 0x00; // FCS - fill in later
      
	calcFcs(cmd, len + 9);
	
  zbSocTransportWrite(cmd, len + 9);
  free(cmd);
}

/*********************************************************************
 * @fn      zbSocInstallCertificate
 *
 * @brief   Parse a certificate file and send data to the IPD SoC.
 *
 * @param   filename: name of the certificate file
 *
 * @return  none
 */
uint8_t zbSocInitiateCertInstall(char *filename, uint8_t force2reset)

{
  FILE *certFile;
  char csLine[MAX_CERT_LINE_LEN];
  char csLabel[MAX_CERT_LINE_LEN];
  char csValue[MAX_CERT_LINE_LEN];
  uint8_t status;

	certFile = fopen(filename, "r");
	if (certFile == NULL)
	{
    printf("Could not open file: %s\n", filename);
		return CERT_RESULT_ERROR_OPENING_FILE;
	}

  status = SUCCESS;

  memset(&zbSocCertInfo, 0, sizeof(zbSocCertInfo));
  
  while (fgets(csLine, MAX_CERT_LINE_LEN, certFile) != NULL)
  {
    int pos;
    int i, j;
  	
//    printf("zbSocInitiateCertInstall: current line = %s\n", csLine);
    // Skip empty lines and lines starting with space
    if ((strlen(csLine) > 0) && (csLine[0] != ' '))
    {
//      printf("zbSocInitiateCertInstall: strlen(csLine) = %d\n", strlen(csLine));
      for (pos = 0; (pos < strlen(csLine)) && (csLine[pos] != ':'); pos++) {}
      if (pos >= strlen(csLine))
      {
//        status = CERT_RESULT_WRONG_FORMAT;
//        break;
          continue;
      }
      
      // CString.Left()
      memcpy(csLabel, csLine, pos);
      csLabel[pos] = 0;
//      printf("zbSocInitiateCertInstall: csLabel = %s\n", csLabel);

      // CString.TrimLeft()
      for (i = 0; (i < strlen(csLabel)) && (!isalpha(csLabel[i])); i++) {}
      for (j = i; j < strlen(csLabel); j++)
      {
        csLabel[j - i] = csLabel[j];
      }
      csLabel[strlen(csLabel) - i] = 0;

      // CString.TrimRight()
      for (i = strlen(csLabel) - 1; (i >= 0) && (!isalpha(csLabel[i])); i--) {}
      csLabel[i + 1] = 0;

      // CString.Mid()
      strcpy(csValue, &csLine[pos + 1]);
//      printf("zbSocInitiateCertInstall: csValue = %s\n", csValue);

      // CString.TrimLeft()
      for (i = 0; (i < strlen(csValue)) && (!isxdigit(csValue[i])); i++) {}
      for (j = i; j < strlen(csValue); j++)
      {
        csValue[j - i] = csValue[j];
      }
      csValue[strlen(csValue) - i] = 0;

      // CString.TrimRight()
      for (i = strlen(csValue) - 1; (i >= 0) && (!isxdigit(csValue[i])); i--) {}
      csValue[i + 1] = 0;

//      printf("zbSocInitiateCertInstall: %s = %s\n", csLabel, csValue);

      if (!strcmp(csLabel, "IEEE Address"))
      {
        uint8_t temp;

        if ((status = hexStr2Array(csValue, Z_EXTADDR_LEN, zbSocCertInfo.ieeeAddr)) != SUCCESS)
        {
          break;
        }

        for (i = 0; i < Z_EXTADDR_LEN / 2; i++)
        {
          temp = zbSocCertInfo.ieeeAddr[i];
          zbSocCertInfo.ieeeAddr[i] = zbSocCertInfo.ieeeAddr[Z_EXTADDR_LEN - i - 1];
          zbSocCertInfo.ieeeAddr[Z_EXTADDR_LEN - i - 1] = temp;
        }
      }

      if (!strcmp(csLabel, "Device Implicit Cert"))
      {
        if ((status = hexStr2Array(csValue, ZCL_KE_IMPLICIT_CERTIFICATE_LEN, zbSocCertInfo.implicitCert)) != SUCCESS)
        {
          break;
        }
      }
     
      if (!strcmp(csLabel, "CA Pub Key"))
      {
        if ((status = hexStr2Array(csValue, ZCL_KE_CA_PUBLIC_KEY_LEN, zbSocCertInfo.caPublicKey)) != SUCCESS)
        {
          break;
        }
      }

      if (!strcmp(csLabel, "Device Private Key"))
      {
        if ((status = hexStr2Array(csValue, ZCL_KE_DEVICE_PRIVATE_KEY_LEN, zbSocCertInfo.devPrivateKey)) != SUCCESS)
        {
          break;
        }
      }
    } 
  }

  fclose(certFile);

  if (status == SUCCESS)
  {
    int i;
    
    for (i = 0; i < sizeof(zbSocCertInfo); i++)
    {
      if (*(((uint8_t *) &zbSocCertInfo) + i))
      {
        break;
      }
    }

    if (i < sizeof(zbSocCertInfo))
    {
      printf("zbSocInitiateCertInstall: Successfully retrieved certificate information.\n");
      zbSocCertForce2Reset = force2reset;
      zbSocCertState = ZBSOC_CERT_STATE_IEEE_ADDRESS;
      processCertInstall(ZBSOC_CERT_OPER_START);
    }
    else
    {
      printf("zbSocInitiateCertInstall: No valid information in the certificate.\n");
      status = CERT_RESULT_WRONG_FORMAT;
    }
}

	return status;
}

/*********************************************************************
 * @fn      zbSocSblInitiateImageDownload
 *
 * @brief   
 *
 * @param   
 *
 * @return  
 */
uint8_t zbSocSblInitiateImageDownload(char * filename, uint8_t progressReportingInterval)
{
	
	printf("loading file: %s\n", filename);
	
	zbSocSblImageFile = fopen(filename, "rb");
	if (zbSocSblImageFile == NULL)
	{
		return FAILURE;
	}
	zbSocSblProgressReportingInterval = progressReportingInterval;

	if (zbSocSblProgressReportingInterval > 0)
	{
		zbSocEnableTimeout(REPORTING_TIMER, zbSocSblProgressReportingInterval * 100);
	}

	zbSocSblState = ZBSOC_SBL_STATE_HANDSHAKING;
	timeout_retries = MAX_TIMEOUT_RETRIES;
	processRpcSysSbl(NULL);

	return SUCCESS;
}

/*********************************************************************
 * @fn      zbSocDisableTimeout
 *
 * @brief   
 *
 * @param   
 *
 * @return  
 */
void zbSocDisableTimeout(int timer)
{
	struct itimerspec its;

	its.it_interval.tv_sec = 0;
	its.it_interval.tv_nsec = 0;
	its.it_value.tv_sec = 0;
	its.it_value.tv_nsec = 0;
	
		if (timerfd_settime(timerFDs[timer].fd, 0, &its, NULL) != 0)
	{
			printf("Error disabling timer %d\n", timer);
			exit (0); //todo: assert
	}
}

/*********************************************************************
 * @fn      zbSocEnableTimeout
 *
 * @brief   
 *
 * @param   
 *
 * @return  
 */
void zbSocEnableTimeout(int timer, uint32_t milliseconds)
{
	struct itimerspec its;

	printf("zbSocEnableTimeout #%d\n",timer);

	its.it_interval.tv_sec = 0;
	its.it_interval.tv_nsec = 0;
	its.it_value.tv_sec = (milliseconds*1000000) / 1000000000;;
	its.it_value.tv_nsec = (milliseconds*1000000) % 1000000000;
	
	if (timerfd_settime(timerFDs[timer].fd, 0, &its, NULL) != 0)
	{
		printf("Error setting timer %d\n", timer);
		exit (0); //todo: assert
		}
	}
	

/*********************************************************************
 * @fn      zbSocFinishLoadingImage
 *
 * @brief   
 *
 * @param   
 *
 * @return  
 */
void zbSocFinishLoadingImage(void)
{
	if (zbSocSblImageFile != NULL)
	{
		fclose(zbSocSblImageFile);
	}
	
	zbSocSblState = ZBSOC_SBL_STATE_IDLE;

	zbSocDisableTimeout(TIMEOUT_TIMER);
	
	if (zbSocSblProgressReportingInterval > 0)
	{
		zbSocSblReportingPending = FALSE;
		zbSocSblProgressReportingInterval = 0;
		zbSocDisableTimeout(REPORTING_TIMER);
	}
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
  	uint8_t cmd[] = {
  		0xFE,                                                                                                                                                                                    
  		14,   //RPC payload Len
  		0x29, //MT_RPC_CMD_AREQ + MT_RPC_SYS_APP
  		0x00, //MT_APP_MSG
  		0x0B, //Application Endpoint
  		(dstAddr & 0x00ff),
  		(dstAddr & 0xff00) >> 8,
  		endpoint, //Dst EP
  		(ZCL_CLUSTER_ID_GEN_LEVEL_CONTROL & 0x00ff),
  		(ZCL_CLUSTER_ID_GEN_LEVEL_CONTROL & 0xff00) >> 8,      
  		0x07, //Data Len
  		addrMode, 
  		0x01, //0x01 ZCL frame control field.  (send to the light cluster only)
  		transSeqNumber++,
  		COMMAND_LEVEL_MOVE_TO_LEVEL,
  		(level & 0xff),
  		(time & 0xff),
  		(time & 0xff00) >> 8,
  		0x00       //FCS - fill in later
  	};    
    
    calcFcs(cmd, sizeof(cmd));
    zbSocTransportWrite(cmd,sizeof(cmd));
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
	uint8_t cmd[] = {
		0xFE,                                                                                                                                                                                    
		15,   //RPC payload Len          
		0x29, //MT_RPC_CMD_AREQ + MT_RPC_SYS_APP          
		0x00, //MT_APP_MSG          
		0x0B, //Application Endpoint          
		(dstAddr & 0x00ff),
		(dstAddr & 0xff00) >> 8,
		endpoint, //Dst EP
		(ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL & 0x00ff),
		(ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL & 0xff00) >> 8,     		   
		0x08, //Data Len
		addrMode, 
		0x01, //0x01 ZCL frame control field.  (send to the light cluster only)
		transSeqNumber++,
		COMMAND_LIGHTING_MOVE_TO_HUE,
		(hue & 0xff),
		0x00, //Move with shortest distance
		(time & 0xff),
		(time & 0xff00) >> 8,
		0x00       //FCS - fill in later
	};    

  calcFcs(cmd, sizeof(cmd));
  zbSocTransportWrite(cmd,sizeof(cmd));
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
  uint8_t cmd[] = {
		0xFE,                                                                                                                                                                                    
		14,   //RPC payload Len          
		0x29, //MT_RPC_CMD_AREQ + MT_RPC_SYS_APP         
		0x00, //MT_APP_MSG          
		0x0B, //Application Endpoint          
		(dstAddr & 0x00ff),
		(dstAddr & 0xff00) >> 8,
		endpoint, //Dst EP         
  	(ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL & 0x00ff),
  	(ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL & 0xff00) >> 8,
		0x07, //Data Len
		addrMode, 
		0x01, //0x01 ZCL frame control field.  (send to the light cluster only)
    transSeqNumber++,
		COMMAND_LIGHTING_MOVE_TO_SATURATION,
		(sat & 0xff),
		(time & 0xff),
		(time & 0xff00) >> 8,
		0x00       //FCS - fill in later
	};
	
	calcFcs(cmd, sizeof(cmd));
  zbSocTransportWrite(cmd,sizeof(cmd));
}


/*********************************************************************
 * @fn      zbSocSblSendMtFrame
 *
 * @brief  
 *
 * @param   
 *
 * @return  
 */
void zbSocSblSendMtFrame(uint8_t cmd, uint8_t * payload, uint8_t payload_len)
{
	uint8_t buf[ZBSOC_SBL_MAX_FRAME_SIZE];
	
	buf[0] = 0xFE;
	buf[1] = payload_len;
	buf[2] = SB_RPC_CMD_AREQ | MT_RPC_SYS_SBL;
	buf[3] = cmd; 

	if (payload_len > 0)
	{
		memcpy(buf + 4, payload, payload_len);
	}
	calcFcs(buf, payload_len + 5);
	zbSocTransportWrite(buf, payload_len + 5);
}


/*********************************************************************
 * @fn      zbSocResetLocalDevice
 *
 * @brief  
 *
 * @param   
 *
 * @return  none
 */
void zbSocResetLocalDevice(void)
{
  uint8_t cmd[] = {
		0xFE,
		1,   //RPC payload Len          
		SB_RPC_CMD_AREQ | MT_RPC_SYS_SYS,      
		MT_SYS_RESET_REQ,         
		0x01, //activate serial bootloader
		0x00       //FCS - fill in later
	};
	
	calcFcs(cmd, sizeof(cmd));
  zbSocTransportWrite(cmd,sizeof(cmd));
}


/*********************************************************************
 * @fn      zbSocSblExecuteImage
 *
 * @brief  
 *
 * @param   
 *
 * @return  none
 */
void zbSocSblExecuteImage(void)
{
	zbSocSblSendMtFrame(SB_ENABLE_CMD, NULL, 0);
	zbSocEnableTimeout(TIMEOUT_TIMER, BOOTLOADER_TIMEOUT);
}


/*********************************************************************
 * @fn      zbSocSblHandshake
 *
 * @brief  
 *
 * @param   
 *
 * @return  none
 */
void zbSocSblHandshake(void)
{
	uint8_t payload[] = {SB_MB_WAIT_HS};
	
	zbSocSblSendMtFrame(SB_HANDSHAKE_CMD, payload, sizeof(payload));
	zbSocEnableTimeout(TIMEOUT_TIMER, BOOTLOADER_TIMEOUT);
}


/*********************************************************************
 * @fn      zbSocSblEnableBootloader
 *
 * @brief  
 *
 * @param   
 *
 * @return  none
 */
void zbSocSblEnableBootloader()
{
	uint8_t cmd[] = {
		SB_FORCE_BOOT,
	};
	
	zbSocTransportWrite(cmd,sizeof(cmd));
}


/*********************************************************************
 * @fn      zbSocRemoveDevice
 *
 * @brief  
 *
 * @param   
 *
 * @return  none
 */
void zbSocRemoveDevice(uint8_t ieeeAddr[])
{
	  uint8_t cmd[] = { 
		  0xFE, 
		  Z_EXTADDR_LEN, //RPC payload Len
		  0x43, //MT_RPC_CMD_AREQ + MT_RPC_SYS_NWK
		  MT_NLME_LEAVE_REQ,
		  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		  0x00 //fcs
	}; 

	memcpy(cmd + 4, ieeeAddr, Z_EXTADDR_LEN);
	
	calcFcs(cmd, sizeof(cmd));
	zbSocTransportWrite(cmd,sizeof(cmd));	
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
	uint8_t cmd[] = { 
		0xFE, 
		15, //RPC payload Len
		0x29, //MT_RPC_CMD_AREQ + MT_RPC_SYS_APP
		0x00, //MT_APP_MSG
		0x0B, //Application Endpoint         
		(dstAddr & 0x00ff),
		(dstAddr & 0xff00) >> 8,
		endpoint, //Dst EP
  	(ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL & 0x00ff),
  	(ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL & 0xff00) >> 8,
		0x08, //Data Len
    addrMode,
		0x01, //ZCL Header Frame Control
		transSeqNumber++,
		0x06, //ZCL Header Frame Command (COMMAND_LEVEL_MOVE_TO_HUE_AND_SAT)
		hue, //HUE - fill it in later
		sat, //SAT - fill it in later
		(time & 0xff),
		(time & 0xff00) >> 8,
		0x00 //fcs
  }; 

  calcFcs(cmd, sizeof(cmd));
  zbSocTransportWrite(cmd,sizeof(cmd));       
}

/*********************************************************************
 * @fn      zbSocAddGroup
 *
 * @brief   Add Group.
 *
 * @param   groupId - Group ID of the Scene.
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be controled.
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast. 
 *
 * @return  none
 */
void zbSocAddGroup(uint16_t groupId, uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode)
{  
	uint8_t cmd[] = {
		0xFE,                                                                                      
		14,   /*RPC payload Len */          
		0x29, /*MT_RPC_CMD_AREQ + MT_RPC_SYS_APP */          
		0x00, /*MT_APP_MSG  */          
		0x0B, /*Application Endpoint */          
		(dstAddr & 0x00ff),
		(dstAddr & 0xff00) >> 8,
		endpoint, /*Dst EP */          
		(ZCL_CLUSTER_ID_GEN_GROUPS & 0x00ff),
		(ZCL_CLUSTER_ID_GEN_GROUPS & 0xff00) >> 8,
		0x07, //Data Len
		addrMode, 
		0x01, //0x01 ZCL frame control field.  (send to the light cluster only)
		transSeqNumber++,
		COMMAND_GROUP_ADD,
 		(groupId & 0x00ff),
		(groupId & 0xff00) >> 8, 
		0, //Null group name - Group Name not pushed to the devices	
		0x00       //FCS - fill in later
	};
	
	printf("zbSocAddGroup: dstAddr 0x%x\n", dstAddr);
    
	calcFcs(cmd, sizeof(cmd));
  zbSocTransportWrite(cmd,sizeof(cmd));
}

/*********************************************************************
 * @fn      zbSocStoreScene
 *
 * @brief   Store Scene.
 * 
 * @param   groupId - Group ID of the Scene.
 * @param   sceneId - Scene ID of the Scene.
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be controled.
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast. 
 *
 * @return  none
 */
void zbSocStoreScene(uint16_t groupId, uint8_t sceneId, uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode)
{  
	uint8_t cmd[] = {
		0xFE,                                                                                      
		14,   /*RPC payload Len */          
		0x29, /*MT_RPC_CMD_AREQ + MT_RPC_SYS_APP */          
		0x00, /*MT_APP_MSG  */          
		0x0B, /*Application Endpoint */          
		(dstAddr & 0x00ff),
		(dstAddr & 0xff00) >> 8,
		endpoint, /*Dst EP */          
		(ZCL_CLUSTER_ID_GEN_SCENES & 0x00ff),
		(ZCL_CLUSTER_ID_GEN_SCENES & 0xff00) >> 8,
		0x07, //Data Len
		addrMode, 
		0x01, //0x01 ZCL frame control field.  (send to the light cluster only)
		transSeqNumber++,
		COMMAND_SCENE_STORE,
 		(groupId & 0x00ff),
		(groupId & 0xff00) >> 8, 	
		sceneId++,	
		0x00       //FCS - fill in later
	};
    
	calcFcs(cmd, sizeof(cmd));
  zbSocTransportWrite(cmd,sizeof(cmd));
}

/*********************************************************************
 * @fn      zbSocRecallScene
 *
 * @brief   Recall Scene.
 *
 * @param   groupId - Group ID of the Scene.
 * @param   sceneId - Scene ID of the Scene.
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be controled. 
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast. 
 
 * @return  none
 */
void zbSocRecallScene(uint16_t groupId, uint8_t sceneId, uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode)
{  
	uint8_t cmd[] = {
		0xFE,                                                                                      
		14,   /*RPC payload Len */          
		0x29, /*MT_RPC_CMD_AREQ + MT_RPC_SYS_APP */          
		0x00, /*MT_APP_MSG  */          
		0x0B, /*Application Endpoint */          
		(dstAddr & 0x00ff),
		(dstAddr & 0xff00) >> 8,
		endpoint, /*Dst EP */          
		(ZCL_CLUSTER_ID_GEN_SCENES & 0x00ff),
		(ZCL_CLUSTER_ID_GEN_SCENES & 0xff00) >> 8,
		0x07, //Data Len
		addrMode, 
		0x01, //0x01 ZCL frame control field.  (send to the light cluster only)
		transSeqNumber++,
		COMMAND_SCENE_RECALL,
 		(groupId & 0x00ff),
		(groupId & 0xff00) >> 8, 	
		sceneId++,	
		0x00       //FCS - fill in later
	};
    
	calcFcs(cmd, sizeof(cmd));
  zbSocTransportWrite(cmd,sizeof(cmd));
}

/*********************************************************************
 * @fn      zbSocBind
 *
 * @brief   Recall Scene.
 *
 * @param   
 *
 * @return  none
 */
void zbSocBind(uint16_t srcNwkAddr, uint8_t srcEndpoint, uint8_t srcIEEE[8], uint8_t dstEndpoint, uint8_t dstIEEE[8], uint16_t clusterID )
{  
	uint8_t cmd[] = {
		0xFE,                                                                                      
		23,                           /*RPC payload Len */          
		0x25,                         /*MT_RPC_CMD_SREQ + MT_RPC_SYS_ZDO */        
		0x21,                         /*MT_ZDO_BIND_REQ*/        
  	(srcNwkAddr & 0x00ff),        /*Src Nwk Addr - To send the bind message to*/
  	(srcNwkAddr & 0xff00) >> 8,   /*Src Nwk Addr - To send the bind message to*/
  	srcIEEE[0],                   /*Src IEEE Addr for the binding*/
  	srcIEEE[1],                   /*Src IEEE Addr for the binding*/
  	srcIEEE[2],                   /*Src IEEE Addr for the binding*/
  	srcIEEE[3],                   /*Src IEEE Addr for the binding*/
  	srcIEEE[4],                   /*Src IEEE Addr for the binding*/
  	srcIEEE[5],                   /*Src IEEE Addr for the binding*/
  	srcIEEE[6],                   /*Src IEEE Addr for the binding*/
  	srcIEEE[7],                   /*Src IEEE Addr for the binding*/ 	
  	srcEndpoint,                  /*Src endpoint for the binding*/ 
  	(clusterID & 0x00ff),         /*cluster ID to bind*/
  	(clusterID & 0xff00) >> 8,    /*cluster ID to bind*/  	
  	afAddr64Bit,                    /*Addr mode of the dst to bind*/
  	dstIEEE[0],                   /*Dst IEEE Addr for the binding*/
  	dstIEEE[1],                   /*Dst IEEE Addr for the binding*/
  	dstIEEE[2],                   /*Dst IEEE Addr for the binding*/
  	dstIEEE[3],                   /*Dst IEEE Addr for the binding*/
  	dstIEEE[4],                   /*Dst IEEE Addr for the binding*/
  	dstIEEE[5],                   /*Dst IEEE Addr for the binding*/
  	dstIEEE[6],                   /*Dst IEEE Addr for the binding*/
  	dstIEEE[7],                   /*Dst IEEE Addr for the binding*/ 	
  	dstEndpoint,                  /*Dst endpoint for the binding*/  	  	
		0x00       //FCS - fill in later
	};
      
	printf("zbSocBind: srcNwkAddr=0x%x, srcEndpoint=0x%x, srcIEEE=0x%x:%x:%x:%x:%x:%x:%x:%x, dstEndpoint=0x%x, dstIEEE=0x%x:%x:%x:%x:%x:%x:%x:%x, clusterID:%x\n", 
	          srcNwkAddr, srcEndpoint, srcIEEE[0], srcIEEE[1], srcIEEE[2], srcIEEE[3], srcIEEE[4], srcIEEE[5], srcIEEE[6], srcIEEE[7], 
	          srcEndpoint, dstIEEE[0], dstIEEE[1], dstIEEE[2], dstIEEE[3], dstIEEE[4], dstIEEE[5], dstIEEE[6], dstIEEE[7], clusterID);
	
	calcFcs(cmd, sizeof(cmd));		
  zbSocTransportWrite(cmd,sizeof(cmd));
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
  	uint8_t cmd[] = {
  		0xFE,                                                                                      
  		13,   /*RPC payload Len */          
  		0x29, /*MT_RPC_CMD_AREQ + MT_RPC_SYS_APP */          
  		0x00, /*MT_APP_MSG  */          
  		0x0B, /*Application Endpoint */          
  		(dstAddr & 0x00ff),
  		(dstAddr & 0xff00) >> 8,
  		endpoint, /*Dst EP */          
  		(ZCL_CLUSTER_ID_GEN_ON_OFF & 0x00ff),
  		(ZCL_CLUSTER_ID_GEN_ON_OFF & 0xff00) >> 8,
  		0x06, //Data Len
  		addrMode, 
  		0x00, //0x00 ZCL frame control field.  not specific to a cluster (i.e. a SCL founadation command)
  		transSeqNumber++,
  		ZCL_CMD_READ,
  		(ATTRID_ON_OFF & 0x00ff),
  		(ATTRID_ON_OFF & 0xff00) >> 8,
  		0x00       //FCS - fill in later
  	};
      
  	calcFcs(cmd, sizeof(cmd));
    zbSocTransportWrite(cmd,sizeof(cmd));
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
  	uint8_t cmd[] = {
  		0xFE,                                                                                      
  		13,   /*RPC payload Len */          
  		0x29, /*MT_RPC_CMD_AREQ + MT_RPC_SYS_APP */          
  		0x00, /*MT_APP_MSG  */          
  		0x0B, /*Application Endpoint */          
  		(dstAddr & 0x00ff),
  		(dstAddr & 0xff00) >> 8,
  		endpoint, /*Dst EP */          
  		(ZCL_CLUSTER_ID_GEN_LEVEL_CONTROL & 0x00ff),
  		(ZCL_CLUSTER_ID_GEN_LEVEL_CONTROL & 0xff00) >> 8,
  		0x06, //Data Len
  		addrMode, 
  		0x00, //0x00 ZCL frame control field.  not specific to a cluster (i.e. a SCL founadation command)
  		transSeqNumber++,
  		ZCL_CMD_READ,
  		(ATTRID_LEVEL_CURRENT_LEVEL & 0x00ff),
  		(ATTRID_LEVEL_CURRENT_LEVEL & 0xff00) >> 8,
  		0x00       //FCS - fill in later
  	};
      
  	calcFcs(cmd, sizeof(cmd));
  	zbSocTransportWrite(cmd,sizeof(cmd));
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
  	uint8_t cmd[] = {
  		0xFE,                                                                                      
  		13,   /*RPC payload Len */          
  		0x29, /*MT_RPC_CMD_AREQ + MT_RPC_SYS_APP */          
  		0x00, /*MT_APP_MSG  */          
  		0x0B, /*Application Endpoint */          
  		(dstAddr & 0x00ff),
  		(dstAddr & 0xff00) >> 8,
  		endpoint, /*Dst EP */          
  		(ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL & 0x00ff),
  		(ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL & 0xff00) >> 8,
  		0x06, //Data Len
  		addrMode, 
  		0x00, //0x00 ZCL frame control field.  not specific to a cluster (i.e. a SCL founadation command)
  		transSeqNumber++,
  		ZCL_CMD_READ,
  		(ATTRID_LIGHTING_COLOR_CONTROL_CURRENT_HUE & 0x00ff),
  		(ATTRID_LIGHTING_COLOR_CONTROL_CURRENT_HUE & 0xff00) >> 8,
  		0x00       //FCS - fill in later
  	};
      
  	calcFcs(cmd, sizeof(cmd));
  	zbSocTransportWrite(cmd,sizeof(cmd));
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
  	uint8_t cmd[] = {
  		0xFE,                                                                                      
  		13,   /*RPC payload Len */          
  		0x29, /*MT_RPC_CMD_AREQ + MT_RPC_SYS_APP */          
  		0x00, /*MT_APP_MSG  */          
  		0x0B, /*Application Endpoint */          
  		(dstAddr & 0x00ff),
  		(dstAddr & 0xff00) >> 8,
  		endpoint, /*Dst EP */          
  		(ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL & 0x00ff),
  		(ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL & 0xff00) >> 8,
  		0x06, //Data Len
  		addrMode, 
  		0x00, //0x00 ZCL frame control field.  not specific to a cluster (i.e. a SCL founadation command)
  		transSeqNumber++,
  		ZCL_CMD_READ,
  		(ATTRID_LIGHTING_COLOR_CONTROL_CURRENT_SATURATION & 0x00ff),
  		(ATTRID_LIGHTING_COLOR_CONTROL_CURRENT_SATURATION & 0xff00) >> 8,
  		0x00       //FCS - fill in later
  	};
      
  	calcFcs(cmd, sizeof(cmd));
  	zbSocTransportWrite(cmd,sizeof(cmd));
} 
/*********************************************************************
 * @fn      zbSocPowerRead
 *
 * @brief   Send the read command to the ESI with regards to Instantaneous Demand.
 *
 * @param   dstAddr - Nwk Addr of the ESI to be sent the command.
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast.
 *
 * @return  none
 */
void zbSocReadPower(uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode)
{
  	uint8_t cmd[] = {
  		0xFE,                                                                                      
  		13,   /*RPC payload Len */          
  		0x29, /*MT_RPC_CMD_AREQ + MT_RPC_SYS_APP */          
  		0x00, /*MT_APP_MSG  */          
  		0x0B, /*Application Endpoint */          
  		(dstAddr & 0x00ff),
  		(dstAddr & 0xff00) >> 8,
  		endpoint, /*Dst EP */          
  		(ZCL_CLUSTER_ID_SE_SIMPLE_METERING & 0x00ff),
  		(ZCL_CLUSTER_ID_SE_SIMPLE_METERING & 0xff00) >> 8,
  		0x06, //Data Len
  		addrMode, 
  		0x00, //0x00 ZCL frame control field.  not specific to a cluster (i.e. a SCL founadation command)
  		transSeqNumber++,
  		ZCL_CMD_READ,
  		(ATTRID_SE_INSTANTANEOUS_DEMAND & 0x00ff),
  		(ATTRID_SE_INSTANTANEOUS_DEMAND & 0xff00) >> 8,
  		0x00       //FCS - fill in later
  	};
      
  	calcFcs(cmd, sizeof(cmd));
  	
    zbSocTransportWrite(cmd,sizeof(cmd));
} 

/*********************************************************************
 * @fn      zbSocGetTemp
 *
 * @brief   Send the get saturation command to a ZLL light.
 *
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be sent the command.
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast.
 *
 * @return  none
 */
void zbSocGetTemp(uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode)
{
  	uint8_t cmd[] = {
  		0xFE,                                                                                      
  		13,   /*RPC payload Len */          
  		0x29, /*MT_RPC_CMD_AREQ + MT_RPC_SYS_APP */          
  		0x00, /*MT_APP_MSG  */          
  		0x0B, /*Application Endpoint */          
  		(dstAddr & 0x00ff),
  		(dstAddr & 0xff00) >> 8,
  		endpoint, /*Dst EP */          
  		(ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT & 0x00ff),
  		(ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT & 0xff00) >> 8,
  		0x06, //Data Len
  		addrMode, 
  		0x00, //0x00 ZCL frame control field.  not specific to a cluster (i.e. a SCL founadation command)
  		transSeqNumber++,
  		ZCL_CMD_READ,
  		(ATTRID_MS_TEMPERATURE_MEASURED_VALUE & 0x00ff),
  		(ATTRID_MS_TEMPERATURE_MEASURED_VALUE & 0xff00) >> 8,
  		0x00       //FCS - fill in later
  	};
      
  	calcFcs(cmd, sizeof(cmd));
    zbSocTransportWrite(cmd,sizeof(cmd));
} 

/*********************************************************************
 * @fn      zbSocGetHumid
 *
 * @brief   Send the get saturation command to a ZLL light.
 *
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be sent the command.
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast.
 *
 * @return  none
 */
void zbSocGetHumid(uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode)
{
  	uint8_t cmd[] = {
  		0xFE,                                                                                      
  		13,   /*RPC payload Len */          
  		0x29, /*MT_RPC_CMD_AREQ + MT_RPC_SYS_APP */          
  		0x00, /*MT_APP_MSG  */          
  		0x0B, /*Application Endpoint */          
  		(dstAddr & 0x00ff),
  		(dstAddr & 0xff00) >> 8,
  		endpoint, /*Dst EP */          
  		(ZCL_CLUSTER_ID_MS_REL_HUMIDITY_MEASUREMENT & 0x00ff),
  		(ZCL_CLUSTER_ID_MS_REL_HUMIDITY_MEASUREMENT & 0xff00) >> 8,
  		0x06, //Data Len
  		addrMode, 
  		0x00, //0x00 ZCL frame control field.  not specific to a cluster (i.e. a SCL founadation command)
  		transSeqNumber++,
  		ZCL_CMD_READ,
  		(ATTRID_MS_RELATIVE_HUMIDITY_MEASURED_VALUE & 0x00ff),
  		(ATTRID_MS_RELATIVE_HUMIDITY_MEASURED_VALUE & 0xff00) >> 8,
  		0x00       //FCS - fill in later
  	};
      
  	calcFcs(cmd, sizeof(cmd));
  	
    zbSocTransportWrite(cmd,sizeof(cmd));
}


/*********************************************************************
 * @fn      zbSocGetLastMessage
 *
 * @brief   Send the GetLastMessage command to the ESI.
 *
 * @return  none
 */
void zbSocGetLastMessage(uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode)
{
  	uint8_t cmd[] = {
  		0xFE,                                                                                                                                                                                    
  		11,   //RPC payload Len
  		0x29, //MT_RPC_CMD_AREQ + MT_RPC_SYS_APP
  		0x00, //MT_APP_MSG
  		0x09, //Application Endpoint
  		(dstAddr & 0x00ff),
  		(dstAddr & 0xff00) >> 8,
  		endpoint, //Dst EP
  		(ZCL_CLUSTER_ID_SE_MESSAGE & 0x00ff),
  		(ZCL_CLUSTER_ID_SE_MESSAGE & 0xff00) >> 8,      
  		0x04, //Data Len
  		addrMode, 
  		0x01, //0x01 ZCL frame control field.  (send to the light cluster only)
  		transSeqNumber++,
  		COMMAND_SE_GET_LAST_MESSAGE,
  		0x00       //FCS - fill in later
  	};    
    
    calcFcs(cmd, sizeof(cmd));
    
    zbSocTransportWrite(cmd,sizeof(cmd));
}

/*********************************************************************
 * @fn      zbSocGetCurrentPrice
 *
 * @brief   Send the GetCurrentPrice command to the ESI.
 *
 * @return  none
 */
void zbSocGetCurrentPrice(uint8_t rxOnIdle, uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode)
{
  	uint8_t cmd[] = {
  		0xFE,                                                                                                                                                                                    
  		12,   //RPC payload Len
  		0x29, //MT_RPC_CMD_AREQ + MT_RPC_SYS_APP
  		0x00, //MT_APP_MSG
  		0x09, //Application Endpoint
  		(dstAddr & 0x00ff),
  		(dstAddr & 0xff00) >> 8,
  		endpoint, //Dst EP
  		(ZCL_CLUSTER_ID_SE_PRICING & 0x00ff),
  		(ZCL_CLUSTER_ID_SE_PRICING & 0xff00) >> 8,      
  		0x05, //Data Len
  		addrMode, 
  		0x01, //0x01 ZCL frame control field.  (send to the light cluster only)
  		transSeqNumber++,
  		COMMAND_SE_GET_CURRENT_PRICE,
  		rxOnIdle,
  		0x00       //FCS - fill in later
  	};    
    
    calcFcs(cmd, sizeof(cmd));

    zbSocTransportWrite(cmd,sizeof(cmd));
}

/*************************************************************************************************
 * @fn      processRpcSysAppTlInd()
 *
 * @brief  process the TL Indication from the ZLL controller
 *
 * @param   none
 *
 * @return  length of current Rx Buffer
 *************************************************************************************************/
static void processRpcSysAppTlInd(uint8_t *TlIndBuff)
{
  epInfo_t epInfo;    
    
  epInfo.nwkAddr = BUILD_UINT16(TlIndBuff[0], TlIndBuff[1]);
  TlIndBuff+=2;      
  epInfo.endpoint = *TlIndBuff++;
  epInfo.profileID = BUILD_UINT16(TlIndBuff[0], TlIndBuff[1]);
  TlIndBuff+=2;      
  epInfo.deviceID = BUILD_UINT16(TlIndBuff[0], TlIndBuff[1]);
  TlIndBuff+=2;   
  epInfo.version = *TlIndBuff++;
  epInfo.status = *TlIndBuff++;
  
  if(zbSocCb.pfnTlIndicationCb)
  {
    zbSocCb.pfnTlIndicationCb(&epInfo);
  }    
}        

/*************************************************************************************************
 * @fn      processRpcSysAppNewDevInd()
 *
 * @brief  process the New Device Indication from the ZLL controller
 *
 * @param   none
 *
 * @return  length of current Rx Buffer
 *************************************************************************************************/
static void processRpcSysAppNewDevInd(uint8_t *TlIndBuff)
{
  epInfo_t epInfo;
  uint8_t i;    
    
  memset(&epInfo, 0, sizeof(epInfo));
  
  epInfo.status = 0;
  
  epInfo.nwkAddr = BUILD_UINT16(TlIndBuff[0], TlIndBuff[1]);
  TlIndBuff+=2;      
  epInfo.endpoint = *TlIndBuff++;
  epInfo.profileID = BUILD_UINT16(TlIndBuff[0], TlIndBuff[1]);
  TlIndBuff+=2;      
  epInfo.deviceID = BUILD_UINT16(TlIndBuff[0], TlIndBuff[1]);
  if (epInfo.deviceID == 0x0100) //substitute Lighting: On/Off Light with Generic: Mains Power Outlet. It is required until support is added in the android app to handle the outlet that pretends to be a light
  {
  	epInfo.deviceID = 0x0009;
  }
  TlIndBuff+=2;   
  epInfo.version = *TlIndBuff++;
  epInfo.deviceName = NULL;
  
  for(i=0; i<8; i++)
  {
    epInfo.IEEEAddr[i] = *TlIndBuff++;
  }
  
  epInfo.flags = *TlIndBuff++;

//  printf("processRpcSysAppNewDevInd: %x:%x\n",  epInfo.nwkAddr, epInfo.endpoint);
  if(zbSocCb.pfnNewDevIndicationCb)
  {
    zbSocCb.pfnNewDevIndicationCb(&epInfo);
  }    
}

/*************************************************************************************************
 * @fn      processRpcSysAppKeyEstablishmentStateInd()
 *
 * @brief  process the Key Establishment State Ind from the IHD
 *
 * @param   none
 *
 * @return  length of current Rx Buffer
 *************************************************************************************************/
static void processRpcSysAppKeyEstablishmentStateInd(uint8_t *KeIndBuf)
{
//  printf("processRpcSysAppKeyEstablishmentStateInd: %x\n", KeIndBuf[0]);
  if(zbSocCb.pfnKeyEstablishmentStateIndCb)
  {
    zbSocCb.pfnKeyEstablishmentStateIndCb(KeIndBuf[0]);
  }    
}

/*************************************************************************************************
 * @fn      processRpcSysAppZcl()
 *
 * @brief  process the ZCL Rsp from the ZLL controller
 *
 * @param   none
 *
 * @return  length of current Rx Buffer
 *************************************************************************************************/
static void processRpcSysAppZcl(uint8_t *zclRspBuff)
{   
  uint8_t zclHdrLen = 3;
  uint16_t nwkAddr, clusterID; 
  uint8_t endpoint, appEP, zclFrameLen, zclFrameFrameControl;
    
//  printf("processRpcSysAppZcl++\n");
    
  //This is a ZCL response
  appEP = *zclRspBuff++;
  nwkAddr = BUILD_UINT16(zclRspBuff[0], zclRspBuff[1]);
  zclRspBuff+=2;

  endpoint = *zclRspBuff++;
  clusterID = BUILD_UINT16(zclRspBuff[0], zclRspBuff[1]);
  zclRspBuff+=2;

  zclFrameLen = *zclRspBuff++;
  zclFrameFrameControl = *zclRspBuff++;
  
  //is it manufacturer specific
  if ( zclFrameFrameControl & (1 <<2) )
  {
    //currently not supported shown for reference
    uint16_t ManSpecCode;
    //manu spec code
    ManSpecCode = BUILD_UINT16(zclRspBuff[0], zclRspBuff[1]);
    zclRspBuff+=2;
    //Manufacturer specif commands have 2 extra byte in te header
    zclHdrLen+=2;
  }      
  
  //is this a foundation command
  if( (zclFrameFrameControl & 0x3) == 0)
  {
//    printf("processRpcSysAppZcl: Foundation messagex\n");
    processRpcSysAppZclFoundation(zclRspBuff, zclFrameLen, clusterID, nwkAddr, endpoint);
  }
  else
  {
 //   printf("processRpcSysAppZcl: Cluster messagex\n");
    processRpcSysAppZclCluster(zclRspBuff, zclFrameLen, clusterID, nwkAddr, endpoint);
  }
}
    
/*************************************************************************************************
 * @fn      processRpcSysAppZclFoundation()
 *
 * @brief  process the ZCL Rsp from the ZLL controller
 *
 * @param   none
 *
 * @return  length of current Rx Buffer
 *************************************************************************************************/
static void processRpcSysAppZclFoundation(uint8_t *zclRspBuff, uint8_t zclFrameLen, uint16_t clusterID, uint16_t nwkAddr, uint8_t endpoint)
{
  uint8_t transSeqNum, commandID;
  
  transSeqNum = *zclRspBuff++;
  commandID = *zclRspBuff++;
  
  if(commandID == ZCL_CMD_READ_RSP)
  {
    uint16_t attrID;
    uint8_t status;
    uint8_t dataType;
    
    attrID = BUILD_UINT16(zclRspBuff[0], zclRspBuff[1]);      
    zclRspBuff+=2;
    status = *zclRspBuff++;
    //get data type;
    dataType = *zclRspBuff++;


//    printf("processRpcSysAppZclFoundation: clusterID:%x, attrID:%x, dataType=%x\n", clusterID, attrID, dataType);
    
    if( (clusterID == ZCL_CLUSTER_ID_GEN_ON_OFF) && (attrID == ATTRID_ON_OFF) && (dataType == ZCL_DATATYPE_BOOLEAN) )
    {              
      if(zbSocCb.pfnZclGetStateCb)
      {
        uint8_t state = zclRspBuff[0];            
        zbSocCb.pfnZclGetStateCb(state, nwkAddr, endpoint);
      }                       
    }
    else if( (clusterID == ZCL_CLUSTER_ID_GEN_LEVEL_CONTROL) && (attrID == ATTRID_LEVEL_CURRENT_LEVEL) && (dataType == ZCL_DATATYPE_UINT8) )
    {    
      if(zbSocCb.pfnZclGetLevelCb)
      {
        uint8_t level = zclRspBuff[0];                             
        zbSocCb.pfnZclGetLevelCb(level, nwkAddr, endpoint);
      }                       
    }
    else if( (clusterID == ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL) && (attrID == ATTRID_LIGHTING_COLOR_CONTROL_CURRENT_HUE) && (dataType == ZCL_DATATYPE_UINT8) )
    {
      if(zbSocCb.pfnZclGetHueCb)      
      {
        uint8_t hue = zclRspBuff[0];            
        zbSocCb.pfnZclGetHueCb(hue, nwkAddr, endpoint);
      }    
    }                   
    else if( (clusterID == ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL) && (attrID == ATTRID_LIGHTING_COLOR_CONTROL_CURRENT_SATURATION) && (dataType == ZCL_DATATYPE_UINT8) )
    {
      if(zbSocCb.pfnZclGetSatCb)      
      {
        uint8_t sat = zclRspBuff[0];            
        zbSocCb.pfnZclGetSatCb(sat, nwkAddr, endpoint);
      }    
    }
    else if( (clusterID == ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT) && (attrID == ATTRID_MS_TEMPERATURE_MEASURED_VALUE) && (dataType == ZCL_DATATYPE_INT16) )
    {
      if(zbSocCb.pfnZclGetTempCb)      
      {
        uint16_t temp;
        temp = BUILD_UINT16(zclRspBuff[0], zclRspBuff[1]);             
        zbSocCb.pfnZclGetTempCb(temp, nwkAddr, endpoint);
      }    
    }
    else if( (clusterID == ZCL_CLUSTER_ID_MS_REL_HUMIDITY_MEASUREMENT) && (attrID == ATTRID_MS_RELATIVE_HUMIDITY_MEASURED_VALUE) && (dataType == ZCL_DATATYPE_UINT16) )
    {
      if(zbSocCb.pfnZclGetHumidCb)      
      {
        uint16_t humid;        
        humid = BUILD_UINT16(zclRspBuff[0], zclRspBuff[1]);             
        zbSocCb.pfnZclGetHumidCb(humid, nwkAddr, endpoint);
      }    
    }    
    else if( (clusterID == ZCL_CLUSTER_ID_SE_SIMPLE_METERING) && (attrID == ATTRID_SE_INSTANTANEOUS_DEMAND) && (dataType == ZCL_DATATYPE_INT24) )
    {
      if(zbSocCb.pfnZclReadPowerRspCb)      
      {
        uint32_t power;  
        power = BUILD_UINT32(zclRspBuff[0], zclRspBuff[1], zclRspBuff[2], 0);            
        printf("processRpcSysAppZclFoundation: Power:%x, %x:%x:%x:%x\n", power, zclRspBuff[0], zclRspBuff[1], zclRspBuff[3], 0);
        zbSocCb.pfnZclReadPowerRspCb(power, nwkAddr, endpoint);
      }    
    }
    else                
    {
      //unsupported ZCL Read Rsp
      printf("processRpcSysAppZclFoundation: Unsupported ZCL Rsp\n");
    } 
  }
  else
  {
    //unsupported ZCL Rsp
    printf("processRpcSysAppZclFoundation: Unsupported ZCL Rsp");;
  }
  
  return;                    
}  
 
 /*************************************************************************************************
 * @fn      processRpcSysAppZclCluster()
 *
 * @brief  process the ZCL Rsp from the ZLL controller
 *
 * @param   none
 *
 * @return  length of current Rx Buffer
 *************************************************************************************************/
static void processRpcSysAppZclCluster(uint8_t *zclRspBuff, uint8_t zclFrameLen, uint16_t clusterID, uint16_t nwkAddr, uint8_t endpoint)
{
  uint8_t transSeqNum, commandID;
  uint8_t len;
  
  transSeqNum = *zclRspBuff++;
  commandID = *zclRspBuff++;
  len = zclFrameLen - 3; //len is frame len - 3byte ZCL header
  
//  printf("processRpcSysAppZclCluster: commandID=%x, len=%x\n", commandID, len); 
  
  if( clusterID == ZCL_CLUSTER_ID_SS_IAS_ZONE)
  {
    if(commandID == COMMAND_SS_IAS_ZONE_STATUS_CHANGE_NOTIFICATION)
    {
      if(zbSocCb.pfnZclZoneStateChangeCb)      
      {
        uint32_t zoneState;  
        zoneState = BUILD_UINT32(zclRspBuff[0], zclRspBuff[1], zclRspBuff[3], 0);            
        zbSocCb.pfnZclZoneStateChangeCb(zoneState, nwkAddr, endpoint);
      }
    }
    if(commandID == COMMAND_SS_IAS_ZONE_STATUS_ENROLL_REQUEST)
    {    
      
    }
  }
  else if (clusterID == ZCL_CLUSTER_ID_SE_MESSAGE)
  {
    if (commandID == COMMAND_SE_DISPLAY_MESSAGE)
    {
      if (zbSocCb.pfnZclDisplayMessageIndCb)
      {
        zbSocCb.pfnZclDisplayMessageIndCb(zclRspBuff, len);
      }
    }
  }
  else if (clusterID == ZCL_CLUSTER_ID_SE_PRICING)
  {
    if (commandID == COMMAND_SE_PUBLISH_PRICE)
    {
      if (zbSocCb.pfnZclPublishPriceIndCb)
      {
        zbSocCb.pfnZclPublishPriceIndCb(zclRspBuff, len);
      }
    }
  }
}

/*************************************************************************************************
 * @fn      processRpcSysSys()
 *
 * @brief   read and process the RPC SYS message from the ZigBee SoC
 *
 * @param   none
 *
 * @return  length of current Rx Buffer
 *************************************************************************************************/
static void processRpcSysSys(uint8_t *rpcBuff)
{
  if ( rpcBuff[1] == MT_SYS_OSAL_NV_WRITE )
  {
//    printf("processRpcSysSys: MT_SYS_OSAL_NV_WRITE response with erro code %d\n", rpcBuff[2]);
    processCertInstall(rpcBuff[2] ? ZBSOC_CERT_OPER_ERROR : ZBSOC_CERT_OPER_CONT);
  }
    
  return;   
}


/*************************************************************************************************
 * @fn      processRpcSysApp()
 *
 * @brief   read and process the RPC App message from the ZLL controller
 *
 * @param   none
 *
 * @return  length of current Rx Buffer
 *************************************************************************************************/
static void processRpcSysApp(uint8_t *rpcBuff)
{
  if( rpcBuff[1] == MT_APP_ZLL_TL_IND )
  {
    processRpcSysAppTlInd(&rpcBuff[2]);
  }          
  else if( rpcBuff[1] == MT_APP_NEW_DEV_IND )
  {
    processRpcSysAppNewDevInd(&rpcBuff[2]);
  }  
  else if ( rpcBuff[1] == MT_APP_KEY_ESTABLISHMENT_STATE_IND )
  {
    processRpcSysAppKeyEstablishmentStateInd(&rpcBuff[2]);
  }
  else if( rpcBuff[1] == MT_APP_RSP )
  {
    processRpcSysAppZcl(&rpcBuff[2]);
  }    
  else if( rpcBuff[1] == 0 )
  {
    if( rpcBuff[2] == 0)
    {
//      printf("processRpcSysApp: Command Received Successfully\n\n");
    }
    else
    {
      printf("processRpcSysApp: Command Error\n\n");
    }    
  }
  else
  {
    printf("processRpcSysApp: Unsupported MT App Msg\n");
  }
    
  return;   
}

/*************************************************************************************************
 * @fn      zbSocSblSendImageBlock()
 *
 * @brief   
 *
 * @param
 *
 * @return 
 *************************************************************************************************/
void zbSocSblSendImageBlock(uint8_t buf[], uint8_t size)
{
	zbSocSblSendMtFrame(SB_WRITE_CMD, buf, size);
	zbSocEnableTimeout(TIMEOUT_TIMER, BOOTLOADER_TIMEOUT);
}


/*************************************************************************************************
 * @fn      zbSocSblReadImageBlock()
 *
 * @brief   
 *
 * @param
 *
 * @return 
 *************************************************************************************************/
void zbSocSblReadImageBlock(uint32_t address)
{
	uint8_t payload[] = {(address / 4) & 0xFF, ((address / 4) >> 8) & 0xFF};
	zbSocSblSendMtFrame(SB_READ_CMD, payload, sizeof(payload));
	zbSocEnableTimeout(TIMEOUT_TIMER, BOOTLOADER_TIMEOUT);
}

/*************************************************************************************************
 * @fn      processRpcSysSbl()
 *
 * @brief   
 *
 * @param
 *
 * @return 
 *************************************************************************************************/
static void processRpcSysSbl(uint8_t *rpcBuff)
{
	static uint8_t buf[ZBSOC_SBL_IMAGE_BLOCK_SIZE + 2];
	
	static uint32_t zbSocCurrentImageBlockAddress;
	static uint16_t zbSocCurrentImageBlockTriesLeft;

	int bytes_read;
	uint8_t finish_code = SBL_TARGET_STILL_WORKING;
	uint8_t discard_current_frame = TRUE;
	uint8_t load_next_block_from_file = FALSE;

	if (rpcBuff == NULL)
	{
		discard_current_frame = FALSE; //not actually an incoming frame... this settings means that processing should continue
		
		if (timeout_retries-- == 0)
		{
			finish_code = SBL_COMMUNICATION_FAILED;
		}
	}
	else
	{
		switch (zbSocSblState)
		{
			case ZBSOC_SBL_STATE_IDLE:
				/* do nothing */
				break;
				
			case ZBSOC_SBL_STATE_HANDSHAKING:
				if (rpcBuff[RPC_BUF_CMD1] == (SB_HANDSHAKE_CMD | SB_RESPONSE))
				{
					discard_current_frame = FALSE;
					if (rpcBuff[RPC_BUF_INCOMING_RESULT] == SB_SUCCESS)
					{
						zbSocSblState = ZBSOC_SBL_STATE_PROGRAMMING;
						zbSocCurrentImageBlockAddress = 0x00000000;
						load_next_block_from_file = TRUE;
					}
					else
					{
						finish_code = SBL_HANDSHAKE_FAILED;
					}
				}
				break;
				
			case ZBSOC_SBL_STATE_PROGRAMMING:
				if (rpcBuff[RPC_BUF_CMD1] == (SB_WRITE_CMD | SB_RESPONSE))
				{
					discard_current_frame = FALSE;
					if (rpcBuff[RPC_BUF_INCOMING_RESULT] == SB_SUCCESS)
					{
						zbSocCurrentImageBlockAddress += ZBSOC_SBL_IMAGE_BLOCK_SIZE;
						load_next_block_from_file = TRUE;
					}
					else if (zbSocCurrentImageBlockTriesLeft-- == 0)
					{
						finish_code = SBL_TARGET_WRITE_FAILED;
					}
				}
				break;

			case ZBSOC_SBL_STATE_VERIFYING:
				if ((rpcBuff[RPC_BUF_CMD1] == (SB_READ_CMD | SB_RESPONSE)) && (memcmp(buf, rpcBuff + 3, 2) == 0)) //process only if the address reported is the requested address. otherwise - discard frame. It was observed that soometimes when switching from writing to reading, the first read response is sent twice. 
				{
					discard_current_frame = FALSE;
					if (rpcBuff[RPC_BUF_INCOMING_RESULT] == SB_SUCCESS)
					{
						if (memcmp(buf+2, rpcBuff + 5, ZBSOC_SBL_IMAGE_BLOCK_SIZE) == 0)
						{
							zbSocCurrentImageBlockAddress += ZBSOC_SBL_IMAGE_BLOCK_SIZE;
							load_next_block_from_file = TRUE;
						}
						else
						{
							finish_code = SBL_VERIFICATION_FAILED;
						}
					}
					else if (zbSocCurrentImageBlockTriesLeft-- == 0)
					{
						finish_code = SBL_TARGET_READ_FAILED;
					}
				}
				break;

			case ZBSOC_SBL_STATE_EXECUTING:
				if (rpcBuff[RPC_BUF_CMD1] == (SB_ENABLE_CMD | SB_RESPONSE))
				{
					discard_current_frame = FALSE;
					if (rpcBuff[RPC_BUF_INCOMING_RESULT] == SB_SUCCESS)
					{
						finish_code = SBL_SUCCESS;
					}
					else
					{
						finish_code = SBL_EXECUTION_FAILED;
					}
				}
				break;

			default:
				//unexpected error
				break;
		}
	}

	if (discard_current_frame)
	{
		return;
	}

	if (finish_code != SBL_TARGET_STILL_WORKING)
	{
		zbSocFinishLoadingImage();
		zbSocCb.pfnBootloadingDoneCb(finish_code);
		return;
	}

	while (load_next_block_from_file) //executed maximum 2 times. Usually 1 time. The second time is when reading past the end of the file in programming mode, and then going to read the block at the beginning of the file
	{
		load_next_block_from_file = FALSE;
		
		zbSocCurrentImageBlockTriesLeft = ZBSOC_SBL_MAX_RETRY_ON_ERROR_REPORTED;
		
		bytes_read = fread(buf + 2, 1, ZBSOC_SBL_IMAGE_BLOCK_SIZE, zbSocSblImageFile);
		
		if ((bytes_read < ZBSOC_SBL_IMAGE_BLOCK_SIZE) && (!feof(zbSocSblImageFile)))
		{
			finish_code = SBL_LOCAL_READ_FAILED;
		}
		else if (bytes_read == 0)
		{
			if (zbSocSblState == ZBSOC_SBL_STATE_PROGRAMMING)
			{
				zbSocSblState = ZBSOC_SBL_STATE_VERIFYING;
				zbSocCurrentImageBlockAddress = 0x00000000;
				fseek(zbSocSblImageFile, 0, SEEK_SET);
				load_next_block_from_file = TRUE;
			}
			else
			{
				zbSocSblState = ZBSOC_SBL_STATE_EXECUTING;
			}
		}
		else
		{
			memset(buf + bytes_read, 0xFF, ZBSOC_SBL_IMAGE_BLOCK_SIZE - bytes_read); //pad the last block with 0xFF
			buf[0] = (zbSocCurrentImageBlockAddress / 4) & 0xFF; //the addresses reported in the packet are word addresses, not byte addresses, hence divided by 4
			buf[1] = ((zbSocCurrentImageBlockAddress / 4) >> 8) & 0xFF;
		}
	}

	timeout_retries = MAX_TIMEOUT_RETRIES;

	switch (zbSocSblState)
	{
		case ZBSOC_SBL_STATE_HANDSHAKING:
			printf("Handshaking\n");
			zbSocResetLocalDevice(); //will only be accepted if the main application is currently active (i.e. bootloader not listening)
		
			zbSocSblEnableBootloader();
			zbSocSblHandshake();//will only be accepted if the bootloader is listening
			break;

		case ZBSOC_SBL_STATE_PROGRAMMING:
			printf("writing to address 0x%08X\n", zbSocCurrentImageBlockAddress);
			zbSocSblSendImageBlock(buf, ZBSOC_SBL_IMAGE_BLOCK_SIZE + 2);
			break;

		case ZBSOC_SBL_STATE_VERIFYING:
			printf("reading from address 0x%08X\n", zbSocCurrentImageBlockAddress);
			zbSocSblReadImageBlock(zbSocCurrentImageBlockAddress);
			break;

		case ZBSOC_SBL_STATE_EXECUTING:
			printf("Executing image\n");
			zbSocSblExecuteImage();
			break;
		
		default:
			//unexpected error
			break;
	}

	/*---Periodic Reporting Handling -----------------------*/

	if (zbSocSblReportingPending)
	{
		zbSocSblReportingPending = FALSE;
		zbSocCb.pfnBootloadingProgressReportingCb(zbSocSblState, zbSocCurrentImageBlockAddress);
	}
}


/*************************************************************************************************
 * @fn      processCertInstall()
 *
 * @brief   
 *
 * @param
 *
 * @return 
 *************************************************************************************************/
static void processCertInstall(uint8_t operation)
{
//  printf("processCertInstall: operation = %d\n", operation);

  if (operation == ZBSOC_CERT_OPER_START)
  {
    zbSocCertState = ZBSOC_CERT_STATE_IMPLICIT_CERT;
  }

//  printf("processCertInstall: zbSocCertState = %d\n", zbSocCertState);

  if (zbSocCertState < ZBSOC_CERT_STATE_IDLE)
  {
    if (operation == ZBSOC_CERT_OPER_STOP || operation == ZBSOC_CERT_OPER_ERROR)
    {
      zbSocCertState = ZBSOC_CERT_STATE_IDLE;
      
      if (zbSocCb.pfnCertInstallResultIndCb)
      {
        zbSocCb.pfnCertInstallResultIndCb((operation == ZBSOC_CERT_OPER_STOP) ? CERT_RESULT_TERMINATED : CERT_RESULT_WRITING_FAILED);
      }
    }
    else
    {
      uint8_t erase = 0xFF;
      
      if (operation == ZBSOC_CERT_OPER_CONT)
      {
        zbSocCertState++;
      }
      
      switch (zbSocCertState)
      {
      case ZBSOC_CERT_STATE_IMPLICIT_CERT:
        zbSocNVWrite(0x69, 0, ZCL_KE_IMPLICIT_CERTIFICATE_LEN, zbSocCertInfo.implicitCert);
        break;
      case ZBSOC_CERT_STATE_DEV_PRIVATE_KEY:
        zbSocNVWrite(0x6a, 0, ZCL_KE_DEVICE_PRIVATE_KEY_LEN, zbSocCertInfo.devPrivateKey);
        break;
      case ZBSOC_CERT_STATE_CA_PUBLIC_KEY:
        zbSocNVWrite(0x6b, 0, ZCL_KE_CA_PUBLIC_KEY_LEN, zbSocCertInfo.caPublicKey);
        break;
      case ZBSOC_CERT_STATE_IEEE_ADDRESS:
        zbSocNVWrite(0x01, 0, Z_EXTADDR_LEN, zbSocCertInfo.ieeeAddr);
        break;
      case ZBSOC_CERT_STATE_ERASE_NWK_INFO:
        zbSocNVWrite(0x03, 0, 1, &erase);
        break;
      case ZBSOC_CERT_STATE_COMPLETE:
        if (zbSocCb.pfnCertInstallResultIndCb)
        {
          zbSocCb.pfnCertInstallResultIndCb(CERT_RESULT_COMPLETED);
        }
        zbSocCertState = ZBSOC_CERT_STATE_IDLE;
        if (zbSocCertForce2Reset)
        {
          zbSocResetLocalDevice();
        }
        break;
      }
    }
  }
}

/*************************************************************************************************
 * @fn      processRpcSysDbg()
 *
 * @brief   read and process the RPC debug message from the ZLL controller
 *
 * @param   none
 *
 * @return  length of current Rx Buffer
 *************************************************************************************************/
static void processRpcSysDbg(uint8_t *rpcBuff, uint8_t len)
{
  if( rpcBuff[1] == MT_DEBUG_MSG )
  {
    //we got a debug string
	
//    system("tput setaf 1");
    printf("  ===> %.*s", len, (char*) &(rpcBuff[2]));
//    system("tput setaf 7");
  }              
  else if( rpcBuff[1] == 0 )
  {
    if( rpcBuff[2] == 0)
    {
//      printf("processRpcSysDbg: Command Received Successfully\n\n");
    }
    else
    {
      printf("processRpcSysDbg: Command Error\n\n");
    }    
  }
  else
  {
    printf("processRpcSysDbg: Unsupported MT App Msg\n");
  }
}


/*************************************************************************************************
 * @fn      zbSocProcessRpc()
 *
 * @brief   read and process the RPC from the ZLL controller
 *
 * @param   none
 *
 * @return  length of current Rx Buffer
 *************************************************************************************************/
void zbSocProcessRpc (void)
{
  uint8_t rpcLen, bytesRead, sofByte, *rpcBuff, rpcBuffIdx;    
  static uint8_t retryAttempts = 0;
  int x;
  uint8_t len;

  //read first byte and check it is a SOF
  read(serialPortFd, &sofByte, 1);
  if ( sofByte == MT_RPC_SOF )
  { 
    retryAttempts = 0;     
        
    //read len
    bytesRead = read(serialPortFd, &rpcLen, 1);

    if( bytesRead == 1)
    {    
      len = rpcLen;
	  
      //allocating RPC payload (+ cmd0, cmd1 and fcs)
      rpcLen += 3;

      rpcBuff = malloc(rpcLen);

      //non blocking read, so we need to wait for the rpc to be read
      rpcBuffIdx = 0;
      while(rpcLen > 0)
      {
        //read rpc
        bytesRead = read(serialPortFd, &(rpcBuff[rpcBuffIdx]), rpcLen);  

        //check for error
        if( bytesRead > rpcLen)
        {
          //there was an error
          printf("zbSocProcessRpc: read of %d bytes failed - %s\n", rpcLen, strerror(errno) );

          if( retryAttempts++ < 50 )
          {
            //sleep for 10ms
	          usleep(100000);
            //try again
            bytesRead = 0;
          }
          else
          {
            //something went wrong.
            printf("zbSocProcessRpc: failed\n");
            free(rpcBuff);
            return;
          }
        }

        rpcLen -= bytesRead;	
        rpcBuffIdx += bytesRead;
      }

	  if (uartDebugPrintsEnabled)
	  {
		  printf("UART IN  <-- %d Bytes: SOF:%02X, Len:%02X, CMD0:%02X, CMD1:%02X, Payload:", len+5, MT_RPC_SOF, len, rpcBuff[0], rpcBuff[1]); \
		  for (x = 0; x < len; x++)
		  {
	        printf("%02X%s", rpcBuff[x + 2], x < len - 1 ? ":" : ",");
		  }
		  printf(" FCS:%02X\n", rpcBuff[x + 2]);
	  }

//todo: verify FCS of incoming MT frames

      //Read CMD0
      switch (rpcBuff[0] & MT_RPC_SUBSYSTEM_MASK) 
      {
        case MT_RPC_SYS_SYS:
          processRpcSysSys(rpcBuff);
          break;
        case MT_RPC_SYS_DBG:
          processRpcSysDbg(rpcBuff, len);        
          break;       
        case MT_RPC_SYS_APP:
          processRpcSysApp(rpcBuff);        
          break;       
		case MT_RPC_SYS_SBL:
          processRpcSysSbl(rpcBuff);		  
          break;
        default:
          printf("zbSocProcessRpc: CMD0:%x, CMD1:%x, not handled\n", rpcBuff[0], rpcBuff[1] );
          break;
      }
      
      free(rpcBuff);
    }
    else
    {
      printf("zbSocProcessRpc: No valid Start Of Frame found\n");
    }
  }
  
  return; 
}


/*************************************************************************************************
 * @fn      zbSocTimeoutCallback()
 *
 * @brief   
 *
 * @param
 *
 * @return 
 *************************************************************************************************/
void zbSocTimeoutCallback(void)
{
	
	zbSocClose();
	usleep(100000); //100 ms
	zbSocOpen( NULL ); 	 
	if( serialPortFd == -1 )
	{
	  exit(-1);
	}

//	printf("-------------------------- TIMEOUT OCCURED!!! --------------------------\n");
	processRpcSysSbl(NULL);
}


/*************************************************************************************************
 * @fn      zbSocTimeoutCallback()
 *
 * @brief   
 *
 * @param
 *
 * @return 
 *************************************************************************************************/
void zbSocSblReportingCallback(void)
{
	if (zbSocSblProgressReportingInterval > 0)
	{
	zbSocSblReportingPending = TRUE;
	zbSocEnableTimeout(REPORTING_TIMER, zbSocSblProgressReportingInterval * 100);
}
	else
	{
		zbSocDisableTimeout(REPORTING_TIMER);
	}
}
