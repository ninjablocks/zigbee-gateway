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

#include "interface_scenelist.h"

/*********************************************************************
 * TYPEDEFS
 */
 
typedef struct
{
  uint8_t sceneId;
  uint16_t groupId;
  char *sceneNameStr;
  void   *next;
}sceneRecord_t;
 
sceneRecord_t *sceneRecordHead = NULL;

/*********************************************************************
 * LOCAL FUNCTION PROTOTYPES
 */ 
static sceneRecord_t* createSceneRec( char *sceneNameStr, uint8_t sceneId, uint16_t groupId, uint8_t storeToFile );
static sceneRecord_t* findSceneRec( char *sceneNameStr, uint16_t groupId );
static uint16_t getFreeSceneId(void);
static void writeSceneListToFile( sceneRecord_t *device );
static void readSceneListFromFile( void );

/*********************************************************************
 * FUNCTIONS
 *********************************************************************/

/*********************************************************************
 * @fn      createSceneRec
 *
 * @brief   create a scene and rec to the list.
 *
 * @return  none
 */
static sceneRecord_t* createSceneRec( char *sceneNameStr, uint8_t sceneId, uint16_t groupId, uint8_t storeToFile )
{  
  //printf("createSceneRec++: sceneId %x, groupId %x\n", sceneId, groupId);
  
  //does it already exist  
  if( findSceneRec( sceneNameStr, groupId ) )
  {
    //printf("createSceneRec: Device already exists\n");
    return 0;
  }
      
  sceneRecord_t *newScene = malloc( sizeof( sceneRecord_t ) );
  
  newScene->sceneNameStr = malloc(sceneNameStr[0]+1);
  memcpy( newScene->sceneNameStr, sceneNameStr, sceneNameStr[0]+1);
  
  //set sceneId
  newScene->sceneId = sceneId;   
  //set groupId
  newScene->groupId = groupId; 
  //NULL the pointers
  newScene->next = NULL;
    
  //store the record
  if(sceneRecordHead)
  {
    sceneRecord_t *srchRec;

    //find the end of the list and add the record
    srchRec = sceneRecordHead;
    // Stop at the last record
    while ( srchRec->next )
      srchRec = srchRec->next;

    // Add to the list
    srchRec->next = newScene; 
  }
  else
    sceneRecordHead = newScene;
      
  if(storeToFile)
  {
    writeSceneListToFile(newScene);
  }
  
  //printf("createSceneRec--\n");
  return newScene;
}

/*********************************************************************
 * @fn      findSceneRec
 *
 * @brief   find a record in the list that matches scene and group.
 *
 *
 * @return  none
 */
static sceneRecord_t* findSceneRec( char *sceneNameStr, uint16_t groupId )
{
  sceneRecord_t *srchRec = sceneRecordHead;

  //printf("findSceneRec++\n");
  // find record
  while ( (srchRec != NULL) ) 
  {
      //printf("findSceneRec: srchRec:%x\n", (uint32_t) srchRec);
      //printf("findSceneRec: srchRec->sceneNameStr:%x\n", (uint32_t) srchRec->sceneNameStr);
      //printf("findSceneRec: sceneNameStr:%x\n", (uint32_t) sceneNameStr);
      if(srchRec->sceneNameStr[0] == sceneNameStr[0])
      {
        if( (strncmp(srchRec->sceneNameStr, sceneNameStr, srchRec->sceneNameStr[0]) == 0) &&
          srchRec->groupId == groupId)
        {
          //we found the scene
          //printf("findSceneRec: scene found\n");
          break;
        }
      }
      srchRec = srchRec->next;  
  }
  
  //printf("findSceneRec--\n");
   
  return srchRec;
}

/*********************************************************************
 * @fn      getFreeSceneId
 *
 * @brief   Finds the next (hieghst) free scene ID.
 *
 *
 * @return  none
 */
static uint16_t getFreeSceneId( void )
{
  sceneRecord_t *srchRec = sceneRecordHead;
  uint16_t heighestSceneIdx = 0;
  
  //printf("getFreeSceneId++\n");
  
  // find record
  while ( srchRec ) 
  {
    //printf("getFreeSceneId: srchRec->sceneId %x\n", srchRec->sceneId);
    if(heighestSceneIdx < srchRec->sceneId)
    {
       heighestSceneIdx = srchRec->sceneId;
    }
       
    srchRec = srchRec->next;  
  }
  
  heighestSceneIdx++;
  
  //printf("getFreeSceneId--: %x\n", heighestSceneIdx);
   
  return (heighestSceneIdx);
}

