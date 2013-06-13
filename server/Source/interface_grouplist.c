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

/*********************************************************************
 * TYPEDEFS
 */
typedef struct
{
  void   *next;
  uint16_t groupMemberNwkAddr;
}groupMembersRecord_t;
 
typedef struct
{
  uint16_t groupId;
  char *groupNameStr;
  groupMembersRecord_t *groupMembers;
  void   *next;
}groupRecord_t;
 
groupRecord_t *groupRecordHead = NULL;

/*********************************************************************
 * LOCAL FUNCTION PROTOTYPES
 */ 
static groupRecord_t* createGroupRec( char *groupNameStr, uint16_t groupId, uint8_t storeToFile );
static groupRecord_t* findGroupRec(char *groupNameStr );
static uint16_t getFreeGroupId(void);
static int addGroupMemberToGroup( char *groupNameStr, uint16_t nwkAddr );
static void writeGroupListToFile( groupRecord_t *device );
static void writeGroupMemberToFile( char *groupNameStr, uint16_t nwkAddr  );
static void readGroupListFromFile( void );

/*********************************************************************
 * FUNCTIONS
 *********************************************************************/

/*********************************************************************
 * @fn      createGroupRec
 *
 * @brief   create a group and rec to the list.
 *
 * @return  none
 */
static groupRecord_t* createGroupRec( char *groupNameStr, uint16_t groupId, uint8_t storeToFile )
{  
  //printf("createGroupRec++\n");
  
  //does it already exist  
  if( findGroupRec( groupNameStr ) )
  {
    //printf("createGroupRec: Device already exists\n");
    return 0;
  }
      
  groupRecord_t *newGroup = malloc( sizeof( groupRecord_t ) );
  
  newGroup->groupNameStr = malloc(groupNameStr[0]+1);
  memcpy( newGroup->groupNameStr, groupNameStr, groupNameStr[0]+1);
  
  //set groupId
  newGroup->groupId = groupId; 
  //NULL the pointers
  newGroup->groupMembers = NULL;
  newGroup->next = NULL;
    
  //store the record
  if(groupRecordHead)
  {
    groupRecord_t *srchRec;

    //find the end of the list and add the record
    srchRec = groupRecordHead;
    // Stop at the last record
    while ( srchRec->next )
      srchRec = srchRec->next;

    // Add to the list
    srchRec->next = newGroup; 
  }
  else
    groupRecordHead = newGroup;
      
  if(storeToFile)
  {
    writeGroupListToFile(newGroup);
  }
  
  //printf("createGroupRec--\n");
  return newGroup;
}

/*********************************************************************
 * @fn      addGroupMemberToGroup
 *
 * @brief   add a device to an existing groupRecordHead.
 *
 * @return  none
 */
static int addGroupMemberToGroup( char *groupNameStr, uint16_t nwkAddr )
{
  groupRecord_t* group;
  
  group = findGroupRec( groupNameStr );
  
  if(group != NULL)
  {
    groupMembersRecord_t *srchGroupMember = group->groupMembers;

    //find the end of the list and check device not already in the list
    while ( srchGroupMember )
    {
      if(srchGroupMember->groupMemberNwkAddr == nwkAddr)
      {
        //device already in group
        return 0;
      }
       
      if(srchGroupMember->next == NULL)
      {
        groupMembersRecord_t*  newGroupMembersRecord;
        //we at the end of the list store the groupMember
        newGroupMembersRecord = malloc(sizeof(groupMembersRecord_t));
        newGroupMembersRecord->groupMemberNwkAddr = nwkAddr;
        srchGroupMember->next = newGroupMembersRecord;
        
        writeGroupMemberToFile(groupNameStr, nwkAddr);
        
        return 0;
      }
      else
      {
        srchGroupMember = srchGroupMember->next;            
      }
    }    
  }
    
  return 0;
}

