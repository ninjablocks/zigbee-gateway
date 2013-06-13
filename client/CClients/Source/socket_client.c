 /**************************************************************************************************
  Filename:       socket_client.c
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

  The following guide and a small portion of the code from Beej's Guide to Unix 
  IPC was used in the development of this software:  
  http://beej.us/guide/bgipc/output/html/multipage/intro.html#audience. 
  The code is Public Domain   
**************************************************************************************************/

/**************************************************************************************************
 *                                           Includes
 **************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>
#include <poll.h>
#include <sys/time.h>
#include <unistd.h>

#ifndef NPI_UNIX
#include <netdb.h>
#include <arpa/inet.h>
#endif

#include "socket_client.h"

#ifdef __BIG_DEBUG__
#define debug_printf(fmt, ...) printf( fmt, ##__VA_ARGS__)
#else
#define debug_printf(fmt, ...)
#endif

#define msg_memcpy(src, dst, len)	memcpy(src, dst, len)

/**************************************************************************************************
 *                                        Externals
 **************************************************************************************************/

/**************************************************************************************************
 *                                           Constant
 **************************************************************************************************/

/**************************************************************************************************
 *                                        Type definitions
 **************************************************************************************************/

struct LinkedMsg
{
	msgData_t message;
	void *nextMessage;
};

typedef struct LinkedMsg linkedMsg;

#define TX_BUF_SIZE			(2 * (sizeof(msgData_t)))


/**************************************************************************************************
 *                                        Global Variables
 **************************************************************************************************/

/**************************************************************************************************
 *                                        Local Variables
 **************************************************************************************************/

// Client socket handle
int sClientFd;
// Client data transmission buffers
char socketBuf[2][TX_BUF_SIZE];
// Client data received buffer
linkedMsg *rxBuf;
// Client data processing buffer
linkedMsg *rxProcBuf;

// Message count to keep track of incoming and processed messages
static int messageCount = 0;

static pthread_t RTISThreadId;
static void *rxThreadFunc (void *ptr);
static void *handleThreadFunc (void *ptr);

// Mutex to handle rx
pthread_mutex_t clientRxMutex = PTHREAD_MUTEX_INITIALIZER;

// conditional variable to notify that the AREQ is handled
static pthread_cond_t clientRxCond;

#ifndef NPI_UNIX
struct addrinfo *resAddr;
#endif

socketClientCb_t socketClientRxCb;

/**************************************************************************************************
 *                                     Local Function Prototypes
 **************************************************************************************************/
static void initSyncRes(void);
static void delSyncRes(void);



/**************************************************************************************************
 *
 * @fn          socketClientInit
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
 * @return      1 if the surrogate module started off successfully.
 *              0, otherwise.
 *
 **************************************************************************************************/