/***************************************************************************************************
 * @fn      writeSceneListToFile - store scene list.
 *
 * @brief   
 * @param   
 *
 * @return 
 ***************************************************************************************************/
static void writeSceneListToFile( sceneRecord_t *scene )
{
  FILE *fpSceneFile;
  
  //printf("writeSceneListToFile++\n");
  
  fpSceneFile = fopen("scenelistfile.dat", "a+b");

  if(fpSceneFile)
  {
    //printf("writeSceneListToFile: opened file\n");
    
    //printf("writeSceneListToFile: Store scene: sceneId %x, sceneNameLen %x, sceneName %s\n", scene->sceneId, (scene->sceneNameStr[0] + 1), scene->sceneNameStr);
    //Store scene
    fwrite((const void *) &(scene->groupId), 2, 1, fpSceneFile);    
    fwrite((const void *) &(scene->sceneId), 1, 1, fpSceneFile);
    fwrite((const void *) &(scene->sceneNameStr[0]), 1, 1, fpSceneFile);    
    fwrite((const void *) &(scene->sceneNameStr[1]), (scene->sceneNameStr[0]), 1, fpSceneFile);
    
    //write scene delimeter
    fwrite((const void *) ";", 1, 1, fpSceneFile);
    
    fflush(fpSceneFile);
    fclose(fpSceneFile); 
  }
}

/***************************************************************************************************
 * @fn      readSceneListFromFile - restore the scene list.
 *
 * @brief   
 *
 * @return 
 ***************************************************************************************************/
static void readSceneListFromFile( void )
{
  FILE *fpSceneFile;
  sceneRecord_t *scene;
  uint32_t fileSize, sceneStrIdx=0, bytesRead=0;
  char *fileBuf;
    
  //printf("readSceneListFromFile++\n");
  fpSceneFile = fopen("scenelistfile.dat", "a+b");

  if(fpSceneFile)
  {    
    //printf("readSceneListFromFile: file opened\n");

    //read the file into a buffer  
    fseek(fpSceneFile, 0, SEEK_END);
    fileSize = ftell(fpSceneFile);
    rewind(fpSceneFile);  
    fileBuf = (char*) calloc(sizeof(char), fileSize);  
    bytesRead = fread(fileBuf, 1, fileSize, fpSceneFile);
    
    //printf("readSceneListFromFile: read file [%d:%d]\n", fileSize, bytesRead);
    
    if(fileBuf)
    {
      //printf("readSceneListFromFile: processing filebuf. sceneStrIdx: %x, (sceneStrIdx + fileBuf[sceneStrIdx + 2] + 2) : %x \n", sceneStrIdx, (sceneStrIdx + fileBuf[sceneStrIdx + 2] + 2));
      
      //read scene if there is a full scene to read (uint16_t groupId + uint8_t sceneId + string length byte + string length (stored in byte before string) )
      while((sceneStrIdx + fileBuf[sceneStrIdx + 3] + 2 + 1 + 1) < fileSize)
      {
        uint16_t groupId = (uint16_t) fileBuf[sceneStrIdx];
        uint8_t sceneId = (uint8_t) fileBuf[sceneStrIdx + 2];        
                       
        scene = createSceneRec(&(fileBuf[(sceneStrIdx + 3)]), sceneId, groupId, 0);              
        
        //printf("readSceneListFromFile: scene ID %s read\n", fileBuf[sceneStrIdx]);
        
        //index past GroupId + SceneId + sceneNameStrLen + sceneNameStr + ';'
        sceneStrIdx += (fileBuf[sceneStrIdx + 3] + 2 + 1 + 1) + 1;
      }
      
      //printf("readSceneListFromFile: processed filebuf\n");      
      
      free(fileBuf);
    }
    
    fflush(fpSceneFile);
    fclose(fpSceneFile);    
  }
  
  //printf("readSceneListFromFile--\n");
}

/*********************************************************************
 * @fn      devListInitDatabase
 *
 * @brief   create a device list from file.
 *
 * @param   none
 *
 * @return  none
 */
void sceneListRestorScenes( void )
{
  //printf("sceneListRestorScenes++\n");
  
  if( sceneRecordHead == NULL)
  {
    readSceneListFromFile();
  }
  //else do what, should we delete the list and recreate from the file?
  
  //printf("sceneListRestorScenes--\n");
}

