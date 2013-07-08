/*
 * zbSocTransportSpi.c
 *
 * This module contains the API for the zll SoC Host Interface.
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

#include "zbSocCmd.h"
#include "spidev.h"
#include "pthread.h"

/*********************************************************************
 * MACROS
 */ 
#define SPI_MAX_PKT_LEN            256
#define SPI_MAX_DAT_LEN           (SPI_MAX_PKT_LEN - SPI_FRM_LEN)

#define SPI_DAT_LEN(PBUF)      ((PBUF)[SPI_LEN_IDX])
#define SPI_PKT_LEN(PBUF)      (SPI_DAT_LEN((PBUF)) + SPI_FRM_LEN)

#define SPI_LEN_T_INCR(LEN)  (LEN)++

#define SPI_SOF                    0xFE  // Start-of-frame delimiter for SPI transport.

// The FCS is calculated over the SPI Frame Header and the Frame Data bytes.
#define SPI_HDR_LEN                1     // One byte LEN pre-pended to data byte array.
// SOF & Header bytes pre-pended and the FCS byte appended.
#define SPI_FRM_LEN               (2 + SPI_HDR_LEN)

#define SPI_SOF_IDX                0
#define SPI_HDR_IDX                1     // The frame header consists only of the LEN byte for now.
#define SPI_LEN_IDX                1     // LEN byte is offset by the SOF byte.
#define SPI_DAT_IDX                2     // Data bytes are offset by the SOF & LEN bytes.

//RF2_GPIO0/SRDY GPIO74
//RF1_SPI_ENABLE GPIO74
//RF2_SPI_ENABLE GPIO74
#define HAL_SRDY_VALUE "/sys/class/gpio/gpio9/value"
#define HAL_SRDY_DIR "/sys/class/gpio/gpio9/direction"
#define HAL_SRDY_EDGE "/sys/class/gpio/gpio9/edge"
#define HAL_SPIENABLE_VALUE "/sys/class/gpio/gpio73/value"
#define HAL_SPIENABLE_DIR "/sys/class/gpio/gpio73/direction"

/************************************************************
 * TYPEDEFS
 */
typedef enum
{
  spiRxSteSOF,
  spiRxSteLen,
  spiRxSteData,
  spiRxSteFcs
} spiRxSte_t;

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
int spiDevFd;
int zbSrdyFd;

static uint8_t spiRxBuf[SPI_MAX_PKT_LEN];
static uint8_t spiRxHead, spiRxTail;  // Indices into spiRxBuf.

static uint8_t spiRxPkt[SPI_MAX_PKT_LEN];
static uint8_t spiRxCnt;  // Count payload data bytes parsed from spiRxBuf or read from spiRxPkt.
static uint8_t spiRxLen;  // Total length of payload data bytes to be parsed from spiRxBuf.
static uint8_t spiRxRdy;  // Set to spiRxLen when all bytes have been parsed and the FCS verified.
static spiRxSte_t spiRxSte;

static uint8_t spiTxPkt[SPI_MAX_PKT_LEN];

// Linear scratch buffer used by spiClockData(); so it could be an automatic variable (but be
// aware of the c-call-stack depth) or dynamic from a malloc()/new().
static uint8_t spiRxTmp[SPI_MAX_PKT_LEN];

static uint8_t bits = 8;
static uint32_t speed = 1000000; 
static uint16_t delay = 100;
static uint8_t mode = 0;


static pthread_mutex_t spiMutex1 = PTHREAD_MUTEX_INITIALIZER;

/**************************************************************************************************
 * @fn      HalSpiSrdysetEdge
 *
 *
 * @brief   Initialise MRDY GPIO.
 *
 * @param   port - SPI port.
 *          edge - 1 rising, 0 falling
 *
 * @return  None
 **************************************************************************************************/
static void HalSpiSrdysetEdge(uint8_t edge)
 {
  int zbSrdyFdEdge;
  
  //open the SRDY GPIO VALUE file so it can be poll'ed to using the file handle later
	zbSrdyFdEdge = open(HAL_SRDY_EDGE, O_RDWR | O_NONBLOCK );
	if (zbSrdyFdEdge < 0) {
		perror("gpio/fd_open");
	} 
	  	
	if(edge)
	{
	  lseek(zbSrdyFdEdge, SEEK_SET, 0);
	  write(zbSrdyFdEdge, "rising", 7); 
	}
	else
	{
	  lseek(zbSrdyFdEdge, SEEK_SET, 0);
	  write(zbSrdyFdEdge, "falling", 8); 	    
	}
	
	close(zbSrdyFdEdge);
}

