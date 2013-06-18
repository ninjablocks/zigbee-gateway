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

#include <errno.h>
#include <string.h>
#include <signal.h>
#include <time.h>

#include "zbSocCmd.h"

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

#define socWrite(fd,rpcBuff,rpcLen) \
	do { \
		if (uartDebugPrintsEnabled) \
		{ \
			int x; \
			printf("UART OUT --> %d Bytes: SOF:%02X, Len:%02X, CMD0:%02X, CMD1:%02X, Payload:", (rpcLen), (rpcBuff)[0], (rpcBuff)[1] , (rpcBuff)[2], (rpcBuff)[3] ); \
			for (x = 4; x < (rpcLen) - 1; x++) \
			{ \
			  printf("%02X%s", (rpcBuff)[x], x < (rpcLen) - 1 - 1 ? ":" : ","); \
			} \
			printf(" FCS:%02X\n", (rpcBuff)[x]); \
		} \
		write((fd),(rpcBuff),(rpcLen)); \
	} while (0)


#define NUM_OF_TIMERS (sizeof(timers) / sizeof(timers[0]))
			
#define TIMEOUT_TIMER (&timers[0])
#define REPORTING_TIMER (&timers[1])
			      
/*********************************************************************
 * CONSTANTS
 */
#define MT_APP_RPC_CMD_TOUCHLINK          0x01
#define MT_APP_RPC_CMD_RESET_TO_FN        0x02
#define MT_APP_RPC_CMD_CH_CHANNEL         0x03
#define MT_APP_RPC_CMD_JOIN_HA            0x04
#define MT_APP_RPC_CMD_PERMIT_JOIN        0x05
#define MT_APP_RPC_CMD_SEND_RESET_TO_FN   0x06

#define MT_APP_RSP                           0x80
#define MT_APP_ZLL_TL_IND                    0x81
#define MT_APP_NEW_DEV_IND               0x82

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
// Lighting Clusters
#define ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL          0x0300
// Mettering and Sensing Clusters
#define ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT			 0x0402
#define ZCL_CLUSTER_ID_MS_REL_HUMIDITY_MEASUREMENT		 0x0405
//Metering
#define ZCL_CLUSTER_ID_SE_SIMPLE_METERING              0x0702
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

#define MT_NLME_LEAVE_REQ         0x05

#define Z_EXTADDR_LEN    8

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

	
int timeout_retries;
uint16_t zbSocSblState = ZBSOC_SBL_STATE_IDLE;

zllTimer timers[2] = {
	{NULL, 0, FALSE},
	{NULL, 0, FALSE},
};

/*********************************************************************
 * EXTERNAL VARIABLES
 */
extern uint8_t uartDebugPrintsEnabled;


