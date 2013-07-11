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
#include <stdint.h>

#include "socket_client.h"
#include "interface_srpcserver.h"
#include "hal_defs.h"
#include "SimpleDB.h"

int keyFd;

#define CONSOLEDEVICE "/dev/console"

void socketClientZllCb( msgData_t *msg ); 
static void srpcSendRemoveDevice( uint8_t ieeeAddr[Z_EXTADDR_LEN]);

typedef uint8_t (*rpcsProcessMsg_t)(uint8_t *msg);

int main(int argc, char *argv[])
{
  char * pBuf;
  parsingResult_t result = {0, SDB_TXT_PARSER_RESULT_OK, 0};
  uint8_t ieeeAddr[Z_EXTADDR_LEN];

  if (argc < 2)
  {
  	printf("ERROR: Missing argument\n");
	exit(0);
  }

  pBuf = argv[1];
  sdb_txt_parser_get_hex_field(&pBuf, ieeeAddr, Z_EXTADDR_LEN, &result);

  if ((result.code != SDB_TXT_PARSER_RESULT_OK) && (result.code != SDB_TXT_PARSER_RESULT_REACHED_END_OF_RECORD))
  {
	  printf("Argument error:\n");
	  printf("  %s\n", argv[1]);
	  printf("  %*s %s\n", result.errorLocation - argv[1] + 1, "^", parsingErrorStrings[result.code]);
	  exit(0);
  }
      
  socketClientInit("127.0.0.1:11235", socketClientZllCb);
  
  //send get devices command
  printf("Removing %s\n", argv[1]);
  srpcSendRemoveDevice(ieeeAddr);

  sleep(2);
  printf("Done\n");
}

//Process the message from SE-Interface
void socketClientZllCb( msgData_t *msg )
{
	return;
}

static void srpcSendRemoveDevice( uint8_t ieeeAddr[Z_EXTADDR_LEN])
{ 
  msgData_t srpcCmd;
  
  srpcCmd.cmdId = SRPC_REMOVE_DEVICE;
  srpcCmd.len = 8;

  memcpy(srpcCmd.pData, ieeeAddr, Z_EXTADDR_LEN);

  socketClientSendData (&srpcCmd);
    
  return; 
}
