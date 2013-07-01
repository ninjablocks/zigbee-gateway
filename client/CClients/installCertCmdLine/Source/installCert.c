 /**************************************************************************************************
  Filename:       simpleIHD.c
  Revised:        $Date: 2012-03-21 17:37:33 -0700 (Wed, 21 Mar 2012) $
  Revision:       $Revision: 246 $

  Description:    This file contains an example client for the zbGateway sever

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

#include "socket_client.h"
#include "interface_srpcserver.h"
#include "hal_defs.h"

int done = 0;

void socketClientCb(msgData_t *msg);

void rpscCertInstallResultInd(uint8_t result);

// constants
const char *certInstallResultStr[] =
{
  "Completed successfully",
  "Error in opening the file",
  "Out of memory",
  "File format is wrong",
  "Server is busy",
  "Error reported from the SoC",
  "Terminated by user or error"
};

static void srpcSendInstallCert(char *filename, uint8_t force2reset)
{ 
  msgData_t srpcCmd;
  uint16_t filenamelength = strlen(filename);

  if ((filenamelength + 4) > AP_MAX_BUF_LEN)
  {
  	printf("ERROR: Filename too long\n");
  	exit(-1);
  }
  
  srpcCmd.cmdId = SRPC_INSTALL_CERTIFICATE;
  srpcCmd.len = 2 + 1 + filenamelength;
  srpcCmd.pData[0] = filenamelength & 0xFF;
  srpcCmd.pData[1] = (filenamelength >> 8) & 0xFF;
  srpcCmd.pData[2] = force2reset;
  strcpy((char *)srpcCmd.pData + 3, filename);

  socketClientSendData (&srpcCmd);
    
  return; 
}

int main(int argc, char *argv[])
{
  int ch;

  if (argc < 2)
  {
    printf("Error, wrong number of arguments. Specify filename\n");
    exit(-1);
  }

  socketClientInit("127.0.0.1:11235", socketClientCb);
  
  //send get devices command
  srpcSendInstallCert(argv[1], 1);
  
  while (!done)
  {
    sleep(1);
  }
   
  socketClientClose();
  
  return 0;
}

void socketClientCb( msgData_t *msg )
{
  if (msg->cmdId == SRPC_CERT_INSTALL_RESULT_IND)
  {
    rpscCertInstallResultInd(msg->pData[0]);
    done = 1;
  }
}

void rpscCertInstallResultInd(uint8_t result)
{
  if (result <= CERT_RESULT_TERMINATED)
  {
    printf("Certificate Installation Result: %s\n", certInstallResultStr[result]);
  }
}
