 /**************************************************************************************************
  Filename:       flashled.c
  Revised:        $Date: 2012-03-21 17:37:33 -0700 (Wed, 21 Mar 2012) $
  Revision:       $Revision: 246 $

  Description:    This file contains an example client for the zllGateway sever

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
#include <string.h>


#include "socket_client.h"
#include "interface_srpcserver.h"
#include "hal_defs.h"
#include "zbSocCmd.h"

int keyFd;

#define CONSOLEDEVICE "/dev/console"

void socketClientZllCb( msgData_t *msg ); 
uint8_t RPSC_ZLL_NewDevice(uint8_t *msg);
static void srpcSendDownloadImage( char * filename, uint8_t enableProgressReporting );
static void srpcSendAbortLoadingImage( void );

typedef uint8_t (*rpcsProcessMsg_t)(uint8_t *msg);

rpcsProcessMsg_t rpcsProcessZllIncoming[] =
{
  NULL,
  RPSC_ZLL_NewDevice, 
};

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

#define BUILD_UINT32(Byte0, Byte1, Byte2, Byte3) \
          ((uint32_t)((uint32_t)((Byte0) & 0x00FF) \
          + ((uint32_t)((Byte1) & 0x00FF) << 8) \
          + ((uint32_t)((Byte2) & 0x00FF) << 16) \
          + ((uint32_t)((Byte3) & 0x00FF) << 24)))


int ignore_response = 0;

int main(int argc, char *argv[])
{
  int ret;
      
  socketClientInit("127.0.0.1:11235", socketClientZllCb);

  if (argc < 3)
  {
  	printf("Error, wrong number of argument. specify filename and reporting_interval\n");
  	exit(-1);
  }
  if ((argc > 3) && (strcmp(argv[3], "force") == 0))
  {
	  srpcSendAbortLoadingImage();
	  ignore_response = 1;
	  sleep(1);
  }
  //send get devices command
  srpcSendDownloadImage(argv[1], atoi(argv[2]));
  
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
	  	srpcSendAbortLoadingImage();
//		srpcSendDownloadImage(argv[1], 0);
		sleep(1);
        //Any key ppress will exit
        socketClientClose();
        exit(0);
      }
    }    
  }
   
  socketClientClose();
  
  return 0;
}

//Process the message from SE-Interface
void socketClientZllCb( msgData_t *msg )
{
  if (msg->cmdId == SRPC_SBL_RSP)
  {
  	printf("Bootloader message: %s\n", msg->pData[0] < (sizeof(BOOTLOADER_RESULT_STRINGS) / sizeof(BOOTLOADER_RESULT_STRINGS[0])) ? BOOTLOADER_RESULT_STRINGS[msg->pData[0]] : "UNKNOWN");
	if (msg->pData[0] == SBL_BUSY)
	{
		printf("Cannot initiate an image download, since another client\n"
			   "is currently downloading an image.\n"
			   "Add \"force\" to the end of the command line to abort the\n"
			   "other download, and retry downloading.\n");
	}

	if (msg->pData[0] == SBL_PENDING)
	{
		printf("Press ENTER to abort.\n");
	}
	else if (!ignore_response ) // && (msg->pData[0] != LOAD_IMAGE_PENDING))// && (msg->pData[0] != LOAD_IMAGE_REMOTE_ABORTED_BY_USER))
	{
		socketClientClose();
		exit (0);
	}

	ignore_response = 0;
  }
  else if (msg->cmdId == SRPC_SBL_PROGRESS)
  {
  	printf("Bootloader message: %s address 0x%08X\n", msg->pData[0] == 1 ? "HANDSHAKING" : msg->pData[0] == 2 ? "WRITING" : msg->pData[0] == 3 ? "READING" : "EXECUTING", BUILD_UINT32(msg->pData[1], msg->pData[2], msg->pData[3], msg->pData[4]));
  }
  else
  {
    printf("Error: no processing function for CMD 0x%x\n", msg->cmdId); 
  }
      
}

static void srpcSendAbortLoadingImage( void )
{
	msgData_t srpcCmd;
	srpcCmd.cmdId = SRPC_SBL_ABORT;
	srpcCmd.len = 0;
	
	socketClientSendData (&srpcCmd);
}

static void srpcSendDownloadImage( char * filename, uint8_t enableProgressReporting )
{ 
  msgData_t srpcCmd;
  uint16_t filenamelength = strlen(filename);

  if ((filenamelength + 4) > AP_MAX_BUF_LEN)
  {
  	printf("ERROR: Filename too long\n");
  	exit(-1);
  }
  
  srpcCmd.cmdId = SRPC_SBL_DOWNLOAD_IMAGE;
  srpcCmd.len = 2 + 1 + filenamelength;
  srpcCmd.pData[0] = filenamelength & 0xFF;
  srpcCmd.pData[1] = (filenamelength >> 8) & 0xFF;
  srpcCmd.pData[2] = enableProgressReporting;
  strcpy((char *)srpcCmd.pData + 3, filename);

  socketClientSendData (&srpcCmd);
    
  return; 
}

/*********************************************************************
 * @fn          RPSC_ZLL_NewDevice
 *
 * @brief       This function proccesses the NewDevice message from the ZLL Gateway.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
uint8_t RPSC_ZLL_NewDevice(uint8_t *pMsg)
{    
  epInfo_t epInfo;
  uint8_t devNameStrLen;
  static int i=0;

  epInfo.nwkAddr = BUILD_UINT16(pMsg[0], pMsg[1]);
  pMsg+=2;

  epInfo.endpoint = *pMsg++;
  
  epInfo.profileID = BUILD_UINT16(pMsg[0], pMsg[1]);
  pMsg+=2;
  
  epInfo.deviceID = BUILD_UINT16(pMsg[0], pMsg[1]);
  pMsg+=2;

  epInfo.version = *pMsg++;
  
  //skip name for now
  devNameStrLen = *pMsg;
  pMsg += devNameStrLen + 1;

  epInfo.status = *pMsg++;
    
  printf("RPSC_ZLL_NewDevice[%d]: %x:%x\n", i++, epInfo.nwkAddr, epInfo.endpoint);  
    
  
  return 0;  
}