/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void calcFcs(uint8_t *msg, int size);
void processRpcSysAppTlInd(uint8_t *TlIndBuff);
static void processRpcSysAppNewDevInd(uint8_t *TlIndBuff);
static void processRpcSysAppZcl(uint8_t *zclRspBuff);
static void processRpcSysAppZclFoundation(uint8_t *zclRspBuff, uint8_t zclFrameLen, uint16_t clusterID, uint16_t nwkAddr, uint8_t endpoint);
static void processRpcSysAppZclCluster(uint8_t *zclRspBuff, uint8_t zclFrameLen, uint16_t clusterID, uint16_t nwkAddr, uint8_t endpoint);
static void processRpcSysApp(uint8_t *rpcBuff);
static void processRpcSysDbg(uint8_t *rpcBuff);
void zbSocSblEnableBootloader();
void zbSocTimerCreate(zllTimer * timer, timerCallback_t callback);
void zbSocTimerDelete(zllTimer * timer);
void zbSocTimeoutCallback(void);
void zbSocSblReportingCallback(void);
static void processRpcSysSbl(uint8_t *rpcBuff);
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
  struct termios tio;
  static char lastUsedDevicePath[255];
  char * devicePath;
  
  if (_devicePath != NULL)
  {
	if (strlen(_devicePath) > (sizeof(lastUsedDevicePath) - 1))
	{
		printf("%s - device path too long\n",_devicePath);
		return(-1);
	}
  	devicePath = _devicePath;
	strcpy(lastUsedDevicePath, _devicePath);
  }
  else
  {
  	devicePath = lastUsedDevicePath;
  }
  

  /* open the device to be non-blocking (read will return immediatly) */
  serialPortFd = open(devicePath, O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (serialPortFd <0) 
  {
    perror(devicePath); 
    printf("%s open failed\n",devicePath);
    return(-1);
  }
  
  //make the access exclusive so other instances will return -1 and exit
  ioctl(serialPortFd, TIOCEXCL);

  /* c-iflags
     B115200 : set board rate to 115200
     CRTSCTS : HW flow control (disabled below)
     CS8     : 8n1 (8bit,no parity,1 stopbit)
     CLOCAL  : local connection, no modem contol
     CREAD   : enable receiving characters*/
  tio.c_cflag = B38400 | CRTSCTS | CS8 | CLOCAL | CREAD;
  /* c-iflags
     ICRNL   : maps 0xD (CR) to 0x10 (LR), we do not want this.
     IGNPAR  : ignore bits with parity erros, I guess it is 
     better to ignStateore an erronious bit then interprit it incorrectly. */
  tio.c_iflag = IGNPAR  & ~ICRNL; 
  tio.c_oflag = 0;
  tio.c_lflag = 0;

  tcflush(serialPortFd, TCIFLUSH);
  tcsetattr(serialPortFd,TCSANOW,&tio);
  
  return serialPortFd;
}


void zbSocForceRun(void)
{
	uint8_t forceBoot = SB_FORCE_RUN;
	
	//Send the bootloader force boot incase we have a bootloader that waits
	socWrite(serialPortFd,&forceBoot, 1);
	tcflush(serialPortFd, TCOFLUSH);  
	
}
	
void zbSocClose( void )
{
  tcflush(serialPortFd, TCOFLUSH);
  close(serialPortFd);

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
    socWrite(serialPortFd,tlCmd, sizeof(tlCmd));
    tcflush(serialPortFd, TCOFLUSH);   
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
    socWrite(serialPortFd,tlCmd, sizeof(tlCmd));
    tcflush(serialPortFd, TCOFLUSH);   
}

/*********************************************************************
 * @fn      zbSocSendResetToFn
 *
 * @brief   Send the reset to factory new command to a ZLL device.
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
    socWrite(serialPortFd,tlCmd, sizeof(tlCmd));
    tcflush(serialPortFd, TCOFLUSH);   
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
    write(serialPortFd,cmd, sizeof(cmd));
    tcflush(serialPortFd, TCOFLUSH);   
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
  	
    socWrite(serialPortFd,cmd,sizeof(cmd));
    tcflush(serialPortFd, TCOFLUSH);
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
		zbSocTimerCreate(REPORTING_TIMER, zbSocSblReportingCallback);
		zbSocEnableTimeout(REPORTING_TIMER, zbSocSblProgressReportingInterval * 100);
	}

	zbSocTimerCreate(TIMEOUT_TIMER, zbSocTimeoutCallback);
	
	zbSocSblState = ZBSOC_SBL_STATE_HANDSHAKING;
	timeout_retries = MAX_TIMEOUT_RETRIES;
	processRpcSysSbl(NULL);

	return SUCCESS;
}

/*********************************************************************
 * @fn      zbSocTimerCreate
 *
 * @brief   
 *
 * @param   
 *
 * @return  
 */
void zbSocTimerCreate(zllTimer * timer, timerCallback_t callback)
{
	int rc;
	struct sigevent sevp;

	sevp.sigev_notify  = SIGEV_NONE;
	
	timer->enabled = FALSE;
	timer->callback = callback;
	rc = timer_create(CLOCK_REALTIME, &sevp, &(timer->id));
//	todo: assert(rc==0);
}