int socketClientInit(const char *devPath, socketClientCb_t cb)
{
	int res = 1, i = 0;
	const char *ipAddress = "", *port = "";
	char *pStr, strTmp[128];
	
	socketClientRxCb = cb;
	
	strncpy(strTmp, devPath, 128);
	// use strtok to split string and find IP address and port;
	// the format is = IPaddress:port
	// Get first token
	pStr = strtok (strTmp, ":");
	while ( pStr != NULL)
	{
		if (i == 0)
		{
			// First part is the IP address
			ipAddress = pStr;
		}
		else if (i == 1)
		{
			// Second part is the port
			port = pStr;
		}
		i++;
		if (i > 2)
			break;
		// Now get next token
		pStr = strtok (NULL, " ,:;-|");
	}

	/**********************************************************************
	 * Initiate synchronization resources
	 */
	initSyncRes();

	/**********************************************************************
	 * Connect to the NPI server
	 **********************************************************************/

#ifdef NPI_UNIX
    int len;
    struct sockaddr_un remote;

    if ((sClientFd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }
#else
    struct addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

//    ipAddress = "192.168.128.133";
//    if ((res = getaddrinfo(NULL, ipAddress, &hints, &resAddr)) != 0)
	if (port == NULL)
	{
		// Fall back to default if port was not found in the configuration file
		printf("Warning! Port not sent to RTIS. Will use default port: %s", DEFAULT_PORT);

	    if ((res = getaddrinfo(ipAddress, DEFAULT_PORT, &hints, &resAddr)) != 0)
	    {
	    	fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(res));
	    	res = 2;
	    }
	    else
	    {
	    	// Because of inverted logic on return value
	    	res = 1;
	    }
	}
	else
	{
	    printf("Port: %s\n\n", port);
	    if ((res = getaddrinfo(ipAddress, port, &hints, &resAddr)) != 0)
	    {
	    	fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(res));
	    	res = 2;
	    }
	    else
	    {
	    	// Because of inverted logic on return value
	    	res = 1;
	    }
	}

    printf("IP addresses for %s:\n\n", ipAddress);

    struct addrinfo *p;
    char ipstr[INET6_ADDRSTRLEN];
    for(p = resAddr;p != NULL; p = p->ai_next) {
        void *addr;
        char *ipver;

        // get the pointer to the address itself,
        // different fields in IPv4 and IPv6:
        if (p->ai_family == AF_INET) { // IPv4
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
            addr = &(ipv4->sin_addr);
            ipver = "IPv4";
        } else { // IPv6
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
            addr = &(ipv6->sin6_addr);
            ipver = "IPv6";
        }

        // convert the IP to a string and print it:
        inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
        printf("  %s: %s\n", ipver, ipstr);
    }


    if ((sClientFd = socket(resAddr->ai_family, resAddr->ai_socktype, resAddr->ai_protocol)) == -1) {
        perror("socket");
        exit(1);
    }
#endif

    printf("Trying to connect...\n");

#ifdef NPI_UNIX
    remote.sun_family = AF_UNIX;
    strcpy(remote.sun_path, ipAddress);
    len = strlen(remote.sun_path) + sizeof(remote.sun_family);
    if (connect(sClientFd, (struct sockaddr *)&remote, len) == -1) {
        perror("connect");
        res = 0;
    }
#else
    if (connect(sClientFd, resAddr->ai_addr, resAddr->ai_addrlen) == -1) {
        perror("connect");
        res = 0;
    }
#endif

    if (res == 1)
    	printf("Connected.\n");


	int no = 0;
	// allow out-of-band data
	if (setsockopt(sClientFd, SOL_SOCKET, SO_OOBINLINE, &no, sizeof(int)) == -1)
	{
		perror("setsockopt");
		res = 0;
	}

	/**********************************************************************
	 * Create thread which can read new messages from the NPI server
	 **********************************************************************/

	if (pthread_create(&RTISThreadId, NULL, rxThreadFunc, NULL))
	{
		// thread creation failed
		printf("Failed to create RTIS LNX IPC Client read thread\n");
		return -1;
	}

	/**********************************************************************
	 * Create thread which can handle new messages from the NPI server
	 **********************************************************************/

	if (pthread_create(&RTISThreadId, NULL, handleThreadFunc, NULL))
	{
		// thread creation failed
		printf("Failed to create RTIS LNX IPC Client handle thread\n");
		return -1;
	}

	return res;
}

