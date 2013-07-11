/**************************************************************************************************
 * Filename:       interface_devicelist.c
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

#include "interface_srpcserver.h"
#include "interface_devicelist.h"

#include "hal_types.h"
#include "SimpleDBTxt.h"

/*********************************************************************
 * LOCAL VARIABLES
 */

static db_descriptor * db;


/*********************************************************************
 * TYPEDEFS
 */
 
/*********************************************************************
 * LOCAL FUNCTION PROTOTYPES
 */ 


typedef struct
{
	uint16 nwkAddr;
	uint8 endpoint;
} dev_key_NA_EP;
  
typedef struct
  {
	uint8 ieeeAddr[8];
	uint8 endpoint;
} dev_key_IEEE_EP;
      
typedef uint8 dev_key_IEEE[8];
  
static char * devListComposeRecord(epInfo_t *epInfo, char * record)
{
	sprintf(record, "        %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X , 0x%04X , 0x%02X , 0x%04X , 0x%04X , 0x%02X , 0x%02X , \"%s\"\n", //leave a space at the beginning to mark this record as deleted if needed later, or as bad format (can happen if edited manually). Another space to write the reason of bad format. 
		epInfo->IEEEAddr[7],
		epInfo->IEEEAddr[6],
		epInfo->IEEEAddr[5],
		epInfo->IEEEAddr[4],
		epInfo->IEEEAddr[3],
		epInfo->IEEEAddr[2],
		epInfo->IEEEAddr[1],
		epInfo->IEEEAddr[0],
		epInfo->nwkAddr,
		epInfo->endpoint,
		epInfo->profileID,
		epInfo->deviceID,
		epInfo->version,
		epInfo->status,
		epInfo->deviceName ? epInfo->deviceName : "");
  
	return record;
}
  
void devListAddDevice( epInfo_t *epInfo)
  {
	char rec[MAX_SUPPORTED_RECORD_SIZE];

	devListComposeRecord(epInfo, rec);
    
	sdb_add_record(db, rec);
}

static epInfo_t * devListParseRecord(char * record)
{
	char * pBuf = record + 1; //+1 is to ignore the 'for deletion' mark that may just be added to this record.
	static epInfo_t epInfo;
	static char deviceName[MAX_SUPPORTED_DEVICE_NAME_LENGTH + 1];
	parsingResult_t parsingResult = {SDB_TXT_PARSER_RESULT_OK, 0};
  
	if (record == NULL)
  {
		return NULL;
  }
  
	sdb_txt_parser_get_hex_field(&pBuf, epInfo.IEEEAddr, 8, &parsingResult);
	sdb_txt_parser_get_numeric_field(&pBuf, (uint8_t *)&epInfo.nwkAddr, 2, FALSE, &parsingResult);
	sdb_txt_parser_get_numeric_field(&pBuf, &epInfo.endpoint, 1, FALSE, &parsingResult);
	sdb_txt_parser_get_numeric_field(&pBuf, (uint8_t *)&epInfo.profileID, 2, FALSE, &parsingResult);
	sdb_txt_parser_get_numeric_field(&pBuf, (uint8_t *)&epInfo.deviceID, 2, FALSE, &parsingResult);
	sdb_txt_parser_get_numeric_field(&pBuf, &epInfo.version, 1, FALSE, &parsingResult);
	sdb_txt_parser_get_numeric_field(&pBuf, &epInfo.status, 1, FALSE, &parsingResult);
	sdb_txt_parser_get_quoted_string(&pBuf, deviceName, MAX_SUPPORTED_DEVICE_NAME_LENGTH, &parsingResult);
   
	if ((parsingResult.code != SDB_TXT_PARSER_RESULT_OK) && (parsingResult.code != SDB_TXT_PARSER_RESULT_REACHED_END_OF_RECORD))
	{
		sdbtMarkError( db, record, &parsingResult);
		return NULL;
}

	if (strlen(deviceName) > 0)
	  {
		epInfo.deviceName = deviceName;
		
		}
		else
		{
		epInfo.deviceName = NULL;
		}	 
		 
	return &epInfo;
		}
		
		
	  
