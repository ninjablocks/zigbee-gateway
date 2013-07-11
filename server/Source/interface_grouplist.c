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

#include "interface_grouplist.h"
#include "hal_types.h"
#include "SimpleDBTxt.h"

 
static db_descriptor * db;

/*********************************************************************
 * TYPEDEFS
 */
  
void groupListInitDatabase( char * dbFilename )
  {
 db = sdb_init_db(dbFilename, sdbtGetRecordSize, sdbtCheckDeleted, sdbtCheckIgnored, sdbtMarkDeleted, (consolidation_processing_f)sdbtErrorComment, SDB_TYPE_TEXT, 0);
 sdb_consolidate_db(&db);
  }
      
  
static char * groupListComposeRecord(groupRecord_t *group, char * record)
  {
	groupMembersRecord_t *groupMembers;

	sprintf(record, "        0x%04X , \"%s\"", //leave a space at the beginning to mark this record as deleted if needed later, or as bad format (can happen if edited manually). Another space to write the reason of bad format. 
		group->id,
		group->name ? group->name : "");

	groupMembers = group->members;
      
	while (groupMembers != NULL)
  {
		sprintf(record + strlen(record), " , 0x%04X , 0x%02X" , groupMembers->nwkAddr, groupMembers->endpoint);
		groupMembers = groupMembers->next;
  }
  
	return record;
}

#define MAX_SUPPORTED_GROUP_NAME_LENGTH 32
#define MAX_SUPPORTED_GROUP_MEMBERS 20

static groupRecord_t * groupListParseRecord(char * record)
{
	char * pBuf = record + 1; //+1 is to ignore the 'for deletion' mark that may just be added to this record.
	static groupRecord_t group;
	static char groupName[MAX_SUPPORTED_GROUP_NAME_LENGTH + 1];
	static groupMembersRecord_t member[MAX_SUPPORTED_GROUP_MEMBERS];
	groupMembersRecord_t ** nextMemberPtr;
	parsingResult_t parsingResult = {SDB_TXT_PARSER_RESULT_OK, 0};
	int i;
  
	if (record == NULL)
	{
		return NULL;
	}
  
	sdb_txt_parser_get_numeric_field(&pBuf, (uint8_t *)&group.id, 2, FALSE, &parsingResult);
	sdb_txt_parser_get_quoted_string(&pBuf, groupName, MAX_SUPPORTED_GROUP_NAME_LENGTH, &parsingResult);
	nextMemberPtr = &group.members;
	for (i = 0; (parsingResult.code == SDB_TXT_PARSER_RESULT_OK) && (i < MAX_SUPPORTED_GROUP_MEMBERS); i++)
  {
		*nextMemberPtr = &(member[i]);
		sdb_txt_parser_get_numeric_field(&pBuf, (uint8_t *)&(member[i].nwkAddr), 2, FALSE, &parsingResult);
		sdb_txt_parser_get_numeric_field(&pBuf, (uint8_t *)&(member[i].endpoint), 1, FALSE, &parsingResult);
		nextMemberPtr = &(member[i].next);
	}
	*nextMemberPtr = NULL;

	if ((parsingResult.code != SDB_TXT_PARSER_RESULT_OK) && (parsingResult.code != SDB_TXT_PARSER_RESULT_REACHED_END_OF_RECORD))
      {
		sdbtMarkError( db, record, &parsingResult);
		return NULL;
      }
       
	if (strlen(groupName) > 0)
      {
		group.name = groupName;
      }
      else
      {
		group.name = NULL;
    }    

	return &group;
  }
    

static int groupListCheckKeyName(char * record, char * key)
{
	groupRecord_t * group;
	int result = SDB_CHECK_KEY_NOT_EQUAL;

	group = groupListParseRecord(record);
	if (group == NULL)
  {
		return SDB_CHECK_KEY_ERROR;
	}

	if (strcmp(group->name, key) == 0)
        {
		result = SDB_CHECK_KEY_EQUAL;
      }
   
	return result;
}