static void *handleThreadFunc (void *ptr)
{
	int done = 0, tryLockFirstTimeOnly = 0;
	
	// Handle message from socket
	do {

    if (tryLockFirstTimeOnly == 0)
		{
			// Lock mutex
			debug_printf("[MUTEX] Lock AREQ Mutex (Handle)\n");
			pthread_mutex_lock(&clientRxMutex);

			debug_printf("\n[MUTEX] AREQ Lock (Handle)\n");
			tryLockFirstTimeOnly = 1;
		}
		
		// Conditional wait for the response handled in the Rx handling thread,
		debug_printf("[MUTEX] Wait for Rx Cond (Handle) signal...\n");
		pthread_cond_wait(&clientRxCond, &clientRxMutex);

		debug_printf("[MUTEX] (Handle) has lock\n");

		// Walk through all received AREQ messages before releasing MUTEX
		linkedMsg *searchList = rxProcBuf, *clearList;
		while (searchList != NULL)
		{
			debug_printf("\n\n[DBG] Processing \t@ 0x%.16X next \t@ 0x%.16X\n",
					(unsigned int)searchList,
					(unsigned int)(searchList->nextMessage));

      if( socketClientRxCb != NULL)
      {
			  debug_printf("[MUTEX] Calling socketClientRxCb (Handle)...\n");
			  socketClientRxCb((msgData_t *)&(searchList->message));
			}

			debug_printf("[MUTEX] (Handle) (message @ 0x%.16X)...\n", (unsigned int)searchList);

			clearList = searchList;
			// Set search list to next message
			searchList = searchList->nextMessage;
			// Free processed buffer
			if (clearList == NULL)
			{
				// Impossible error, must abort
				done = 1;
				printf("[ERR] clearList buffer was already free\n");
				break;
			}
			else
			{
				messageCount--;
				debug_printf("[DBG] Clearing \t\t@ 0x%.16X (processed %d messages)...\n",
						(unsigned int)clearList,
						messageCount);
				memset(clearList, 0, sizeof(linkedMsg));				
				free(clearList);								
			}
		}
		debug_printf("[MUTEX] Signal message(s) handled (Handle) (processed %d messages)...\n", messageCount);

		debug_printf("[DBG] Finished processing (processed %d messages)...\n",
				messageCount);

	} while (!done);

	return ptr;
}

static void *rxThreadFunc (void *ptr)
{
	int done = 0, n;

	/* thread loop */

	struct pollfd ufds[1];
	int pollRet;
	ufds[0].fd = sClientFd;
	ufds[0].events = POLLIN | POLLPRI;

	// Read from socket
	do {
		pollRet = poll((struct pollfd*)&ufds, 1, 1);
		if (pollRet == -1)
		{
			// Error occured in poll()
			perror("poll");
		}
		else if (pollRet == 0)
		{
			// Timeout, could still be AREQ to process
		}
		else
		{		  
			if (ufds[0].revents & POLLIN) {
				n = recv(sClientFd,
						socketBuf[0],
						SRPC_FRAME_HDR_SZ,
						0); // normal data
			}
			if (ufds[0].revents & POLLPRI) {
				n = recv(sClientFd,
						socketBuf[0],
						SRPC_FRAME_HDR_SZ,
						MSG_OOB); // out-of-band data
			}
			if (n <= 0)
			{
				if (n < 0)
					perror("recv");
				done = 1;
			}
			else if (n == SRPC_FRAME_HDR_SZ)
			{
				// We have received the header, now read out length byte and process it
				n = recv(sClientFd,
						(uint8_t*)&(socketBuf[0][SRPC_FRAME_HDR_SZ]),
						((msgData_t *)&(socketBuf[0][0]))->len,
						0);
				if (n == ((msgData_t *)&(socketBuf[0][0]))->len)
				{
					int i;
					debug_printf("Received %d bytes,\t cmdId 0x%.2X, pData:\t",
							((msgData_t *)&(socketBuf[0][0]))->len,
							((msgData_t *)&(socketBuf[0][0]))->cmdId);
					for (i = SRPC_FRAME_HDR_SZ; i < (n + SRPC_FRAME_HDR_SZ); i++)
					{
						debug_printf(" 0x%.2X", (uint8_t)socketBuf[0][i]);
					}
					debug_printf("\n");

  				// Allocate memory for new message
					linkedMsg *newMessage = (linkedMsg *) malloc(sizeof(linkedMsg));
					
					//debug_printf("Freeing new message (@ 0x%.16X)...\n", (unsigned int)newMessage);
					//free(newMessage);
					
					if (newMessage == NULL)
					{
						// Serious error, must abort
						done = 1;
						printf("[ERR] Could not allocate memory for AREQ message\n");
						break;
					}
					else
					{
						messageCount++;
						memset(newMessage, 0, sizeof(linkedMsg));
						debug_printf("\n[DBG] Allocated \t@ 0x%.16X (received\040 %d messages), size:%x...\n",
								(unsigned int)newMessage,
								messageCount,
								sizeof(linkedMsg));
					}
					
					debug_printf("Filling new message (@ 0x%.16X)...\n", (unsigned int)newMessage);

					// Copy AREQ message into AREQ buffer
					memcpy(&(newMessage->message),
							(uint8_t*)&(socketBuf[0][0]),
							(((msgData_t *)&(socketBuf[0][0]))->len + SRPC_FRAME_HDR_SZ));

					// Place message in read list
					if (rxBuf == NULL)
					{
						// First message in list
						rxBuf = newMessage;				
					}
					else
					{
						linkedMsg *searchList = rxBuf;
						// Find last entry and place it here
						while (searchList->nextMessage != NULL)
						{
							searchList = searchList->nextMessage;
						}
						searchList->nextMessage = newMessage;
					}
				}
				else
				{
					// Serious error
					printf("ERR: Incoming Rx has incorrect length field; %d\n",
							((msgData_t *)&(socketBuf[0][0]))->len);

					debug_printf("[MUTEX] Unlock Rx Mutex (Read)\n");
					// Then unlock the thread so the handle can handle the AREQ
					pthread_mutex_unlock(&clientRxMutex);
				}
		  }
		  else
		  {
		  	// Impossible. ... ;)
		  }
		}

		// Handle thread must make sure it has finished its list. See if there are new messages to move over
		if (rxBuf != NULL)
		{
		    pthread_mutex_lock(&clientRxMutex);
		    
				// Move list over for processing
				rxProcBuf = rxBuf;			
				
				// Clear receiving buffer for new messages
				rxBuf = NULL;

				debug_printf("[DBG] Copied message list (processed %d messages)...\n",
						messageCount);
				
				debug_printf("[MUTEX] Unlock Mutex (Read)\n");
				// Then unlock the thread so the handle can handle the AREQ
				pthread_mutex_unlock(&clientRxMutex);
				
				debug_printf("[MUTEX] Signal message read (Read)...\n");
				// Signal to the handle thread an AREQ message is ready
				pthread_cond_signal(&clientRxCond);
				debug_printf("[MUTEX] Signal message read (Read)... sent\n");
		}

	} while (!done);


	return ptr;
}