/*********************************************************************
 * @fn      zbSocTimerDelete
 *
 * @brief   
 *
 * @param   
 *
 * @return  
 */
void zbSocTimerDelete(zllTimer * timer)
{
	int rc;
	
	zbSocDisableTimeout(timer);
	rc = timer_delete(timer->id);
//	todo: assert(rc==0);
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
void zbSocDisableTimeout(zllTimer * timer)
{
	struct itimerspec its;

	its.it_interval.tv_sec = 0;
	its.it_interval.tv_nsec = 0;
	its.it_value.tv_sec = 0;
	its.it_value.tv_nsec = 0;
	
	if (timer_settime(timer->id, 0, &its, NULL) != 0)
	{
		exit (0); //assert
	}
	
	timer->enabled = FALSE;
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
void zbSocEnableTimeout(zllTimer * timer, uint32_t milliseconds)
{
	struct itimerspec its;

	its.it_interval.tv_sec = 0;
	its.it_interval.tv_nsec = 0;
	its.it_value.tv_sec = (milliseconds*1000000) / 1000000000;;
	its.it_value.tv_nsec = (milliseconds*1000000) % 1000000000;
	
	if (timer_settime(timer->id, 0, &its, NULL) != 0)
	{
		exit (0); //assert
	}

	timer->enabled = TRUE;
}

/*********************************************************************
 * @fn      zbSocIsTimeoutEnabled
 *
 * @brief   
 *
 * @param   
 *
 * @return  
 */
uint8_t zbSocIsTimeoutEnabled(zllTimer * timer)
{
	return timer->enabled;
}

/*********************************************************************
 * @fn      zbSocHandleTimers
 *
 * @brief   
 *
 * @param   
 *
 * @return  
 */
uint8_t zbSocHandleTimers(void)
{
	int x;
	uint8_t timers_enabled = FALSE;

	for (x = 0; x < NUM_OF_TIMERS; x++)
	{
		if (zbSocIsTimeoutEnabled(&(timers[x])))
		{
			if (zbSocIsTimerExpired(&(timers[x])))
			{
				timers[x].callback();
			}
			else
			{
				timers_enabled = TRUE;
			}
		}
	}
	
	return timers_enabled;
}

/*********************************************************************
 * @fn      zbSocIsTimerExpired
 *
 * @brief   
 *
 * @param   
 *
 * @return  
 */
uint8_t zbSocIsTimerExpired(zllTimer * timer)
{
	struct itimerspec its;

	if (timer->enabled)
	{
		if (timer_gettime(timer->id, &its) != 0)
		{
			exit(0); //assert
		}
		
		if ((its.it_value.tv_sec == 0) && (its.it_value.tv_nsec == 0))
		{
			timer->enabled = FALSE;
		}
	}

	return (!timer->enabled);
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

	zbSocTimerDelete(TIMEOUT_TIMER);
	
	if (zbSocSblProgressReportingInterval > 0)
	{
		zbSocTimerDelete(REPORTING_TIMER);
		zbSocSblReportingPending = FALSE;
	}
}


/*********************************************************************
 * @fn      zbSocSetLevel
 *
 * @brief   Send the level command to a ZLL light.
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
    
    socWrite(serialPortFd,cmd,sizeof(cmd));
    tcflush(serialPortFd, TCOFLUSH);
}

/*********************************************************************
 * @fn      zbSocSetHue
 *
 * @brief   Send the hue command to a ZLL light.
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
  socWrite(serialPortFd,cmd,sizeof(cmd));
  tcflush(serialPortFd, TCOFLUSH);
}

/*********************************************************************
 * @fn      zbSocSetSat
 *
 * @brief   Send the satuartion command to a ZLL light.
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
  socWrite(serialPortFd,cmd,sizeof(cmd));
  tcflush(serialPortFd, TCOFLUSH);
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
	socWrite(serialPortFd, buf, payload_len + 5);
	tcflush(serialPortFd, TCOFLUSH);
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
  socWrite(serialPortFd,cmd,sizeof(cmd));
  tcflush(serialPortFd, TCOFLUSH);
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
	
	socWrite(serialPortFd,cmd,sizeof(cmd));
	tcflush(serialPortFd, TCOFLUSH);
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
	socWrite(serialPortFd,cmd,sizeof(cmd));
	tcflush(serialPortFd, TCOFLUSH);		
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
  socWrite(serialPortFd,cmd,sizeof(cmd));
  tcflush(serialPortFd, TCOFLUSH);        
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
	
  socWrite(serialPortFd,cmd,sizeof(cmd));
  tcflush(serialPortFd, TCOFLUSH);
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
	
  socWrite(serialPortFd,cmd,sizeof(cmd));
  tcflush(serialPortFd, TCOFLUSH);
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
	
  socWrite(serialPortFd,cmd,sizeof(cmd));
  tcflush(serialPortFd, TCOFLUSH);
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
      
	calcFcs(cmd, sizeof(cmd));
	
	/*printf("zbSocBind: srcNwkAddr=0x%x, srcEndpoint=0x%x, srcIEEE=0x%x:%x:%x:%x:%x:%x:%x:%x, dstEndpoint=0x%x, dstIEEE=0x%x:%x:%x:%x:%x:%x:%x:%x, clusterID:%x\n", 
	          srcNwkAddr, srcEndpoint, srcIEEE[0], srcIEEE[1], srcIEEE[2], srcIEEE[3], srcIEEE[4], srcIEEE[5], srcIEEE[6], srcIEEE[7], 
	          srcEndpoint, dstIEEE[0], dstIEEE[1], dstIEEE[2], dstIEEE[3], dstIEEE[4], dstIEEE[5], dstIEEE[6], dstIEEE[7], clusterID);*/
	
  write(serialPortFd,cmd,sizeof(cmd));
  tcflush(serialPortFd, TCOFLUSH);
}

/*********************************************************************
 * @fn      zbSocGetState
 *
 * @brief   Send the get state command to a ZLL light.
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
  	
    socWrite(serialPortFd,cmd,sizeof(cmd));
    tcflush(serialPortFd, TCOFLUSH);
} 
 
/*********************************************************************
 * @fn      zbSocGetLevel
 *
 * @brief   Send the get level command to a ZLL light.
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
  	
    socWrite(serialPortFd,cmd,sizeof(cmd));
    tcflush(serialPortFd, TCOFLUSH);
} 

/*********************************************************************
 * @fn      zbSocGetHue
 *
 * @brief   Send the get hue command to a ZLL light.
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
  	
    socWrite(serialPortFd,cmd,sizeof(cmd));
    tcflush(serialPortFd, TCOFLUSH);
} 

/*********************************************************************
 * @fn      zbSocGetSat
 *
 * @brief   Send the get saturation command to a ZLL light.
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
  	
    socWrite(serialPortFd,cmd,sizeof(cmd));
    tcflush(serialPortFd, TCOFLUSH);
} 
/*********************************************************************
 * @fn      zbSocGetPower
 *
 * @brief   Send the get saturation command to a ZLL light.
 *
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be sent the command.
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast.
 *
 * @return  none
 */
void zbSocGetPower(uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode)
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
  		(ATTRID_MASK_SE_HISTORICAL_CONSUMPTION & 0x00ff),
  		(ATTRID_MASK_SE_HISTORICAL_CONSUMPTION & 0xff00) >> 8,
  		0x00       //FCS - fill in later
  	};
      
  	calcFcs(cmd, sizeof(cmd));
  	
    socWrite(serialPortFd,cmd,sizeof(cmd));
    tcflush(serialPortFd, TCOFLUSH);
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
  	
    socWrite(serialPortFd,cmd,sizeof(cmd));
    tcflush(serialPortFd, TCOFLUSH);
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
  	
    socWrite(serialPortFd,cmd,sizeof(cmd));
    tcflush(serialPortFd, TCOFLUSH);
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
void processRpcSysAppTlInd(uint8_t *TlIndBuff)
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
    
  epInfo.status = 0;
  
  epInfo.nwkAddr = BUILD_UINT16(TlIndBuff[0], TlIndBuff[1]);
  TlIndBuff+=2;      
  epInfo.endpoint = *TlIndBuff++;
  epInfo.profileID = BUILD_UINT16(TlIndBuff[0], TlIndBuff[1]);
  TlIndBuff+=2;      
  epInfo.deviceID = BUILD_UINT16(TlIndBuff[0], TlIndBuff[1]);
  TlIndBuff+=2;   
  epInfo.version = *TlIndBuff++;
  epInfo.deviceName = NULL;
  
  for(i=0; i<8; i++)
  {
    epInfo.IEEEAddr[i] = *TlIndBuff++;
  }
  