static int devListCheckKeyIeeeEp(char * record, dev_key_IEEE_EP * key)
	  {
	epInfo_t * epInfo;
	int result = SDB_CHECK_KEY_NOT_EQUAL;
		
	epInfo = devListParseRecord(record);
	if (epInfo == NULL)
		{
		return SDB_CHECK_KEY_ERROR;
		  } 	 
		  
	if ((memcmp(epInfo->IEEEAddr, key->ieeeAddr, Z_EXTADDR_LEN) == 0) && (epInfo->endpoint == key->endpoint))
		  {
		result = SDB_CHECK_KEY_EQUAL;
	  }
		
	return result;
	}

static int devListCheckKeyIeee(char * record, uint8_t key[Z_EXTADDR_LEN])
    {    
	epInfo_t * epInfo;
	int result = SDB_CHECK_KEY_NOT_EQUAL;
      
	epInfo = devListParseRecord(record);
	if (epInfo == NULL)
      {
		return SDB_CHECK_KEY_ERROR;
    }
//printf("Comparing 0x%08X%08X to 0x%08X%08X\n", *(uint32_t *)((epInfo->IEEEAddr)+4), *(uint32_t *)((epInfo->IEEEAddr)+0), *(uint32_t *)(key+4), *(uint32_t *)(key+0));
	if (memcmp(epInfo->IEEEAddr, key, Z_EXTADDR_LEN) == 0)
    {
		result = SDB_CHECK_KEY_EQUAL;
    }
    
	return result;
}    

  

static int devListCheckKeyNaEp(char * record, dev_key_NA_EP * key)
  {
	epInfo_t * epInfo;
	int result = SDB_CHECK_KEY_NOT_EQUAL;
      
	epInfo = devListParseRecord(record);
	if (epInfo == NULL)
      {
		return SDB_CHECK_KEY_ERROR;
	}
        
	if ((epInfo->nwkAddr == key->nwkAddr) && (epInfo->endpoint == key->endpoint))
        {
		result = SDB_CHECK_KEY_EQUAL;
          }

	return result;
        }

epInfo_t * devListRemoveDeviceByNaEp( uint16 nwkAddr, uint8 endpoint )
        {
	dev_key_NA_EP key = {nwkAddr, endpoint};
    
	return devListParseRecord(sdb_delete_record(db, &key , (check_key_f)devListCheckKeyNaEp));
  
}

epInfo_t * devListRemoveDeviceByIeee( uint8_t ieeeAddr[8] )
  {
	return devListParseRecord(sdb_delete_record(db, ieeeAddr , (check_key_f)devListCheckKeyIeee));
  }
  
epInfo_t * devListGetDeviceByIeeeEp( uint8_t ieeeAddr[8], uint8_t endpoint )
{
	char * rec;
	dev_key_IEEE_EP key;
  
	memcpy(key.ieeeAddr, ieeeAddr, 8);
	key.endpoint = endpoint;
	rec = SDB_GET_UNIQUE_RECORD(db, &key, (check_key_f)devListCheckKeyIeeeEp);
	if (rec == NULL)
  {
		return NULL;
  }
     
	return devListParseRecord(rec);
}

epInfo_t * devListGetDeviceByNaEp( uint16_t nwkAddr, uint8_t endpoint )
{
	char * rec;
	dev_key_NA_EP key;

	key.nwkAddr = nwkAddr;
	key.endpoint = endpoint;
	rec = SDB_GET_UNIQUE_RECORD(db, &key, (check_key_f)devListCheckKeyNaEp);
	if (rec == NULL)
  {
		return NULL;
    }
    
	return devListParseRecord(rec);
}

uint32_t devListNumDevices(void)
    {
	return sdbtGetRecordCount(db);
    }
    
        
epInfo_t * devListGetNextDev(uint32_t *context)
  {
	char * rec;
	epInfo_t *epInfo;

	do
{  
		rec = SDB_GET_NEXT_RECORD(db,context);
  
		if (rec == NULL)
  {
			return NULL;
  }
    
		epInfo = devListParseRecord(rec);
	} while (epInfo == NULL); //in case of a bad-format record - skip it and read the next one
  
	return epInfo;
}

  
void devListInitDatabase( char * dbFilename )
    {
	db = sdb_init_db(dbFilename, sdbtGetRecordSize, sdbtCheckDeleted, sdbtCheckIgnored, sdbtMarkDeleted, (consolidation_processing_f)sdbtErrorComment, SDB_TYPE_TEXT, 0);
	sdb_consolidate_db(&db);
  }
        
  