/*********************************************************************
 * @fn      spiSrdyInit
 *
 * @brief   opens Fd to the CC2530 - host Interupt.
 *
 * @param   devicePath - path to the UART device
 *
 * @return  status
 */
static int spiSrdyInit(void)
{       
  //printf("spiSrdyInit++\n"); 
//----------------- SPI ENABLE GPIO -------------------------
  //open the SPI Enable GPIO DIR file
  zbSrdyFd = open(HAL_SPIENABLE_DIR, O_RDWR); 
  if(zbSrdyFd == 0)                                                 
  {                                                                 
    perror(HAL_SPIENABLE_DIR);                    
    //printf("%s open failed\n",HAL_SPIENABLE_DIR); 
    exit(-1);                                                       
  }                      
  //Set SPI Enable GPIO as output                                                                  
  write(zbSrdyFd, "out", 3);   
  //close SRDY DIR file
  close(zbSrdyFd);     
	       
  //open the SPI Enable GPIO VALUE file so it can be written to
	zbSrdyFd = open(HAL_SPIENABLE_VALUE, O_RDWR | O_NONBLOCK );
	if (zbSrdyFd < 0) {
		perror("gpio/fd_open");
	} 	
  //Set SPI Enable GPIO as high                                                                  
  write(zbSrdyFd, "1", 1); 
  //close SRDY DIR file
  close(zbSrdyFd);	

//----------------- SRDY GPIO -------------------------	
  //open the SPI Enable GPIO VALUE  file
  zbSrdyFd = open(HAL_SRDY_DIR, O_RDWR); 
  if(zbSrdyFd == 0)                                                 
  {                                                                 
    perror(HAL_SRDY_DIR);                    
    //printf("%s open failed\n",HAL_SRDY_DIR); 
    exit(-1);                                                       
  }                      
  //Set SRDY GPIO as input                                                                  
  write(zbSrdyFd, "in", 2);   
  //close SRDY DIR file
  close(zbSrdyFd);     

  //set to use failing edge.
  HalSpiSrdysetEdge(0);
  	       
  //open the SRDY GPIO VALUE file so it can be poll'ed to using the file handle later
	zbSrdyFd = open(HAL_SRDY_VALUE, O_RDWR | O_NONBLOCK );
	if (zbSrdyFd < 0) {
		perror("gpio/fd_open");
	} 
	
	//TODO: Lock the SRDY GPIO.	  
	return zbSrdyFd; 
}

/**************************************************************************************************
 * @fn      spiSrdyCheck
 *
 *
 * @brief   Check SRDY Clear.
 *
 * @param   port - SPI port.
 *          state - state to check for (0 or 1).
 *
 * @return  None
 **************************************************************************************************/
static uint8_t spiSrdyCheck(uint8_t state)
{
  uint32_t rtn;
  uint8_t srdy;
  char ch;  
  
  if(zbSrdyFd != -1)
  {
    lseek(zbSrdyFd, SEEK_SET, 0);
    rtn = read(zbSrdyFd,&ch, 1);

    //srdy will be '1' or '0' in Ascii, so -30 will give it in int
    srdy=ch-0x30;
    
    //printf("spiSrdyCheck: srdy=%x\n", srdy);
    
  //SRDY is active low
  }
  else
  {
    //printf("spiSrdyCheck: zbSrdyFd == -1\n");
    srdy = state;
  }
  
  //Note the !, SRDY is active low.  
  return (!(state == srdy));
}

/**************************************************************************************************
 * @fn          spiCalcFcs
 *
 * @brief       Calculate the FCS of a SPI Transport frame and append it to the packet.
 *
 * input parameters
 *
 * @param       pBuf - Pointer to the SPI Transport frame over which to calculate the FCS.
 *
 * output parameters
 *
 * None.
 *
 * @return      The FCS calculated.
 */
static uint8_t spiCalcFcs(uint8_t *pBuf)
{
  uint8_t len = SPI_DAT_LEN(pBuf) + SPI_HDR_LEN + SPI_HDR_IDX;
  uint8_t idx = SPI_HDR_IDX;
  uint8_t fcs = 0;

  while (idx < len)
  {
    fcs ^= pBuf[idx++];
  }
  pBuf[idx] = fcs;

  return fcs;
}