/*********************************************************************
 * @fn      findGroupRec
 *
 * @brief   find a record in the list.
 *
 *
 * @return  none
 */
static groupRecord_t* findGroupRec( char *groupNameStr )
{
  groupRecord_t *srchRec = groupRecordHead;

  //printf("findGroupRec++\n");
  // find record
  while ( (srchRec != NULL) ) 
  {
      //printf("findGroupRec: srchRec:%x\n", (uint32_t) srchRec);
      //printf("findGroupRec: srchRec->groupNameStr:%x\n", (uint32_t) srchRec->groupNameStr);
      //printf("findGroupRec: groupNameStr:%x\n", (uint32_t) groupNameStr);
      if(srchRec->groupNameStr[0] == groupNameStr[0])
      {
        if(strncmp(srchRec->groupNameStr, groupNameStr, srchRec->groupNameStr[0]) == 0)
        {
          //we found the group
          //printf("findGroupRec: group found\n");
          break;
        }
      }
      srchRec = srchRec->next;  
  }
  
  //printf("findGroupRec--\n");
   
  return srchRec;
}

/*********************************************************************
 * @fn      getFreeGroupId
 *
 * @brief   Finds the next (hieghst) free group ID.
 *
 *
 * @return  none
 */
static uint16_t getFreeGroupId( void )
{
  groupRecord_t *srchRec = groupRecordHead;
  uint16_t heighestGroupIdx = 0;
  
  //printf("findGroupRec++\n");
  
  // find record
  while ( srchRec ) 
  {
    if(heighestGroupIdx < srchRec->groupId)
    {
       heighestGroupIdx = srchRec->groupId;
    }
       
    srchRec = srchRec->next;  
  }
  
  //printf("findGroupRec--\n");
   
  return heighestGroupIdx + 1;
}

/***************************************************************************************************
 * @fn      writeGroupListToFile - store group list.
 *
 * @brief   
 * @param   
 *
 * @return 
 ***************************************************************************************************/
static void writeGroupListToFile( groupRecord_t *group )
{
  FILE *fpGRoupFile;
  
  //printf("writeGroupListToFile++\n");
  
  fpGRoupFile = fopen("grouplistfile.dat", "a+b");

  if(fpGRoupFile)
  {
    //printf("writeGroupListToFile: opened file\n");
    
    //printf("writeGroupListToFile: Store group: groupId %x, groupNameLen %x, groupName %s\n", group->groupId, (group->groupNameStr[0] + 1), group->groupNameStr);
    //Store group
    fwrite((const void *) &(group->groupId), 2, 1, fpGRoupFile);
    fwrite((const void *) &(group->groupNameStr[0]), 1, 1, fpGRoupFile);    
    fwrite((const void *) &(group->groupNameStr[1]), (group->groupNameStr[0]), 1, fpGRoupFile);
    
    //write group delimeter
    fwrite((const void *) ";", 1, 1, fpGRoupFile);
    
    fflush(fpGRoupFile);
    fclose(fpGRoupFile); 
  }
}

/***************************************************************************************************
 * @fn      writeGroupMemberToFile - store group list.
 *
 * @brief   
 * @param   
 *
 * @return 
 ***************************************************************************************************/