/*********************************************************************
 * @fn      devListDescoverDevice
 *
 * @brief   Descovers a Scene.
 *
 * @param   table
 * @param   rmTimer
 *
 * @return  none
 */
void sceneListDescoverScene( char *sceneNameStr )
{  
  //printf("sceneListDescoverDevice++\n");
  
  //Need to BCast a get scene membership
  
  //printf("sceneListDescoverDevice--\n");
}

/*********************************************************************
 * @fn      sceneListGetNextScene
 *
 * @brief   Return the next scene in the list.
 *
 * @param   sceneNameStr - if NULL it will return head of the list
 *          groupId - group that the scene is apart of, ignored if sceneStr is NULL.
 *
 * @return  sceneListItem_t, return next scene from sceneNameStr supplied or 
 *          NULL if at end of the list
 */
sceneListItem_t* sceneListGetNextScene( char *sceneNameStr, uint16_t groupId )
{  
  sceneRecord_t *srchRec = sceneRecordHead;
  sceneListItem_t *sceneItem = NULL;
  
  //printf("sceneListGetNextScene++\n");
  
  if(sceneNameStr != NULL)
  {
    //printf("sceneListGetNextScene: sceneNameStr != NULL\n");
    
    //Find the record for previous scene found
    srchRec = findSceneRec( sceneNameStr, groupId );
    //printf("sceneListGetNextScene: findSceneRec %x \n", (uint32_t) srchRec);
    //get the next record (may be NULL if at end of list)
    srchRec = srchRec->next;
    //printf("sceneListGetNextScene: findSceneRec next  %x \n", (uint32_t) srchRec);
    //Store the sceneItem
    if(srchRec)
    {
      //printf("sceneListGetNextScene: storing sceneItem, sceneId %x, groupId %x\n", srchRec->sceneId, srchRec->groupId);
      sceneItem = malloc(sizeof(sceneListItem_t));
      if(sceneItem)
      {
        sceneItem->groupId = srchRec->groupId;
        sceneItem->sceneId = srchRec->sceneId;
        sceneItem->sceneNameStr = malloc(srchRec->sceneNameStr[0] + 1);
        if(sceneItem->sceneNameStr)
        {
          strncpy(sceneItem->sceneNameStr, srchRec->sceneNameStr, (srchRec->sceneNameStr[0] + 1));
        }
      }
    }
        
  }
  else if(sceneRecordHead)
  {
    //else return the head of revord
    sceneItem = malloc(sizeof(sceneListItem_t));
    if(sceneItem)
    {      
      sceneItem->groupId = sceneRecordHead->groupId;
      sceneItem->sceneId = sceneRecordHead->sceneId;
      sceneItem->sceneNameStr = malloc(sceneRecordHead->sceneNameStr[0] + 1);
      if(sceneItem->sceneNameStr)
      {
        strncpy(sceneItem->sceneNameStr, sceneRecordHead->sceneNameStr, (sceneRecordHead->sceneNameStr[0] + 1));
      }
    }
  }
  
  //printf("sceneListGetNextScene--\n");
  
  return (sceneItem);
}

/*********************************************************************
 * @fn      sceneListAddScene
 *
 * @brief   add a scene to the scene list.
 *
 * @return  sceneId
 */
uint8_t sceneListAddScene( char *sceneNameStr, uint16_t groupId )
{
  uint8_t sceneId = 0;
  sceneRecord_t *scene;
  
  //printf("sceneListAddScene++\n");
  
  scene = findSceneRec( sceneNameStr, groupId );
  
  if( scene == NULL)
  {     
    sceneId = getFreeSceneId();
    createSceneRec( sceneNameStr, sceneId, groupId, 1);
  }
  else
  {
    sceneId = scene->sceneId;
  }
  
  //printf("sceneListAddScene--\n");
  
  return sceneId;
}

/*********************************************************************
 * @fn      sceneListAddScene
 *
 * @brief   add a scene to the scene list.
 *
 * @return  sceneId
 */
uint8_t sceneListGetSceneId( char *sceneNameStr, uint16_t groupId )
{
  uint8_t sceneId = 0;
  sceneRecord_t *scene;
  
  //printf("sceneListGetSceneId++\n");
  
  scene = findSceneRec( sceneNameStr, groupId );
  
  if( scene == NULL)
  {     
    sceneId = -1;
  }
  else
  {
    sceneId = scene->sceneId;
  }
  
  //printf("sceneListGetSceneId--\n");
  
  return sceneId;
}