/**************************************************************************************************
 * @fn          spiClockData
 *
 * @brief       Clock Rx bytes on the SPI bus.
 *
 * input parameters
 *
 * @param       pBuf - pointer to the memory of the data bytes to send; NULL to send dummy zeroes.
 * @param       len - the length of the data bytes to send.
 *
 * output parameters
 *
 * None.
 *
 * @return      The FCS calculated.
 */
static void spiClockData(uint8_t *pBuf, uint8_t len)
{
  struct spi_ioc_transfer tr = {
    .tx_buf = (unsigned long)pBuf,
    .rx_buf = (unsigned long)spiRxTmp,
    .len = len,
    .delay_usecs = delay,
    .speed_hz = speed,
    .bits_per_word = bits,
  };

  if (pBuf == NULL)  // If just requesting to send dummy zero bytes.
  {
    (void)memset(spiRxTmp, 0, len);
    tr.tx_buf = (unsigned long)spiRxTmp;
  }

  pthread_mutex_lock(&spiMutex1);
  int rtrn = ioctl(spiDevFd, SPI_IOC_MESSAGE(1), &tr);
  pthread_mutex_unlock(&spiMutex1);

  if (rtrn < 0)
  {
    //printf("spiClockData: Full duplex transfer failed\n");
  }
  else
  {    
    pBuf = spiRxTmp;

    // Now that the SPI bus Mutex has been released, copy the Rx data from this temporary linear
    // buffer into the circular buffer accessed by the background parsing algorithm.
    while (len--)
    {
      //printf("spiClockData[%x]: %x\n", spiRxTail, *pBuf); 
      spiRxBuf[spiRxTail++] = *pBuf++;                    
    }
  }
}

/**************************************************************************************************
 * @fn          spiParseRx
 *
 * @brief       Parse all available bytes from the spiRxBuf[] into the spiRxPkt[].
 *              Note: Only call if (spiRxRdy == 0).
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
 */
static void spiParseRx(void)
{ 
  //printf("spiParseRx++\n");
  
  while (1)
  {       
    if (spiRxHead == spiRxTail)
    {
      //If we have FCS/Sync errors consider giving zbSoC longer to change SRDY
      //usleep(50);

      if (spiSrdyCheck(1))
      {
        spiClockData(NULL, 1);
        continue;
      }

      break;
    }   
    
    uint8_t ch = spiRxBuf[spiRxHead];
    SPI_LEN_T_INCR(spiRxHead);
    
    switch (spiRxSte)
    {     
    case spiRxSteSOF:
      if (ch == SPI_SOF)
      {
        spiRxSte = spiRxSteLen;

        // At this point, the master has effected the protocol for ensuring that the SPI slave is
        // awake, so set the spiRxLen to non-zero to prevent the slave from re-entering sleep until
        // the entire packet is received - even if the master interrupts the sending of the packet
        // by de-asserting/re-asserting MRDY one or more times.
        spiRxLen = 1;
      }
      break;

    case spiRxSteLen:
      spiRxPkt[SPI_LEN_IDX] = ch;
      if ((ch == 0) || (ch > SPI_MAX_DAT_LEN))
      {
        spiRxSte = spiRxSteSOF;
        spiRxLen = 0;
      }
      else
      {
        spiRxLen = ch;
        spiRxCnt = 0;
        spiRxSte = spiRxSteData;
        spiClockData(NULL, (ch+1));  // Clock out the SPI Frame Data bytes and FCS.
      }
      break;

    case spiRxSteData:
      spiRxPkt[SPI_DAT_IDX + spiRxCnt++] = ch;

      if (spiRxCnt == spiRxLen)
      {
        spiRxSte = spiRxSteFcs;
      }
      break;

    case spiRxSteFcs:
      spiRxSte = spiRxSteSOF;

      if (ch == spiCalcFcs(spiRxPkt))
      {        
        spiRxRdy = spiRxLen;
        // Zero spiRxLen now to re-enable sleep within SPI_PM_DLY time. Before sleep is entered,
        // both the Application will be notified of spiRxRdy via uartCB(), and the spiRxBuf will be
        // polled again for another SOF (which would block the return to sleep).
        spiRxLen = 0;
        // Zero spiRxCnt now to re-use it for counting the data bytes incrementally read with
        // HalUARTReadSPI() for Applications that do not read all at once.
        spiRxCnt = 0;
        return;
      }
      else
      {
        printf("spiParseRx: FCS failed for len %x, Calc:%x Read:%x\n", spiRxPkt[SPI_LEN_IDX], spiCalcFcs(spiRxPkt), ch);    
      }

      spiRxCnt = 0;
      break;

    default:
      spiRxSte = spiRxSteSOF;
      break;
    }
  }
}
  
