/**************************************************************************************************
 * Filename:       socket_server.c
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
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <errno.h>

#include "socket_server.h"

#define MAX_CLIENTS 50

#define SOCKET_BOOTLOADING_STATE_IDLE 0
#define SOCKET_BOOTLOADING_STATE_ACTIVE 1

//todo: use a callback instead.
void SRPC_killLoadingImage(void);
extern uint16_t SocketBootloadingState;
extern uint32_t bootloader_initiator_clientFd;


/*********************************************************************
 * TYPEDEFS
 */ 
typedef struct
{
  void   *next;
  int socketFd;
  socklen_t clilen;
  struct sockaddr_in cli_addr;
} socketRecord_t;


/*********************************************************************
 * GLOBAL VARIABLES
 */
socketRecord_t *socketRecordHead = NULL;

socketServerCb_t socketServerRxCb;
socketServerCb_t socketServerConnectCb;
	
 
/*********************************************************************
 * LOCAL FUNCTION PROTOTYPES
 */ 
static void deleteSocketRec( int rmSocketFd );
static int createSocketRec( void );

/*********************************************************************
 * FUNCTIONS
 *********************************************************************/

/*********************************************************************
 * @fn      createSocketRec
 *
 * @brief   create a socket and add a rec fto the list.
 *
 * @param   table
 * @param   rmTimer
 *
 * @return  new clint fd
 */
int createSocketRec( void  )
{
  int tr=1;
  socketRecord_t *srchRec;
  
  socketRecord_t *newSocket = malloc( sizeof( socketRecord_t ) );
  
  //open a new client connection with the listening socket (at head of list)
  newSocket->clilen = sizeof(newSocket->cli_addr);
      
  //Head is always the listening socket
  newSocket->socketFd = accept(socketRecordHead->socketFd, 
               (struct sockaddr *) &(newSocket->cli_addr), 
               &(newSocket->clilen));

   //printf("connected\n");
                 
   if (newSocket->socketFd < 0) 
        printf("ERROR on accept");

   // Set the socket option SO_REUSEADDR to reduce the chance of a 
   // "Address Already in Use" error on the bind
   setsockopt(newSocket->socketFd,SOL_SOCKET,SO_REUSEADDR,&tr,sizeof(int));
   // Set the fd to none blocking
   fcntl(newSocket->socketFd, F_SETFL, O_NONBLOCK);
   
   //printf("New Client Connected fd:%d - IP:%s\n", newSocket->socketFd, inet_ntoa(newSocket->cli_addr.sin_addr));
   
   newSocket->next = NULL;
   
   //find the end of the list and add the record
   srchRec = socketRecordHead;
   // Stop at the last record
   while ( srchRec->next )
     srchRec = srchRec->next;

   // Add to the list
   srchRec->next = newSocket; 

   return(newSocket->socketFd);
}


 
/*********************************************************************
 * @fn      deleteSocketRec
 *
 * @brief   Delete a rec from list.
 *
 * @param   table
 * @param   rmTimer
 *
 * @return  none
 */
void deleteSocketRec( int rmSocketFd )
{
  socketRecord_t *srchRec, *prevRec=NULL;

  // Head of the timer list
  srchRec = socketRecordHead;        
  
  // Stop when rec found or at the end
  while ( (srchRec->socketFd != rmSocketFd) && (srchRec->next) )
  {
    prevRec = srchRec;  
    // over to next
    srchRec = srchRec->next;        
  }
  
  if (srchRec->socketFd != rmSocketFd)
  {
      printf("deleteSocketRec: record not found\n");
      return;    
  }
  
  // Does the record exist
  if ( srchRec )
  {               
    // delete the timer from the list
    if ( prevRec == NULL )      
    {
      //trying to remove first rec, which is always the listining socket
      printf("deleteSocketRec: removing first rec, which is always the listining socket\n");
      return; 
    }
    
    //remove record from list    
    prevRec->next = srchRec->next;    
      
    close(srchRec->socketFd);
    free(srchRec);        
  }
}

/***************************************************************************************************
 * @fn      serverSocketInit
 *
 * @brief   initialises the server.
 * @param   
 *
 * @return  Status
 */      
int32 socketSeverInit( uint32_t port )
{
  struct sockaddr_in serv_addr;
  int stat, tr=1;
  
  if(socketRecordHead == NULL)   
  {
    // New record
    socketRecord_t *lsSocket = malloc( sizeof( socketRecord_t ) );
  
    lsSocket->socketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (lsSocket->socketFd < 0) 
    {
       printf("ERROR opening socket");
       return -1;
    }
    
    // Set the socket option SO_REUSEADDR to reduce the chance of a 
    // "Address Already in Use" error on the bind
    setsockopt(lsSocket->socketFd,SOL_SOCKET,SO_REUSEADDR,&tr,sizeof(int));
    // Set the fd to none blocking
    fcntl(lsSocket->socketFd, F_SETFL, O_NONBLOCK);
      
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);
    stat = bind(lsSocket->socketFd, (struct sockaddr *) &serv_addr,
             sizeof(serv_addr));
    if ( stat < 0) 
    {
      printf("ERROR on binding: %s\n", strerror( errno ) );
      return -1;
    }
    //will have 5 pending open client requests
    listen(lsSocket->socketFd,5); 
    
    lsSocket->next = NULL;
    //Head is always the listening socket
    socketRecordHead = lsSocket;   
  }
    
  //printf("waiting for socket new connection\n");  
  
  return 0;
}

/***************************************************************************************************
 * @fn      serverSocketConfig
 *
 * @brief   register the Rx Callback.
 * @param   
 *
 * @return  Status
 */      
