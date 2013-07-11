/*
 * zbSocCmd.h
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

#ifndef ZBSOCCMD_H
#define ZBSOCCMD_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>


/********************************************************************/
// ZigBee Soc Definitions

#define SBL_SUCCESS 0
#define SBL_INTERNAL_ERROR 1
#define SBL_BUSY 2
#define SBL_OUT_OF_MEMORY 3
#define SBL_PENDING 4
#define SBL_ABORTED_BY_USER 5
#define SBL_NO_ACTIVE_DOWNLOAD 6
#define SBL_ERROR_OPENING_FILE 7
#define SBL_TARGET_WRITE_FAILED 8
#define SBL_EXECUTION_FAILED 9
#define SBL_HANDSHAKE_FAILED 10
#define SBL_LOCAL_READ_FAILED 11
#define SBL_VERIFICATION_FAILED 12
#define SBL_COMMUNICATION_FAILED 13
#define SBL_ABORTED_BY_ANOTHER_USER 14
#define SBL_REMOTE_ABORTED_BY_USER 15
#define SBL_TARGET_READ_FAILED 16
#define SBL_TARGET_STILL_WORKING 0xFF

#define KE_STATE_INITIATED    0
#define KE_STATE_CONFIRMED    1
#define KE_STATE_TERMINATED   2

#define CERT_RESULT_COMPLETED            0 // Completed with success
#define CERT_RESULT_ERROR_OPENING_FILE   1 // Error in opening the file
#define CERT_RESULT_OUT_OF_MEMORY        2 // Out of memory
#define CERT_RESULT_WRONG_FORMAT         3 // Format is wrong
#define CERT_RESULT_BUSY                 4 // Busy
#define CERT_RESULT_WRITING_FAILED       5 // Non-success response from the SoC
#define CERT_RESULT_TERMINATED           6 // Terminated by user or error

#define SUCCESS 0
#define FAILURE 1

#define BOOTLOADER_TIMEOUT 100 //milliseconds

/********************************************************************/
// ZigBee Soc Types

// Endpoint information record entry
typedef struct
{
  uint8_t IEEEAddr[8];
  uint16_t nwkAddr;   // Network address
  uint8_t endpoint;   // Endpoint identifier
  uint16_t profileID; // Profile identifier
  uint16_t deviceID;  // Device identifier
  uint8_t version;    // Version
  char* deviceName;
  uint8_t status;
  uint8_t flags;
} epInfo_t;

typedef struct
{
	epInfo_t * epInfo;
	uint16_t prevNwkAddr;	// Precious network address
	uint8_t type;	// new / updated / old
} epInfoExtended_t;

#define EP_INFO_TYPE_EXISTING 0
#define EP_INFO_TYPE_NEW 1
#define EP_INFO_TYPE_UPDATED 2
#define EP_INFO_TYPE_REMOVED 4



// String Data Type
typedef struct
{
  uint8_t strLen;
  uint8_t *pStr;
} UTF8String_t;

// Publish Price command payload
typedef struct
{
  uint32_t  providerId;
  UTF8String_t rateLabel;
  uint32_t  issuerEventId;
  uint32_t  currentTime;
  uint8_t   unitOfMeasure;
  uint16_t  currency;
  uint8_t   priceTrailingDigit;
  uint8_t   numberOfPriceTiers;
  uint32_t  startTime;
  uint16_t  durationInMinutes;
  uint32_t  price;
  uint8_t   priceRatio;
  uint32_t  generationPrice;
  uint8_t   generationPriceRatio;
  uint32_t  alternateCostDelivered;      // Alternative measure of the cost of the energy consumed
  uint8_t   alternateCostUnit;           // 8-bit enum identifying the unit for Alternate Cost Delivered field
  uint8_t   alternateCostTrailingDigit;  // Location of decimal point in alternatecost field
  uint8_t   numberOfBlockThresholds;     // Number of block thresholds available
  uint8_t   priceControl;                // Additional control options for the price event
} zclCCPublishPrice_t;

