/**************************************************************************************************
 * Filename:       socket_server.h
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
 */
 
#ifndef SOCKET_SERVER_H
#define SOCKET_SERVER_H

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************
 * TYPEDEFS
 */
 
typedef void (*socketServerCb_t)( int clientFd );
 
/*********************************************************************
 * INCLUDES
 */
#include "hal_types.h"

/*********************************************************************
 * CONSTANTS
 */
//#define SOCKET_SERVER_PORT 1234
#define MAX_CLIENTS 50

/*
 * serverSocketInit - initialises the server.
 */      
int32 socketSeverInit( uint32 port );

/*
 * serverSocketConfig - initialises the server.
 */      
int32 serverSocketConfig(socketServerCb_t rxCb, socketServerCb_t connectCb);

/*
 * getClientFds -  get clients fd's.
 */
void socketSeverGetClientFds(int *fds, int maxFds);

/* 
 * getClientFds - get clients fd's. 
 */
uint32 socketSeverGetNumClients(void);

/*
 * socketSeverPoll - services the Socket events.
 */
void socketSeverPoll(int clinetFd, int revent);

/*
 * socketSeverSendAllclients - Send a buffer to all clients.
 */
int32 socketSeverSendAllclients(uint8* buf, uint32 len);

/*
 * socketSeverSend - Send a buffer to a clients.
 */
int32 socketSeverSend(uint8* buf, uint32 len, int32 fdClient);


/*
 * socketSeverClose - Closes the client connections.
 */
void socketSeverClose(void);


#ifdef __cplusplus
}
#endif

#endif /* SOCKET_SERVER_H */