int32 serverSocketConfig(socketServerCb_t rxCb, socketServerCb_t connectCb)
{
  socketServerRxCb = rxCb;
  socketServerConnectCb = connectCb;
  
  return 0;
}
/*********************************************************************
 * @fn      socketSeverGetClientFds()
 *
 * @brief   get clients fd's.
 *
 * @param   none
 *
 * @return  list of Timerfd's
 */
void socketSeverGetClientFds(int *fds, int maxFds)
{  
  uint32_t recordCnt=0;
  socketRecord_t *srchRec;

  // Head of the timer list
  srchRec = socketRecordHead; 
    
  // Stop when at the end or max is reached
  while ( (srchRec) && (recordCnt < maxFds) )
  {  
    //printf("getClientFds: adding fd%d, to idx:%d \n", srchRec->socketFd, recordCnt);
    fds[recordCnt++] = srchRec->socketFd;
    
    srchRec = srchRec->next;      
  }
      	
  return;
}

/*********************************************************************
 * @fn      socketSeverGetNumClients()
 *
 * @brief   get clients fd's.
 *
 * @param   none
 *
 * @return  list of Timerfd's
 */
uint32_t socketSeverGetNumClients(void)
{  
  uint32_t recordCnt=0;
  socketRecord_t *srchRec;
  
  //printf("socketSeverGetNumClients++\n", recordCnt);
  
  // Head of the timer list
  srchRec = socketRecordHead;
  
  if(srchRec==NULL)
  {
    //printf("socketSeverGetNumClients: socketRecordHead NULL\n");
    return -1;
  }
    
  // Stop when rec found or at the end
  while ( srchRec )
  {  
    //printf("socketSeverGetNumClients: recordCnt=%d\n", recordCnt);
    srchRec = srchRec->next;  
    recordCnt++;      
  }
  
  //printf("socketSeverGetNumClients %d\n", recordCnt);
  return (recordCnt);
}


/*********************************************************************
 * @fn      socketSeverPoll()
 *
 * @brief   services the Socket events.
 *
 * @param   clinetFd - Fd to services
 * @param   revent - event to services
 *
 * @return  none
 */
void socketSeverPoll(int clientFd, int revent)
{
  //printf("pollSocket++\n");

  //is this a new connection on the listening socket
  if(clientFd == socketRecordHead->socketFd)
  {
    int newClientFd = createSocketRec();
    
    if(socketServerConnectCb)
    {
      socketServerConnectCb(newClientFd);
    }            
  }
  else
  {
    //this is a client socket is it a input or shutdown event
    if (revent & POLLIN)
    {
      //its a Rx event
//      printf("got Rx on fd %d, pakcetCnt=%d\n", clientFd, pakcetCnt++);      
      if(socketServerRxCb)
      {
        socketServerRxCb(clientFd);
      }
             
    } 
    if (revent & POLLRDHUP)
    {
      //its a shut down close the socket
//      printf("Client fd:%d disconnected\n", clientFd); 

//todo: use a callback instead.
	  if ((SocketBootloadingState != SOCKET_BOOTLOADING_STATE_IDLE) && (clientFd == bootloader_initiator_clientFd))
	  {
	  	SRPC_killLoadingImage( );
		printf("Image download aborted by client disconnection\n");
	  }
      
      //remove the record and close the socket
      deleteSocketRec(clientFd);              
    }     
  }
   
     
     //write(clientSockFd,"I got your message",18);
     
     return;
}

/***************************************************************************************************
 * @fn      socketSeverSend
 *
 * @brief   Send a buffer to a clients.
 * @param   uint8* srpcMsg - message to be sent
 *          int32 fdClient - Client fd
 *
 * @return  Status
 */
int32 socketSeverSend(uint8* buf, uint32_t len, int32 fdClient)
{ 
  int32 rtn;

  //printf("socketSeverSend++: writing to socket fd %d\n", fdClient);
  
  if(fdClient)
  {
    rtn = write(fdClient, buf, len);
    if (rtn < 0) 
    {
      printf("ERROR writing to socket %d\n", fdClient);       
      return rtn;
    }
  }
   
  //printf("socketSeverSend--\n");
  return 0;
}  
  
  
/***************************************************************************************************
 * @fn      socketSeverSendAllclients
 *
 * @brief   Send a buffer to all clients.
 * @param   uint8* srpcMsg - message to be sent
 *
 * @return  Status
 */
int32 socketSeverSendAllclients(uint8* buf, uint32_t len)
{ 
  int rtn;
  socketRecord_t *srchRec;
   
  // first client socket
  srchRec = socketRecordHead->next; 
    
  // Stop when at the end or max is reached
  while ( srchRec )
  { 
//    printf("SRPC_Send: client %d\n", cnt++);
    rtn = write(srchRec->socketFd, buf, len);
    if (rtn < 0) 
    {
      printf("ERROR writing to socket %d\n", srchRec->socketFd);
      printf("closing client socket\n");
      //remove the record and close the socket
      deleteSocketRec(srchRec->socketFd);
      
      return rtn;
    }
    srchRec = srchRec->next;      
  }    
    
  return 0; 
}

/***************************************************************************************************
 * @fn      socketSeverClose
 *
 * @brief   Closes the client connections.
 *
 * @return  Status
 */
void socketSeverClose(void)
{
  int fds[MAX_CLIENTS], idx=0;
  
  socketSeverGetClientFds(fds, MAX_CLIENTS);
  
  while(socketSeverGetNumClients() > 1)
  {         
    printf("socketSeverClose: Closing socket fd:%d\n", fds[idx]);
    deleteSocketRec( fds[idx++] );
  }
    
  //Now remove the listening socket
  if(fds[0])
  {
    printf("socketSeverClose: Closing the listening socket\n");
    close(fds[0]);
  }
}