// Message Control
typedef struct
{
  uint8_t transmissionMode;     // valid value  0~2
  uint8_t importance;           // 0~3
  uint8_t pinRequired;          // 0~1 - applicable only to SE UK Extension
  uint8_t acceptanceRequired;   // 0~1 - applicable only to SE UK Extension
  uint8_t confirmationRequired; // 0~1
} zclMessageCtrl_t;

// Display Message command payload
typedef struct
{
  uint32_t  messageId;
  zclMessageCtrl_t messageCtrl;
  uint32_t  startTime;
  uint16_t  durationInMinutes;
  UTF8String_t msgString;
} zclCCDisplayMessage_t;

typedef uint8_t (*zbSocTlIndicationCb_t)(epInfo_t *epInfo);
typedef uint8_t (*zbSocNewDevIndicationCb_t)(epInfo_t *epInfo);
typedef uint8_t (*zbSocZclGetStateCb_t)(uint8_t state, uint16_t nwkAddr, uint8_t endpoint);
typedef uint8_t (*zbSocZclGetLevelCb_t)(uint8_t level, uint16_t nwkAddr, uint8_t endpoint);
typedef uint8_t (*zbSocZclGetHueCb_t)(uint8_t hue, uint16_t nwkAddr, uint8_t endpoint);
typedef uint8_t (*zbSocZclGetSatCb_t)(uint8_t sat, uint16_t nwkAddr, uint8_t endpoint);
typedef uint8_t (*zbSocZclGetTempCb_t)(uint16_t sat, uint16_t nwkAddr, uint8_t endpoint);
typedef uint8_t (*zbSocZclReadPowerRspCb_t) (uint32_t power, uint16_t nwkAddr, uint8_t endpoint);
typedef uint8_t (*zbSocZclGetHumidCb_t)(uint16_t humnidity, uint16_t nwkAddr, uint8_t endpoint);
typedef uint8_t (*zbSocZoneStateChangeCb_t)(uint32_t zoneState, uint16_t nwkAddr, uint8_t endpoint);
typedef uint8_t (*zbSocBootloadingDoneCb_t)(uint8_t State);
typedef uint8_t (*zbSocBootloadingProgressReportingCb_t)(uint8_t Phase, uint32_t location);
typedef uint8_t (*zbSocCertInstallResultIndCb_t)(uint8_t result);
typedef uint8_t (*zbSocKeyEstablishmentStateIndCb_t)(uint8_t state);
typedef uint8_t (*zbSocZclDisplayMessageIndCb_t)(uint8_t *zclPayload, uint8_t len);
typedef uint8_t (*zbSocZclPublishPriceIndCb_t)(uint8_t *zclPayload, uint8_t len);

typedef struct
{
  zbSocTlIndicationCb_t          pfnTlIndicationCb;      // TouchLink Indication callback
  zbSocNewDevIndicationCb_t         pfnNewDevIndicationCb;  // New device Indication callback    
  zbSocZclGetStateCb_t           pfnZclGetStateCb;       // ZCL response callback for get State
  zbSocZclGetLevelCb_t           pfnZclGetLevelCb;     // ZCL response callback for get Level
  zbSocZclGetHueCb_t             pfnZclGetHueCb;         // ZCL response callback for get Hue
  zbSocZclGetSatCb_t             pfnZclGetSatCb;         // ZCL response callback for get Sat
  zbSocZclGetTempCb_t            pfnZclGetTempCb;         // ZCL response callback for get Temp
  zbSocZclReadPowerRspCb_t       pfnZclReadPowerRspCb;    // ZCL response callback for read Power
  zbSocZclGetHumidCb_t           pfnZclGetHumidCb;         // ZCL response callback for get Temp    
  zbSocZoneStateChangeCb_t       pfnZclZoneStateChangeCb;    //ZCL Command indicating Alarm Zone State Change
  zbSocBootloadingDoneCb_t       pfnBootloadingDoneCb;    //Bootloader processing ended
  zbSocBootloadingProgressReportingCb_t pfnBootloadingProgressReportingCb; //bootloader progress reporting
  zbSocCertInstallResultIndCb_t   pfnCertInstallResultIndCb;  // Certificate installation result indication callback
  zbSocKeyEstablishmentStateIndCb_t   pfnKeyEstablishmentStateIndCb;  // Key Establishment state change reporting
  zbSocZclDisplayMessageIndCb_t  pfnZclDisplayMessageIndCb;  // ZCL response callback for GetLastMessage or ZCL unsolicited message callback for DisplayMessage
  zbSocZclPublishPriceIndCb_t    pfnZclPublishPriceIndCb;    // ZCL response callback for GetCurrentPrice or ZCL unsolicited message callback for PublishPrice
} zbSocCallbacks_t;