/*********************************************************************
 * API FUNCTIONS
 */


/*********************************************************************
 * @fn      zbSocTransportOpen
 *
 * @brief   opens the serial port to the CC253x.
 *
 * @param   devicePath - path to the UART device
 *
 * @return  status
 */
int32_t zbSocTransportOpen( char *devicePath  )
{
  //printf("zbSocTransportOpen++: %s\n",devicePath);
  
  /* open the device */  
  spiDevFd = open(devicePath, O_RDWR );
  if (spiDevFd <0)
  {
    perror(devicePath);
    printf("%s open failed\n",devicePath);
    exit(-1);
  }

 	/*
 	 * spi mode
 	 */
 	ioctl(spiDevFd, SPI_IOC_WR_MODE, &mode);
 	ioctl(spiDevFd, SPI_IOC_RD_MODE, &mode);

	/*
	 * bits per word
	 */
	ioctl(spiDevFd, SPI_IOC_WR_BITS_PER_WORD, &bits);;
	ioctl(spiDevFd, SPI_IOC_RD_BITS_PER_WORD, &bits);

	/*
	 * max speed hz
	 */
	ioctl(spiDevFd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	ioctl(spiDevFd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);

  return spiSrdyInit();
}

/*********************************************************************
 * @fn      zbSocTransportClose
 *
 * @brief   closes the serial port to the CC253x.
 *
 * @param   fd - file descriptor of the UART device
 *
 * @return  status
 */
void zbSocTransportClose( void )
{
  tcflush(spiDevFd, TCOFLUSH);
  close(spiDevFd);
  close(zbSrdyFd);
  
  return;
}

/*********************************************************************
 * @fn      zbSocTransportWrite
 *
 * @brief   Write to the the serial port to the CC253x.
 *
 * @param   fd - file descriptor of the UART device
 *
 * @return  status
 */
void zbSocTransportWrite( uint8_t* buf, uint8_t len )
{
  if (len > SPI_MAX_DAT_LEN)
  {
    len = SPI_MAX_DAT_LEN;
  }

  spiTxPkt[SPI_LEN_IDX] = len;
  (void)memcpy(spiTxPkt + SPI_DAT_IDX, buf, len);

  spiCalcFcs(spiTxPkt);
  spiTxPkt[SPI_SOF_IDX] = SPI_SOF;

  spiClockData(spiTxPkt, SPI_PKT_LEN(spiTxPkt));

  return;
}

/*********************************************************************
 * @fn      zbSocTransportRead
 *
 * @brief   Reads from the the serial port to the CC253x.
 *
 * @param   fd - file descriptor of the UART device
 *
 * @return  status
 */
uint8_t zbSocTransportRead( uint8_t* buf, uint8_t len )
{
  //printf("zbSocTransportRead++\n");
  if (spiRxRdy == 0)
  {
    spiParseRx();
  }
    
  if (len > spiRxRdy)
  {
    len = spiRxRdy;
  }
  spiRxRdy -= len;

  (void)memcpy(buf, spiRxPkt + SPI_DAT_IDX + spiRxCnt, len);

  spiRxCnt += len;

  return len;
}

/*********************************************************************
 * @fn      zbSocTransportPoll
 *
 * @brief   poll the SPI SRDY to see if there is data to read.
 *
 * @param   fd - file descriptor of the UART device
 *
 * @return  status
 */
uint8_t zbSocTransportPoll(void)
{
  //printf("zbSocTransportPoll++: spiRxRdy=%x\n", spiRxRdy);
  if (spiRxRdy == 0)
  {
    spiParseRx();
  }

  //printf("zbSocTransportPoll--: spiRxRdy=%x\n", spiRxRdy);
  return spiRxRdy;
}