/* Initialize thread synchronization resources */
static void initSyncRes(void)
{
  // initialize all mutexes
  pthread_mutex_init(&clientRxMutex, NULL);

  // initialize all conditional variables
  pthread_cond_init(&clientRxCond, NULL);
}

/* Destroy thread synchronization resources */
static void delSyncRes(void)
{
  // In Linux, there is no dynamically allocated resources
  // and hence the following calls do not actually matter.

  // destroy all conditional variables
  pthread_cond_destroy(&clientRxCond);

  // destroy all mutexes
  pthread_mutex_destroy(&clientRxMutex);

}

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
void socketClientClose(void)
{
	// Close the NPI socket connection

	close(sClientFd);

	// Delete synchronization resources
	delSyncRes();

#ifndef NPI_UNIX
	freeaddrinfo(resAddr); // free the linked-list
#endif //NPI_UNIX

}

/**************************************************************************************************
 *
 * @fn          socketClientSendData
 *
 * @brief       This function sends a message asynchronously over the socket
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
void socketClientSendData (msgData_t *pMsg)
{
	int i;
	debug_printf("trying to send %d bytes,\t cmdId 0x%.2X, pData:",
			pMsg->len,
			pMsg->cmdId);
	debug_printf("\t");
	for (i = 0; i < pMsg->len; i++)
	{
		debug_printf(" 0x%.2X", pMsg->pData[i]);
	}
	debug_printf("\n");

	if (send(sClientFd, ((uint8_t*)pMsg), pMsg->len + SRPC_FRAME_HDR_SZ , 0) == -1)
	{
		perror("send");
		exit(1);
	}
}

/**************************************************************************************************
 **************************************************************************************************/