static void writeGroupMemberToFile( char *groupNameStr, uint16_t nwkAddr  )
{
  FILE *fpGroupMemberFile;
  uint32_t fileSize, groupStrIdx=0;
  char *fileBuf, *groupStr, *groupStrEnd;
  
  //printf("writeGroupMemberToFile++\n");
  
  fpGroupMemberFile = fopen("grouplistfile.dat", "a+b");
  
  if(fpGroupMemberFile)
  {
    //read the file into a buffer  
    fseek(fpGroupMemberFile, 0, SEEK_END);
    fileSize = ftell(fpGroupMemberFile);
    rewind(fpGroupMemberFile);  
    fileBuf = (char*) calloc(sizeof(char), fileSize + 20);  
    fread(fileBuf, 1, fileSize, fpGroupMemberFile);

    //find the group
    groupStr = strstr(groupNameStr, fileBuf);
    //find group delimiter
    groupStrEnd = strchr(groupStr, ';');
    //get byte offset in fileSize
    groupStrIdx += (groupStr - groupStrEnd);
    //back up to before the ;    
    groupStrIdx =- 1;
        
    //set the file pointer to the 
    fseek(fpGroupMemberFile, SEEK_SET, groupStrIdx);

    //printf("writeGroupMemberToFile: Store group member\n");
    //write member delimeter
    fwrite((const void *) ":", 1, 1, fpGroupMemberFile);
    //write the member nwk addr
    fwrite((const void *) &(nwkAddr), sizeof(uint16_t), 1, fpGroupMemberFile);
  }
    
  fflush(fpGroupMemberFile);
  fclose(fpGroupMemberFile);
  free(fileBuf);   
} 

/***************************************************************************************************
 * @fn      readGroupListFromFile - restore the group list.
 *
 * @brief   
 *
 * @return 
 ***************************************************************************************************/
static void readGroupListFromFile( void )
{
  FILE *fpGRoupFile;
  groupRecord_t *group;
  uint32_t fileSize, groupStrIdx=0, bytesRead=0;
  char *fileBuf;
    
  //printf("readGroupListFromFile++\n");
  fpGRoupFile = fopen("grouplistfile.dat", "a+b");

  if(fpGRoupFile)
  {    
    //printf("readGroupListFromFile: file opened\n");

    //read the file into a buffer  
    fseek(fpGRoupFile, 0, SEEK_END);
    fileSize = ftell(fpGRoupFile);
    rewind(fpGRoupFile);  
    fileBuf = (char*) calloc(sizeof(char), fileSize);  
    bytesRead = fread(fileBuf, 1, fileSize, fpGRoupFile);
    
    //printf("readGroupListFromFile: read file [%d:%d]\n", fileSize, bytesRead);
    
    if(fileBuf)
    {
      //printf("readGroupListFromFile: processing filebuf. groupStrIdx: %x, (groupStrIdx + fileBuf[groupStrIdx + 2] + 2) : %x \n", groupStrIdx, (groupStrIdx + fileBuf[groupStrIdx + 2] + 2));
      
      //read group if there is a full group to read (uint16_t groupId + string length byte + string length (stored in byte before string) )
      while((groupStrIdx + 2 + fileBuf[groupStrIdx + 2] + 1) < fileSize)
      {            
        //printf("readGroupListFromFile: group read for group ID %x, str len %d\n", (uint16_t) fileBuf[groupStrIdx], fileBuf[(groupStrIdx + 2)]);
        
        group = createGroupRec(&(fileBuf[(groupStrIdx + 2)]), (uint16_t) fileBuf[groupStrIdx], 0);              
        
        //printf("readGroupListFromFile: group ID %x read\n", fileBuf[groupStrIdx]);
        
        //index past GroupId + groupNameStr + groupNameStrLen + ';'
        groupStrIdx += 2 + fileBuf[groupStrIdx + 2] + 1 + 1;
      }
      
      //printf("readGroupListFromFile: processed filebuf\n");      
      
      free(fileBuf);
    }
    
    fflush(fpGRoupFile);
    fclose(fpGRoupFile);    
  }
  
  //printf("readGroupListFromFile--\n");
}

/*********************************************************************
 * @fn      devListRestorDevices
 *
 * @brief   create a device list from file.
 *
 * @param   none
 *
 * @return  none
 */
void groupListRestorGroups( void )
{
  //printf("groupListRestorGroups++\n");
  
  if( groupRecordHead == NULL)
  {
    readGroupListFromFile();
  }
  //else do what, should we delete the list and recreate from the file?
  
  //printf("groupListRestorGroups--\n");
}