static int groupListCheckKeyId(char * record, uint16_t * key)
{
	groupRecord_t * group;
	int result = SDB_CHECK_KEY_NOT_EQUAL;
  
	group = groupListParseRecord(record);
	if (group == NULL)
  {
		return SDB_CHECK_KEY_ERROR;
    }
       
	if (group->id == *key)
	{
		result = SDB_CHECK_KEY_EQUAL;
  }
  
	return result;
}

groupRecord_t * groupListGetGroupByName( char * groupName )
{
	char * rec;

	rec = SDB_GET_UNIQUE_RECORD(db, groupName, (check_key_f)groupListCheckKeyName);
	if (rec == NULL)
  {
		return NULL;
	}
    
	return groupListParseRecord(rec);
}
    

uint16_t groupListGetUnusedGroupId(void)
{
	static uint16_t lastUsedGroupId = 0;
  
	lastUsedGroupId++;
  
	while (SDB_GET_UNIQUE_RECORD(db, &lastUsedGroupId, (check_key_f)groupListCheckKeyId) != NULL)
  {
		lastUsedGroupId++;
  }
    
	return lastUsedGroupId;
} 

uint16_t groupListAddGroup( char *groupNameStr )
  {    
  groupRecord_t *exsistingGroup;
  groupRecord_t newGroup;
  char rec[MAX_SUPPORTED_RECORD_SIZE];

  //printf("groupListAddGroup++\n");
    
  exsistingGroup = groupListGetGroupByName( groupNameStr );
    
  if( exsistingGroup != NULL)
    {
	return exsistingGroup->id;
  }
      
  newGroup.id = groupListGetUnusedGroupId();
  newGroup.name = groupNameStr;
  newGroup.members = NULL;
        
  groupListComposeRecord(&newGroup, rec);
        
  sdb_add_record(db, rec);
        
  //printf("groupListAddGroup--\n");
      
  return newGroup.id;
    }
    
groupRecord_t * groupListRemoveGroupByName( char * groupName )
{
	return groupListParseRecord(sdb_delete_record(db, groupName , (check_key_f)groupListCheckKeyName));
  }
  

uint16_t groupListAddDeviceToGroup( char *groupNameStr, uint16_t nwkAddr, uint8_t endpoint )
{
  groupRecord_t *group;
  groupRecord_t newGroup;
  char rec[MAX_SUPPORTED_RECORD_SIZE];
  groupMembersRecord_t ** nextMemberPtr;
  groupMembersRecord_t newMember;
  bool memberExists = FALSE;
  uint16_t groupId;
  
  //printf("groupListAddGroup++\n");
  
  group = groupListGetGroupByName( groupNameStr );

  if( group == NULL)
{  
	group = &newGroup;
	group->id = groupListGetUnusedGroupId();
	group->name = groupNameStr;
	group->members = NULL;
}

  groupId = group->id;
  
  nextMemberPtr = &(group->members);
  while ((*nextMemberPtr != NULL) && (!memberExists))
  {
  	if (((*nextMemberPtr)->nwkAddr == nwkAddr) && ((*nextMemberPtr)->endpoint == endpoint))
    {
		memberExists = TRUE;
  	}
	else
      {
  		nextMemberPtr = &((*nextMemberPtr)->next);
      }
    }
        
  if (!memberExists)
      {
	  *nextMemberPtr = &newMember;
	  newMember.nwkAddr = nwkAddr;
	  newMember.endpoint = endpoint;
	  newMember.next = NULL;
	  groupListComposeRecord(group, rec);
	  groupListRemoveGroupByName( groupNameStr );
	  sdb_add_record(db, rec);
  }
  
  //printf("groupListAddGroup--\n");
  
  return groupId;
}

groupRecord_t * groupListGetNextGroup(uint32_t *context)
{
	char * rec;
  groupRecord_t *group;
  
	do
	{
		rec = SDB_GET_NEXT_RECORD(db,context);
  
		if (rec == NULL)
  {
			return NULL;
  }
  
		group = groupListParseRecord(rec);
	} while (group == NULL); //in case of a bad-format record - skip it and read the next one
  
	return group;
}

  
