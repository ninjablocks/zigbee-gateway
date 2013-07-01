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
 
void changemode(int);
int  kbhit(void);

void sendLightState(uint16_t addr, uint16_t addrMode, uint16_t ep, uint8_t state);
void socketClientCb(msgData_t *msg);

void rpscKeyEstablishmentStateInd(uint8_t state);
void rpscDisplayMessageInd(uint8_t *pData, uint8_t len);
void rpscPublishPriceInd(uint8_t *pData, uint8_t len);

void sendGetLastMessage(void);
void sendGetCurrentPrice(void);

// constants
const char *keyEstablishmentStateStr[] =
{
  "Initiated",
  "Confirmed",
  "Terminated"
};

const char *msgCtrlTxModeStr[] = 
{
  "Normal transmission only",
  "Normal and Anonymous Inter-PAN transmission"
  "Anonymous Inter-PAN transmission only"
};

const char *msgCtrlImportanceStr[] =
{
  "Low",
  "Medium",
  "High",
  "Critical"
};

void changemode(int dir)
{
  static struct termios oldt, newt;
 
  if ( dir == 1 )
  {
    tcgetattr( STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~( ICANON | ECHO );
    tcsetattr( STDIN_FILENO, TCSANOW, &newt);
  }
  else
  {
    tcsetattr( STDIN_FILENO, TCSANOW, &oldt);
  }
}
 
int kbhit (void)
{
  struct timeval tv;
  fd_set rdfs;
 
  tv.tv_sec = 0;
  tv.tv_usec = 0;
 
  FD_ZERO(&rdfs);
  FD_SET (STDIN_FILENO, &rdfs);
 
  select(STDIN_FILENO+1, &rdfs, NULL, NULL, &tv);
  return FD_ISSET(STDIN_FILENO, &rdfs);
 
}

int main(int argc, char *argv[])
{
  int ch;

  socketClientInit("127.0.0.1:11235", socketClientCb);
  
  changemode(1);

  while (1)
  {
    if (kbhit())
    {
      ch = getchar();

      if (ch == 'm' || ch == 'M')
      {
        sendGetLastMessage();
      }
      else if (ch == 'p' || ch == 'P')
      {
        sendGetCurrentPrice();
      }
      else if (ch == 'q' || ch == 'Q')
      {
        break;
      }
    }
  }
   
  changemode(0);
  
  socketClientClose();
  
  return 0;
}

void socketClientCb( msgData_t *msg )
{
  switch (msg->cmdId)
  {
  case SRPC_KEY_ESTABLISHMENT_STATE_IND:
    rpscKeyEstablishmentStateInd(msg->pData[0]);
    break;
    
  case SRPC_DISPLAY_MESSAGE_IND:
    rpscDisplayMessageInd(msg->pData, msg->len);
    break;
    
  case SRPC_PUBLISH_PRICE_IND:
    rpscPublishPriceInd(msg->pData, msg->len);
    break;
    
  default:
    break;
  }
}

void rpscKeyEstablishmentStateInd(uint8_t state)
{
  if (state <= KE_STATE_TERMINATED)
  {
    printf("[Key Establishment] State = %s\n", keyEstablishmentStateStr[state]);
  }
}

void rpscDisplayMessageInd(uint8_t *pData, uint8_t len)
{
  uint32_t  messageId;
  uint8_t   msgCtrlTxModeIndex;
  uint8_t   msgCtrlImportanceIndex;
  uint8_t   msgCtrlCnfRequired;
  uint32_t  startTime;
  uint16_t  durationInMinutes;
  char      msgString[255];

  messageId = BUILD_UINT32(pData[0], pData[1], pData[2], pData[3]);
  msgCtrlTxModeIndex = pData[4] & 0x03;
  msgCtrlImportanceIndex = (pData[4] >> 2) & 0x03;
  msgCtrlCnfRequired = (pData[4] >> 7);
  startTime = BUILD_UINT32(pData[5], pData[6], pData[7], pData[8]);
  durationInMinutes = BUILD_UINT16(pData[9], pData[10]);
  memcpy(&msgString[0], &pData[12], pData[11]);
  msgString[pData[11]] = 0; // null terminate

  printf("\n== SRPC_DISPLAY_MESSAGE_IND received from the server. ==\n");
  printf("[DisplayMessage] Message ID = %u\n", messageId);
  printf("[DisplayMessage] Transmission Mode = %s\n", msgCtrlTxModeStr[msgCtrlTxModeIndex]);
  printf("[DisplayMessage] Importance = %s\n", msgCtrlImportanceStr[msgCtrlImportanceIndex]);
  printf("[DisplayMessage] Confirmation Required = %s\n", msgCtrlCnfRequired ? "Yes" : "No");

  if (startTime == 0)
  {
    printf("[DisplayMessage] Start Time = Now, ");
  }
  else
  {
    printf("[DisplayMessage] Start Time(UTC) = %d, ", startTime);
  }

  if (durationInMinutes == 0xFFFF)
  {
    printf("Duration = Until Channged\n");
  }
  else
  {
    printf("Duration = %d minutes\n", durationInMinutes);
  }

  printf("[DisplayMessage] Message = %s\n", msgString);  
}

void rpscPublishPriceInd(uint8_t *pData, uint8_t len)
{
  uint32_t  providerId;
  char rateLabel[255];
  uint32_t  issuerEventId;
  uint32_t  currentTime;
  uint8_t   unitOfMeasure;
  uint16_t  currency;
  uint8_t   priceTrailingDigit;
  uint8_t   priceTier;
  uint8_t   numberOfPriceTiers_registerTier;
  uint32_t  startTime;
  uint16_t  durationInMinutes;
  double    price;
  uint8_t   priceRatio;
  uint32_t  generationPrice;
  uint8_t   generationPriceRatio;
  uint32_t  alternateCostDelivered;      // Alternative measure of the cost of the energy consumed
  uint8_t   alternateCostUnit;           // 8-bit enum identifying the unit for Alternate Cost Delivered field
  uint8_t   alternateCostTrailingDigit;  // Location of decimal point in alternatecost field
  uint8_t   numberOfBlockThresholds;     // Number of block thresholds available
  uint8_t   priceControl;                // Additional control options for the price event

  providerId = BUILD_UINT32(pData[0], pData[1], pData[2], pData[3]);
  pData += 4;
  memcpy(&rateLabel[0], &pData[1], pData[0]);
  rateLabel[pData[0]] = 0;  // Null termination
  pData += pData[0] + 1;
  issuerEventId = BUILD_UINT32(pData[0], pData[1], pData[2], pData[3]);
  pData +=4;
  currentTime = BUILD_UINT32(pData[0], pData[1], pData[2], pData[3]);
  pData +=4;
  unitOfMeasure = *pData++;
  currency = BUILD_UINT16(pData[0], pData[1]);
  pData += 2;
  priceTrailingDigit = *pData >> 4;
  priceTier = (*pData++) & 0x0F;
  numberOfPriceTiers_registerTier = *pData++;
  startTime = BUILD_UINT32(pData[0], pData[1], pData[2], pData[3]);
  pData += 4;
  durationInMinutes = BUILD_UINT16(pData[0], pData[1]);
  pData += 2;
  price = (double) BUILD_UINT32(pData[0], pData[1], pData[2], pData[3]);

  // All optional hereafter...
  pData += 4;
  (void) priceRatio;
  (void) generationPrice;
  (void) generationPriceRatio;
  (void) alternateCostDelivered;
  (void) alternateCostUnit;
  (void) alternateCostTrailingDigit;
  (void) numberOfBlockThresholds;
  (void) priceControl;
  
  printf("\n== SRPC_PUBLISH_PRICE_IND received from the server. ==\n");
  printf("[PublishPrice] Privide ID = 0x%04x\n", providerId);
  printf("[PublishPrice] Rate Label = %s\n", rateLabel);
  printf("[PublishPrice] Issuer Event ID = %d\n", issuerEventId);
  printf("[PublishPrice] Current Time(UTC)= %d\n", currentTime);
//  printf("[PublishPrice] Unit of Measure = %d\n", unitOfMeasure);
  printf("[PublishPrice] Unit of Measure = kWh\n"); // phew...
  printf("[PublishPrice] Currency Code(ISO 4217) = %d\n", currency);
  if ((priceTier & 0x0F) == 0)
  {
    printf("[PublishPrice] Price Tier = none\n");
  }
  else
  {
    printf("[PublishPrice] Price Tier = %d\n", priceTier);
  }
  printf("[PublishPrice] Number of Price Tiers = %d\n", numberOfPriceTiers_registerTier >> 4);
  if ((numberOfPriceTiers_registerTier & 0x0F) == 0)
  {
    printf("[PublishPrice] Register Tier = none\n");
  }
  else
  {
    printf("[PublishPrice] Register Tier = %d\n", numberOfPriceTiers_registerTier & 0x0F);
  }
  if (startTime == 0)
  {
    printf("[PublishPrice] Start Time = Now, ");
  }
  else
  {
    printf("[PublishPrice] Start Time(UTC) = %d, ", startTime);
  }

  if (durationInMinutes == 0xFFFF)
  {
    printf("Duration = Until Channged\n");
  }
  else
  {
    printf("Duration = %d minutes\n", durationInMinutes);
  }

  for (int i = 0; i < priceTrailingDigit; i++)
  {
    price /= 10;
  }
  
  printf("[PublishPrice] Price = %lf\n", price);
}

void sendGetLastMessage(void)
{     
  msgData_t msg;
  uint8_t* pRpcCmd = msg.pData;		 
  		
  msg.cmdId = SRPC_GET_LAST_MESSAGE;
  msg.len = 14;
  //Addr Mode
  *pRpcCmd++ = 2; // Short Address
  //Addr
  *pRpcCmd++ = 0; // ESI(Coordinator) address
  *pRpcCmd++ = 0; // ESI(Coordinator) address    
  //index past 8byte addr
  pRpcCmd += 6;
  //Ep
  *pRpcCmd++ = 9; // SampleApp ESI address
  //Pad out Pan ID
  pRpcCmd += 2;        

  socketClientSendData(&msg);

  printf("\n== SRPC_GET_LAST_MESSAGE sent to the server.\n");
}

void sendGetCurrentPrice(void)
{     
  msgData_t msg;
  uint8_t* pRpcCmd = msg.pData;		 
  		
  msg.cmdId = SRPC_GET_CURRENT_PRICE;
  msg.len = 14;
  //Addr Mode
  *pRpcCmd++ = 2; // Short Address
  //Addr
  *pRpcCmd++ = 0; // ESI(Coordinator) address
  *pRpcCmd++ = 0; // ESI(Coordinator) address    
  //index past 8byte addr
  pRpcCmd += 6;
  //Ep
  *pRpcCmd++ = 9; // SampleApp ESI address
  //Pad out Pan ID
  pRpcCmd += 2;        

  socketClientSendData(&msg);

  printf("\n== SRPC_GET_CURRENT_PRICE sent to the server.\n");
}