/*********************************************************************
 * @fn      devListDescoverDevice
 *
 * @brief   Descovers a GRoup.
 *
 * @param   table
 * @param   rmTimer
 *
 * @return  none
 */
void groupListDescoverGroup( char *groupNameStr )
{  
  //printf("groupListDescoverDevice++\n");
  
  //Need to BCast a get group membership
  
  //printf("groupListDescoverDevice--\n");
}

/*********************************************************************
 * @fn      groupListGetNextGroup
 *
 * @brief   Return the next group in the list.
 *
 * @param   groupNameStr - if NULL it will return head of the list
 *
 * @return  groupListItem_t, return next group from groupNameStr supplied or 
 *          NULL if at end of the list
 */
groupListItem_t* groupListGetNextGroup( char *groupNameStr )
{  
  groupRecord_t *srchRec = groupRecordHead;
  groupListItem_t *groupItem = NULL;
  
  //printf("groupListGetNextGroup++\n");
  
  if(groupNameStr != NULL)
  {
    //printf("groupListGetNextGroup: groupNameStr != NULL\n");
    
    //Find the record for group
    srchRec = findGroupRec( groupNameStr );
    //printf("groupListGetNextGroup: findGroupRec %x \n", (uint32_t) srchRec);
    //get the next record (may be NULL if at end of list)
    srchRec = srchRec->next;
    //printf("groupListGetNextGroup: findGroupRec next  %x \n", (uint32_t) srchRec);
    //Store the groupItem
    if(srchRec)
    {
      //printf("groupListGetNextGroup: storing groupItem\n");
      groupItem = malloc(sizeof(groupListItem_t));
      if(groupItem)
      {
        groupItem->groupId = srchRec->groupId;
        groupItem->groupNameStr = malloc(srchRec->groupNameStr[0] + 1);
        if(groupItem->groupNameStr)
        {
          strncpy(groupItem->groupNameStr, srchRec->groupNameStr, (srchRec->groupNameStr[0] + 1));
        }
      }
    }
        
  }
  else if(groupRecordHead)
  {
    //else return the head of revord
    groupItem = malloc(sizeof(groupListItem_t));
    if(groupItem)
    {
      groupItem->groupId = groupRecordHead->groupId;
      groupItem->groupNameStr = malloc(groupRecordHead->groupNameStr[0] + 1);
      if(groupItem->groupNameStr)
      {
        strncpy(groupItem->groupNameStr, groupRecordHead->groupNameStr, (groupRecordHead->groupNameStr[0] + 1));
      }
    }
  }
  
  //printf("groupListGetNextGroup--\n");
  
  return (groupItem);
}

/*********************************************************************
 * @fn      groupListAddGroup
 *
 * @brief   add a device to the group list.
 *
 * @return  none
 */
uint16_t groupListAddGroup( char *groupNameStr )
{
  groupRecord_t *group;
  uint16_t groupId;
  
  //printf("groupListAddGroup++\n");
  
  group = findGroupRec( groupNameStr );
  
  if( group == NULL)
  {     
    groupId = getFreeGroupId();
    createGroupRec( groupNameStr, groupId, 1);
  }
  else
  {
    groupId = group->groupId;
  }
  
  //printf("groupListAddGroup--\n");
  
  return groupId;
}

/*********************************************************************
 * @fn      groupListAddDeviceToGroup
 *
 * @brief   create a group and add a rec to the list.
 *
 * @param   table
 * @param   rmTimer
 *
 * @return  none
 */
void groupListAddDeviceToGroup( char *groupNameStr, uint16_t nwkAddr )
{  
  groupRecord_t* group;
  
  //printf("groupListAddDeviceToGroup++\n");
  
  group = findGroupRec( groupNameStr );
  
  if( group == NULL)
  {     
    createGroupRec( groupNameStr, getFreeGroupId(), 1 );
  }
  
  if(group)
  {
    addGroupMemberToGroup(groupNameStr, nwkAddr);
  }
}