typedef void (*timerCallback_t)(void);

typedef struct
{
	timerCallback_t callback;
	timer_t id;
	uint8_t enabled;
} zllTimer;

extern zllTimer TIMEOUT_TIMER;
extern int serialPortFd;
extern const char * BOOTLOADER_RESULT_STRINGS[];

typedef struct 
{
	int fd;
	timerCallback_t callback;
}timerFDs_t;

#define NUM_OF_TIMERS 2
void zbSocGetTimerFds(timerFDs_t *fds);

/********************************************************************/
// ZigBee Soc API

//configuration API's
int32_t zbSocOpen( char *devicePath );
void zbSocRegisterCallbacks( zbSocCallbacks_t zbSocCallbacks);
void zbSocClose( void );
void zbSocProcessRpc (void);

//ZigBee Control API's
void zbSocTouchLink(void);
void zbSocResetToFn(void);
void zbSocSendResetToFn(void);
void zbSocOpenNwk(void);
uint8_t zbSocInitiateCertInstall(char *filename, uint8_t force2reset);
//ZCL Set API's
void zbSocSetState(uint8_t state, uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode);
void zbSocSetLevel(uint8_t level, uint16_t time, uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode);
void zbSocSetHue(uint8_t hue, uint16_t time, uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode);
void zbSocSetSat(uint8_t sat, uint16_t time, uint16_t dstAddr, uint8_t  endpoint, uint8_t addrMode);
void zbSocSetHueSat(uint8_t hue, uint8_t sat, uint16_t time, uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode);
void zbSocAddGroup(uint16_t groupId, uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode);
void zbSocStoreScene(uint16_t groupId, uint8_t sceneId, uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode);
void zbSocRecallScene(uint16_t groupId, uint8_t sceneId, uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode);
void zbSocBind(uint16_t srcNwkAddr, uint8_t srcEndpoint, uint8_t srcIEEE[8], uint8_t dstEndpoint, uint8_t dstIEEE[8], uint16_t clusterID);
//ZCL Get API's
void zbSocGetState(uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode);
void zbSocGetLevel(uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode);
void zbSocGetHue(uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode);
void zbSocGetSat(uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode);
void zbSocGetTemp(uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode);
void zbSocReadPower(uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode);
void zbSocGetHumid(uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode);
void zbSocGetLastMessage(uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode);
void zbSocGetCurrentPrice(uint8_t rxOnIdle, uint16_t dstAddr, uint8_t endpoint, uint8_t addrMode);

void zbSocSblHandshake(void);
void zbSocResetLocalDevice(void);
uint8_t zbSocSblInitiateImageDownload(char * filename, uint8_t enableProgressReporting);
void zbSocFinishLoadingImage(void);
//void zbSocTimeoutCallback(void);
//void zbSocExecuteTimerCallback(zllTimer * timer);
void zbSocDisableTimeout(int timer);
void zbSocEnableTimeout(int timer, uint32_t milliseconds);
void zbSocRemoveDevice(uint8_t ieeeAddr[]);
void zbSocForceRun(void);

#ifdef __cplusplus
}
#endif

#endif /* ZBSOCCMD_H */