//  printf("processRpcSysAppNewDevInd: %x:%x\n",  epInfo.nwkAddr, epInfo.endpoint);
  if(zbSocCb.pfnNewDevIndicationCb)
  {
    zbSocCb.pfnNewDevIndicationCb(&epInfo);
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
    else if( (clusterID == ZCL_CLUSTER_ID_SE_SIMPLE_METERING) && (attrID == ATTRID_MASK_SE_HISTORICAL_CONSUMPTION) && (dataType == ZCL_DATATYPE_INT24) )
    {
      if(zbSocCb.pfnZclGetPowerCb)      
      {
        uint32_t power;  
        power = BUILD_UINT32(zclRspBuff[0], zclRspBuff[1], zclRspBuff[2], 0);            
        printf("processRpcSysAppZclFoundation: Power:%x, %x:%x:%x:%x\n", power, zclRspBuff[0], zclRspBuff[1], zclRspBuff[3], 0);
        zbSocCb.pfnZclGetPowerCb(power, nwkAddr, endpoint);
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
  
  printf("processRpcSysAppZclCluster: commandID=%x, len=%x\n", commandID, len); 
  
  if( clusterID == ZCL_CLUSTER_ID_SS_IAS_ZONE)
  {
    if(commandID == COMMAND_SS_IAS_ZONE_STATUS_CHANGE_NOTIFICATION)
    {
      if(zbSocCb.pfnZclZoneSateChangeCb)      
      {
        uint32_t zoneState;  
        zoneState = BUILD_UINT32(zclRspBuff[0], zclRspBuff[1], zclRspBuff[3], 0);            
        zbSocCb.pfnZclZoneSateChangeCb(zoneState, nwkAddr, endpoint);
      }
    }
    if(commandID == COMMAND_SS_IAS_ZONE_STATUS_ENROLL_REQUEST)
    {    
      
    }
  }
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
 * @fn      processRpcSysDbg()
 *
 * @brief   read and process the RPC debug message from the ZLL controller
 *
 * @param   none
 *
 * @return  length of current Rx Buffer
 *************************************************************************************************/
static void processRpcSysDbg(uint8_t *rpcBuff)
{
  if( rpcBuff[1] == MT_DEBUG_MSG )
  {
    //we got a debug string
	
    system("tput setaf 1");
    printf("%s", (char*) &(rpcBuff[2]));
    system("tput setaf 7");
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

          if( retryAttempts++ < 5 )
          {
            //sleep for 10ms
	          usleep(10000);
            //try again
            bytesRead = 0;
          }
          else
          {
            //something went wrong.
            printf("zbSocProcessRpc: failed\n");
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
        case MT_RPC_SYS_DBG:
          processRpcSysDbg(rpcBuff);        
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
	zbSocSblReportingPending = TRUE;
	zbSocEnableTimeout(REPORTING_TIMER, zbSocSblProgressReportingInterval * 100);
}
