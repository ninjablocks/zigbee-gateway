/**************************************************************************************************
  Filename:       ZigBeeSrpcClient.java
  Revised:        $$
  Revision:       $$

  Description:    ZigBee SRPC Client Class

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

package com.lightingcontroller.Zigbee;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.math.BigInteger;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.SocketAddress;
import java.net.UnknownHostException;
import java.nio.charset.Charset;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Timer;
import java.util.TimerTask;
import java.util.concurrent.TimeUnit;

import com.lightingcontroller.Zigbee.ZigbeeScene;

import android.app.Activity;
import android.app.ProgressDialog;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;

import android.graphics.Color;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.Parcelable;
import android.os.SystemClock;
import android.util.Log;
import android.widget.Toast;

public class ZigbeeSrpcClient {
	
	static Thread thread;
	static Socket kkSocket;
	static OutputStream outStream;
	static InputStream inStream;
	
	private static String gatewayIp= "192.168.1.111";
	static public String gatewayPort= "11235"; //"2be3";

	//SRPC header bit positions
	private static final int SRPC_CMD_ID_POS = 0;
	private static final int SRPC_CMD_LEN_POS = 1;
	
	//SRPC CMD ID's	
	//define the outgoing RPSC command ID's
	private static final byte SRPC_NEW_DEVICE     = (byte) 0x0001;
	private static final byte SRPC_DEV_ANNCE		      = (byte) 0x0002;
	private static final byte SRPC_SIMPLE_DESC	      = (byte) 0x0003;
	private static final byte SRPC_TEMP_READING       = (byte) 0x0004;
	private static final byte SRPC_POWER_READING      = (byte) 0x0005;
	private static final byte SRPC_PING               = (byte) 0x0006;
	private static final byte SRPC_GET_DEV_STATE_RSP  = (byte) 0x0007;
	private static final byte SRPC_GET_DEV_LEVEL_RSP  = (byte) 0x0008;
	private static final byte SRPC_GET_DEV_HUE_RSP    = (byte) 0x0009;
	private static final byte SRPC_GET_DEV_SAT_RSP    = (byte) 0x000a;
	private static final byte SRPC_ADD_GROUP_RSP      = (byte) 0x000b;
	private static final byte SRPC_GET_GROUP_RSP      = (byte) 0x000c;
	private static final byte SRPC_ADD_SCENE_RSP      = (byte) 0x000d;
	private static final byte SRPC_GET_SCENE_RSP      = (byte) 0x000e;


	//define incoming RPCS command ID's
	private static final byte SRPC_CLOSE              = (byte) 0x80;
	private static final byte SRPC_GET_DEVICES        = (byte) 0x81;
	private static final byte SRPC_SET_DEV_STATE      = (byte) 0x82;	
	private static final byte SRPC_SET_DEV_LEVEL      = (byte) 0x83;	
	private static final byte SRPC_SET_DEV_COLOR      = (byte) 0x84;
	private static final byte SRPC_GET_DEV_STATE      = (byte) 0x85;	
	private static final byte SRPC_GET_DEV_LEVEL      = (byte) 0x86;	
	private static final byte SRPC_GET_DEV_HUE        = (byte) 0x87;
	private static final byte SRPC_GET_DEV_SAT        = (byte) 0x88;
	private static final byte SRPC_BIND_DEVICES       = (byte) 0x89;
	private static final byte SRPC_GET_THERM_READING  = (byte) 0x8a;
	private static final byte SRPC_GET_POWER_READING  = (byte) 0x8b;
	private static final byte SRPC_DISCOVER_DEVICES   = (byte) 0x8c;
	private static final byte SRPC_SEND_ZCL           = (byte) 0x8d;
	private static final byte SRPC_GET_GROUPS         = (byte) 0x8e;	
	private static final byte SRPC_ADD_GROUP          = (byte) 0x8f;	
	private static final byte SRPC_GET_SCENES         = (byte) 0x90;	
	private static final byte SRPC_STORE_SCENE        = (byte) 0x91;	
	private static final byte SRPC_RECALL_SCENE       = (byte) 0x92;	
	private static final byte SRPC_IDENTIFY_DEVICE    = (byte) 0x93;	
	private static final byte SRPC_CHANGE_DEVICE_NAME = (byte) 0x94;   

	//SRPC AfAddr Addr modes ID's	
	public static final byte AddrNotPresent = 0;
	public static final byte AddrGroup = 1;
	public static final byte Addr16Bit = 2;
	public static final byte Addr64Bit = 3;
	public static final byte AddrBroadcast = 1;
			  
	public static final String PREFS_NAME = "MyPrefsFile";
	private static Timer ResponseTimer;
	private static Handler ResponseTimerHandler;
	
	static byte[] srpcResponse;		
	
	private static int rpcsProcessIncoming(byte[] msg, int msgPtr)
	{	
		int msgLen;		
		
		switch (msg[msgPtr + SRPC_CMD_ID_POS])
		{

		case SRPC_NEW_DEVICE:
		{
			int profileId=0, deviceId=0, nwkAddr=0; 
			char endPoint;
			String deviceName = "";
			byte[] ieee = new byte[8];

			msgLen = msg[msgPtr + SRPC_CMD_LEN_POS] + 2;
			//index passed len and cmd ID
			msgPtr+=2;
			
			//Get the NwkAddr
            for (int i=0; i < 2; i++, msgPtr++) 
            {   
            	//java does not support unsigned so use a bigger container to avoid conversion issues
            	int nwkAddrTemp = (msg[msgPtr] & 0xff);            	 
            	nwkAddr += (nwkAddrTemp << (8 * i));             	
            }			
            
            //Get the EndPoint
            endPoint = (char) msg[msgPtr++];
            
            //Get the ProfileId
            for (int i=0; i < 2; i++, msgPtr++) 
            {   
            	//java does not support unsigned so use a bigger container to avoid conversion issues
            	int profileIdTemp = (msg[msgPtr] & 0xff);            	 
            	profileId += (profileIdTemp << (8 * i));             	
            }	
            
            //Get the DeviceId
            for (int i=0; i < 2; i++, msgPtr++) 
            {   
            	//java does not support unsigned so use a bigger container to avoid conversion issues
            	int deviceIdTemp = (msg[msgPtr] & 0xff);            	 
            	deviceId += (deviceIdTemp << (8 * i));             	
            }	            
			
            //index passed version
			msgPtr++;
			
			//index passed device name
			int nameSize = msg[msgPtr++];
			for(int i = 0; i < nameSize; i++)
			{
				deviceName += (char) msg[msgPtr++];
			}			
			msgPtr += nameSize;
			
            //index passed status
			msgPtr++;
			
			//copy IEEE Addr
			for(int i = 0; i < 8; i++)
			{
				ieee[i] = msg[msgPtr++];
			}			
			
            ZigbeeAssistant.newDevice(profileId, deviceId, nwkAddr, endPoint, ieee, deviceName);	               
            	                        
            break;
		}
		case SRPC_ADD_GROUP_RSP:
		{
			short groupId = 0;
			msgLen = msg[msgPtr + SRPC_CMD_LEN_POS] + 2;
			  
			//index passed len and cmd ID
			msgPtr+=2;
			
			//Get the GroupId
            for (int i=0; i < 2; i++, msgPtr++) 
            {   
            	//java does not support unsigned so use a bigger container to avoid conversion issues
            	int groupIdTemp = (msg[msgPtr] & 0xff);            	 
            	groupId += (groupIdTemp << (8 * i));             	
            }			
			
            String groupNameStr = new String(msg, msgPtr+1, msg[msgPtr], Charset.defaultCharset());
			
            List<ZigbeeGroup> groupList = ZigbeeAssistant.getGroups();
            //find the group
            for(int i=0; i < groupList.size(); i++)
            {
            	if(groupNameStr.equals(groupList.get(i).getGroupName()))
            	{
            		groupList.get(i).setGroupId(groupId);
            		groupList.get(i).setStatus(ZigbeeGroup.groupStatusActive);
            		break;
            	}
            }
			break;
		}
						
		case SRPC_GET_GROUP_RSP:
		{
			short groupId = 0;
			msgLen = msg[msgPtr + SRPC_CMD_LEN_POS] + 2;
			  
			//index passed len and cmd ID
			msgPtr+=2;
			
			//Get the groupId
            for (int i=0; i < 2; i++, msgPtr++) 
            {   
            	//java does not support unsigned so use a bigger container to avoid conversion issues
            	int groupIdTemp = (msg[msgPtr] & 0xff);            	 
            	groupId += (groupIdTemp << (8 * i));             	
            }			
			
            String groupNameStr = new String(msg, msgPtr+1, msg[msgPtr], Charset.defaultCharset());
			
            ZigbeeAssistant.newGroup(groupNameStr, groupId, ZigbeeGroup.groupStatusActive);
            
			break;
		}	
		
		case SRPC_ADD_SCENE_RSP:
		{
			short groupId = 0;
			byte sceneId = 0;
			msgLen = msg[msgPtr + SRPC_CMD_LEN_POS] + 2;
			  
			//index passed len and cmd ID
			msgPtr+=2;
			
			//Get the GroupId
            for (int i=0; i < 2; i++, msgPtr++) 
            {   
            	//java does not support unsigned so use a bigger container to avoid conversion issues
            	int groupIdTemp = (msg[msgPtr] & 0xff);            	 
            	groupId += (groupIdTemp << (8 * i));             	
            }		
            
            //Get the sceneId
            sceneId = (byte) msg[msgPtr++];
			
            String sceneNameStr = new String(msg, msgPtr+1, msg[msgPtr], Charset.defaultCharset());
			
            List<ZigbeeScene> sceneList = ZigbeeAssistant.getScenes();
            //find the scene
            for(int i=0; i < sceneList.size(); i++)
            {
            	if(sceneNameStr.equals(sceneList.get(i).getSceneName()) && (groupId == sceneList.get(i).getGroupId() ))
            	{
            		sceneList.get(i).setSceneId(sceneId);
            		sceneList.get(i).setStatus(ZigbeeScene.sceneStatusActive);
            		break;
            	}
            }
			break;
		}
						
		case SRPC_GET_SCENE_RSP:
		{
			short groupId = 0;
			byte sceneId = 0;
			msgLen = msg[msgPtr + SRPC_CMD_LEN_POS] + 2;
			  
			//index passed len and cmd ID
			msgPtr+=2;

			//Get the groupId
            for (int i=0; i < 2; i++, msgPtr++) 
            {   
            	//java does not support unsigned so use a bigger container to avoid conversion issues
            	int groupIdTemp = (msg[msgPtr] & 0xff);            	 
            	groupId += (groupIdTemp << (8 * i));             	
            }    
            
            //Get the sceneId
            sceneId = (byte) msg[msgPtr++];            
			
            String sceneNameStr = new String(msg, msgPtr+1, msg[msgPtr], Charset.defaultCharset());
			
            ZigbeeAssistant.newScene(sceneNameStr, groupId, sceneId, ZigbeeScene.sceneStatusActive);
            
			break;
		}
		case SRPC_GET_DEV_STATE_RSP:
		{
			short nwkAddr = 0;
			byte endPoint = 0;
			byte state = 0;
			msgLen = msg[msgPtr + SRPC_CMD_LEN_POS] + 2;
			  
			//index passed len and cmd ID
			msgPtr+=2;

			//Get the nwkAddr
            for (int i=0; i < 2; i++, msgPtr++) 
            {   
            	//java does not support unsigned so use a bigger container to avoid conversion issues
            	int nwkAddrTemp = (msg[msgPtr] & 0xff);            	 
            	nwkAddr += (nwkAddrTemp << (8 * i));             	
            }    
            
            //Get the EP
            endPoint = (byte) msg[msgPtr++]; 

            //Get the state
            state = (byte) msg[msgPtr++]; 
            
            
            List<ZigbeeDevice> devList = ZigbeeAssistant.getDevices();            
            //find the device
            for (int i = 0 ; i < devList.size() ; i++)
            {        	
            	if( ( ((short) devList.get(i).NetworkAddr) == nwkAddr) && (devList.get(i).EndPoint == endPoint) )
            	{
            		devList.get(i).setCurrentState(state);
            		break;
            	}            	
            }            				
			break;
		}
		case SRPC_GET_DEV_LEVEL_RSP:
		{
			short nwkAddr = 0;
			byte endPoint = 0;
			byte level = 0;
			msgLen = msg[msgPtr + SRPC_CMD_LEN_POS] + 2;
			  
			//index passed len and cmd ID
			msgPtr+=2;

			//Get the nwkAddr
            for (int i=0; i < 2; i++, msgPtr++) 
            {   
            	//java does not support unsigned so use a bigger container to avoid conversion issues
            	int nwkAddrTemp = (msg[msgPtr] & 0xff);            	 
            	nwkAddr += (nwkAddrTemp << (8 * i));             	
            }    
            
            //Get the EP
            endPoint = (byte) msg[msgPtr++]; 

            //Get the state
            level = (byte) msg[msgPtr++]; 
            
            
            List<ZigbeeDevice> devList = ZigbeeAssistant.getDevices();            
            //find the device
            for (int i = 0 ; i < devList.size() ; i++)
            {        	
            	if( ( ((short) devList.get(i).NetworkAddr) == nwkAddr) && (devList.get(i).EndPoint == endPoint) )
            	{
            		devList.get(i).setCurrentLevel(level);
                	break;
            	}
            }            				
			break;			
		}		
		case SRPC_GET_DEV_HUE_RSP:
		{
			short nwkAddr = 0;
			byte endPoint = 0;
			byte hue = 0;
			msgLen = msg[msgPtr + SRPC_CMD_LEN_POS] + 2;
			  
			//index passed len and cmd ID
			msgPtr+=2;

			//Get the nwkAddr
            for (int i=0; i < 2; i++, msgPtr++) 
            {   
            	//java does not support unsigned so use a bigger container to avoid conversion issues
            	int nwkAddrTemp = (msg[msgPtr] & 0xff);            	 
            	nwkAddr += (nwkAddrTemp << (8 * i));             	
            }    
            
            //Get the EP
            endPoint = (byte) msg[msgPtr++]; 

            //Get the state
            hue = (byte) msg[msgPtr++]; 
            
            
            List<ZigbeeDevice> devList = ZigbeeAssistant.getDevices();            
            //find the device
            for (int i = 0 ; i < devList.size() ; i++)
            {        	
            	if( ( ((short) devList.get(i).NetworkAddr) == nwkAddr) && (devList.get(i).EndPoint == endPoint) )
            	{
            		devList.get(i).setCurrentHue(hue);
                	break;
            	}
            }            				
			break;
		}
		case SRPC_GET_DEV_SAT_RSP:
		{
			short nwkAddr = 0;
			byte endPoint = 0;
			byte sat = 0;
			msgLen = msg[msgPtr + SRPC_CMD_LEN_POS] + 2;
			  
			//index passed len and cmd ID
			msgPtr+=2;

			//Get the nwkAddr
            for (int i=0; i < 2; i++, msgPtr++) 
            {   
            	//java does not support unsigned so use a bigger container to avoid conversion issues
            	int nwkAddrTemp = (msg[msgPtr] & 0xff);            	 
            	nwkAddr += (nwkAddrTemp << (8 * i));             	
            }    
            
            //Get the EP
            endPoint = (byte) msg[msgPtr++]; 

            //Get the state
            sat = (byte) msg[msgPtr++]; 
            
            
            List<ZigbeeDevice> devList = ZigbeeAssistant.getDevices();            
            //find the device
            for (int i = 0 ; i < devList.size() ; i++)
            {        	
            	if( ( ((short) devList.get(i).NetworkAddr) == nwkAddr) && (devList.get(i).EndPoint == endPoint) )
            	{
            		devList.get(i).setCurrentSat(sat);
            		break;
            	}            	
            }            				
			break;
		}
		
		default:
		{
			msgLen = 0;
		}		
		}
		
		return msgLen;
	}
	
	public static void setDeviceState(short nwkAddr, byte AddrMode, char endPoint, boolean state)
	{
		byte[] msg = new byte[15];
		byte  msgIdx;
		 
		//set SRPC len and CMD ID
		msg[SRPC_CMD_ID_POS] = SRPC_SET_DEV_STATE;
		msg[SRPC_CMD_LEN_POS] = 13;
		
		//set ptr to point to data
		msgIdx=2;
		
		//dstAddr.addrMode
		msg[msgIdx++] = (byte) AddrMode;
		//set afAddrMode_t nwk address		
		msg[msgIdx++] = (byte) (nwkAddr & 0xFF);
		msg[msgIdx++] = (byte) ((nwkAddr & 0xFF00)>>8);		
		//pad for an ieee addr size;
		msgIdx += 6;	
		//set Ep
		msg[msgIdx++] = (byte) endPoint;
		//pad out pan ID
		msgIdx+=2;
		
		//set State
		if(state)
		{
			msg[msgIdx++] = (byte) 1;
		}
		else
		{
			msg[msgIdx++] = (byte) 0;
		}		
		
		sendSrpc(msg);
		
	}

	public static void setDeviceLevel(short nwkAddr, byte AddrMode, char endPoint, char level, short transitionTime)
	{
		byte[] msg = new byte[17];
		byte  msgIdx;
		 
		//set SRPC len and CMD ID
		msg[SRPC_CMD_ID_POS] = SRPC_SET_DEV_LEVEL;
		msg[SRPC_CMD_LEN_POS] = 15;
		
		//set ptr to point to data
		msgIdx=2;
		
		//dstAddr.addrMode
		msg[msgIdx++] = (byte) AddrMode;
		//set afAddrMode_t nwk address		
		msg[msgIdx++] = (byte) (nwkAddr & 0xFF);
		msg[msgIdx++] = (byte) ((nwkAddr & 0xFF00)>>8);		
		//pad for an ieee addr size;
		msgIdx += 6;	
		//set Ep
		msg[msgIdx++] = (byte) endPoint;	
		//pad out pan ID
		msgIdx+=2;

		//set level
		msg[msgIdx++] = (byte) level;	

		//set transitionTime
		msg[msgIdx++] = (byte) transitionTime;		
		
		sendSrpc(msg);		
	}	

	public static void setDeviceColor(short nwkAddr, byte AddrMode, char endPoint, char hue, char saturation, short transitionTime)
	{
		byte[] msg = new byte[18];
		byte  msgIdx;
		 
		//set SRPC len and CMD ID
		msg[SRPC_CMD_ID_POS] = SRPC_SET_DEV_COLOR;
		msg[SRPC_CMD_LEN_POS] = 16;
		
		//set ptr to point to data
		msgIdx=2;
		
		//dstAddr.addrMode
		msg[msgIdx++] = (byte) AddrMode;
		//set afAddrMode_t nwk address		
		msg[msgIdx++] = (byte) (nwkAddr & 0xFF);
		msg[msgIdx++] = (byte) ((nwkAddr & 0xFF00)>>8);		
		//pad for an ieee addr size;
		msgIdx += 6;	
		//set Ep
		msg[msgIdx++] = (byte) endPoint;		
		//pad out pan ID
		msgIdx+=2;

		//set level
		msg[msgIdx++] = (byte) hue;	

		//set saturation
		msg[msgIdx++] = (byte) saturation;	
		
		//set transitionTime
		msg[msgIdx++] = (byte) transitionTime;		
		
		sendSrpc(msg);		
	}	

	public static void getDeviceState(short nwkAddr, byte AddrMode, char endPoint)
	{
		byte[] msg = new byte[15];
		byte  msgIdx;
		 
		//set SRPC len and CMD ID
		msg[SRPC_CMD_ID_POS] = SRPC_GET_DEV_STATE;
		msg[SRPC_CMD_LEN_POS] = 13;
		
		//set ptr to point to data
		msgIdx=2;
		
		//dstAddr.addrMode
		msg[msgIdx++] = (byte) AddrMode;
		//set afAddrMode_t nwk address		
		msg[msgIdx++] = (byte) (nwkAddr & 0xFF);
		msg[msgIdx++] = (byte) ((nwkAddr & 0xFF00)>>8);		
		//pad for an ieee addr size;
		msgIdx += 6;	
		//set Ep
		msg[msgIdx++] = (byte) endPoint;
		//pad out pan ID
		msgIdx+=2;		
		
		sendSrpc(msg);		
	}

	public static void getDeviceLevel(short nwkAddr, byte AddrMode, char endPoint)
	{
		byte[] msg = new byte[15];
		byte  msgIdx;
		 
		//set SRPC len and CMD ID
		msg[SRPC_CMD_ID_POS] = SRPC_GET_DEV_LEVEL;
		msg[SRPC_CMD_LEN_POS] = 13;
		
		//set ptr to point to data
		msgIdx=2;
		
		//dstAddr.addrMode
		msg[msgIdx++] = (byte) AddrMode;
		//set afAddrMode_t nwk address		
		msg[msgIdx++] = (byte) (nwkAddr & 0xFF);
		msg[msgIdx++] = (byte) ((nwkAddr & 0xFF00)>>8);		
		//pad for an ieee addr size;
		msgIdx += 6;	
		//set Ep
		msg[msgIdx++] = (byte) endPoint;
		//pad out pan ID
		msgIdx+=2;		
		
		sendSrpc(msg);		
	}	

	public static void getDeviceHue(short nwkAddr, byte AddrMode, char endPoint)
	{
		byte[] msg = new byte[15];
		byte  msgIdx;
		 
		//set SRPC len and CMD ID
		msg[SRPC_CMD_ID_POS] = SRPC_GET_DEV_HUE;
		msg[SRPC_CMD_LEN_POS] = 13;
		
		//set ptr to point to data
		msgIdx=2;
		
		//dstAddr.addrMode
		msg[msgIdx++] = (byte) AddrMode;
		//set afAddrMode_t nwk address		
		msg[msgIdx++] = (byte) (nwkAddr & 0xFF);
		msg[msgIdx++] = (byte) ((nwkAddr & 0xFF00)>>8);		
		//pad for an ieee addr size;
		msgIdx += 6;	
		//set Ep
		msg[msgIdx++] = (byte) endPoint;
		//pad out pan ID
		msgIdx+=2;		
		
		sendSrpc(msg);		
	}

	public static void getDeviceSat(short nwkAddr, byte AddrMode, char endPoint)
	{
		byte[] msg = new byte[15];
		byte  msgIdx;
		 
		//set SRPC len and CMD ID
		msg[SRPC_CMD_ID_POS] = SRPC_GET_DEV_SAT;
		msg[SRPC_CMD_LEN_POS] = 13;
		
		//set ptr to point to data
		msgIdx=2;
		
		//dstAddr.addrMode
		msg[msgIdx++] = (byte) AddrMode;
		//set afAddrMode_t nwk address		
		msg[msgIdx++] = (byte) (nwkAddr & 0xFF);
		msg[msgIdx++] = (byte) ((nwkAddr & 0xFF00)>>8);		
		//pad for an ieee addr size;
		msgIdx += 6;	
		//set Ep
		msg[msgIdx++] = (byte) endPoint;
		//pad out pan ID
		msgIdx+=2;		
		
		sendSrpc(msg);		
	}
	
	public static boolean bindDevices(short network_a, char end_a, byte ieee_a[], short network_b, char end_b, byte ieee_b[], short clusterId)
	{
		byte[] msg = new byte[24];
		byte  msgIdx;

		//set SRPC len and CMD ID
		msg[SRPC_CMD_ID_POS] = SRPC_BIND_DEVICES;
		msg[SRPC_CMD_LEN_POS] = 22;
		
		//set ptr to point to data
		msgIdx=2;
		
		//set src nwk address		
		msg[msgIdx++] = (byte) (network_a & 0xFF);
		msg[msgIdx++] = (byte) ((network_a & 0xFF00)>>8);		

		//set Ep
		msg[msgIdx++] = (byte) end_a;

		//set ieee of a
		msg[msgIdx++] = ieee_a[0];
		msg[msgIdx++] = ieee_a[1];
		msg[msgIdx++] = ieee_a[2];
		msg[msgIdx++] = ieee_a[3];
		msg[msgIdx++] = ieee_a[4];
		msg[msgIdx++] = ieee_a[5];
		msg[msgIdx++] = ieee_a[6];
		msg[msgIdx++] = ieee_a[7];

		//set Ep
		msg[msgIdx++] = (byte) end_b;
		  
		//set ieee of b
		msg[msgIdx++] = ieee_b[0];
		msg[msgIdx++] = ieee_b[1];
		msg[msgIdx++] = ieee_b[2];
		msg[msgIdx++] = ieee_b[3];
		msg[msgIdx++] = ieee_b[4];
		msg[msgIdx++] = ieee_b[5];
		msg[msgIdx++] = ieee_b[6];
		msg[msgIdx++] = ieee_b[7];

		//set clusterId		
		msg[msgIdx++] = (byte) (clusterId & 0xFF);
		msg[msgIdx++] = (byte) ((clusterId & 0xFF00)>>8);	
		
		sendSrpc(msg);
		
		return true;
	}			

	public static void getDevices()
	{
		byte[] msg = new byte[2];

		//set SRPC len and CMD ID
		msg[SRPC_CMD_ID_POS] = SRPC_GET_DEVICES;
		msg[SRPC_CMD_LEN_POS] = 0;
		
		sendSrpc(msg);
	}
	
	public static void discoverGroups()
	{
		byte[] msg = new byte[2];

		//set SRPC len and CMD ID
		msg[SRPC_CMD_ID_POS] = SRPC_GET_GROUPS;
		msg[SRPC_CMD_LEN_POS] = 0;
		
		sendSrpc(msg);
	}

	public static void discoverScenes()
	{
		byte[] msg = new byte[2];

		//set SRPC len and CMD ID
		msg[SRPC_CMD_ID_POS] = SRPC_GET_SCENES;
		msg[SRPC_CMD_LEN_POS] = 0;
		
		sendSrpc(msg);
	}	
	
	public static void addGroup(short nwkAddr, char endPoint, String groupName)
	{
		byte[] msg = new byte[14 + groupName.length() + 1];
		byte msgIdx;

		//set SRPC len and CMD ID
		msg[SRPC_CMD_ID_POS] = SRPC_ADD_GROUP;
		msg[SRPC_CMD_LEN_POS] = (byte) (12 + groupName.length() + 1);
		
		//set ptr to point to data
		msgIdx=2;
		
		//dstAddr.addrMode = Addr16Bit
		msg[msgIdx++] = (byte) Addr16Bit;
		//set afAddrMode_t nwk address		
		msg[msgIdx++] = (byte) (nwkAddr & 0xFF);
		msg[msgIdx++] = (byte) ((nwkAddr & 0xFF00)>>8);		
		//pad for an ieee addr size;
		msgIdx += 6;	
		//set Ep
		msg[msgIdx++] = (byte) endPoint;		
		//pad out pan ID
		msgIdx+=2;

		msg[msgIdx++] = (byte) groupName.length();
		for(int i = 0; i < groupName.length(); i++)
		{
			msg[msgIdx++] = groupName.getBytes()[i];
		}
				
		sendSrpc(msg);				
	}
	
	public static void storeScene(String sceneName, int groupId)
	{
		byte[] msg = new byte[2 + 15 + sceneName.length()];
		byte msgIdx;

		//set SRPC len and CMD ID
		msg[SRPC_CMD_ID_POS] = SRPC_STORE_SCENE;
		msg[SRPC_CMD_LEN_POS] = (byte) (15 + sceneName.length());
		
		//set ptr to point to data
		msgIdx=2;		

		//dstAddr.addrMode = Addr16Bit
		msg[msgIdx++] = (byte) AddrGroup;
		//set afAddrMode_t nwk address		
		msg[msgIdx++] = (byte) (groupId & 0xFF);
		msg[msgIdx++] = (byte) ((groupId & 0xFF00)>>8);		
		//pad for an ieee addr size;
		msgIdx += 6;	
		//set Ep
		msg[msgIdx++] = (byte) 0xFF;		
		//pad out pan ID
		msgIdx+=2;
		
		msg[msgIdx++] = (byte) (groupId & 0xFF);
		msg[msgIdx++] = (byte) ((groupId & 0xFF00)>>8);	

		msg[msgIdx++] = (byte) sceneName.length();
		for(int i = 0; i < sceneName.length(); i++)
		{
			msg[msgIdx++] = sceneName.getBytes()[i];
		}
		
		sendSrpc(msg);					
	}

	public static void recallScene(String sceneName, int groupId)
	{
		byte[] msg = new byte[2 + 15 + sceneName.length()];
		byte msgIdx;

		//set SRPC len and CMD ID
		msg[SRPC_CMD_ID_POS] = SRPC_RECALL_SCENE;
		msg[SRPC_CMD_LEN_POS] = (byte) (15 + sceneName.length());
		
		//set ptr to point to data
		msgIdx=2;		

		//dstAddr.addrMode = Addr16Bit
		msg[msgIdx++] = (byte) AddrGroup;
		//set afAddrMode_t nwk address		
		msg[msgIdx++] = (byte) (groupId & 0xFF);
		msg[msgIdx++] = (byte) ((groupId & 0xFF00)>>8);		
		//pad for an ieee addr size;
		msgIdx += 6;	
		//set Ep
		msg[msgIdx++] = (byte) 0xFF;		
		//pad out pan ID
		msgIdx+=2;
		
		//set groupId
		msg[msgIdx++] = (byte) (groupId & 0xFF);
		msg[msgIdx++] = (byte) ((groupId & 0xFF00)>>8);	

		msg[msgIdx++] = (byte) sceneName.length();
		for(int i = 0; i < sceneName.length(); i++)
		{
			msg[msgIdx++] = sceneName.getBytes()[i];
		}
		
		sendSrpc(msg);					
	}
	

	public static void IdentifyDevice(short nwkAddr, byte AddrMode, char endPoint, short identifyTime)
	{
		byte[] msg = new byte[16];
		byte  msgIdx;
		 
		//set SRPC len and CMD ID
		msg[SRPC_CMD_ID_POS] = SRPC_IDENTIFY_DEVICE;
		msg[SRPC_CMD_LEN_POS] = 14;
		
		//set ptr to point to data
		msgIdx=2;
		
		//dstAddr.addrMode
		msg[msgIdx++] = (byte) AddrMode;
		//set afAddrMode_t nwk address		
		msg[msgIdx++] = (byte) (nwkAddr & 0xFF);
		msg[msgIdx++] = (byte) ((nwkAddr & 0xFF00)>>8);		
		//pad for an ieee addr size;
		msgIdx += 6;	
		//set Ep
		msg[msgIdx++] = (byte) endPoint;	
		//pad out pan ID
		msgIdx+=2;

		//set transitionTime
		msg[msgIdx++] = (byte) identifyTime;		
		
		sendSrpc(msg);		
	}	
	
	public static void changeDeviceName(ZigbeeDevice device, String deviceName)
	{
		byte[] msg = new byte[4 + deviceName.length()];
		byte msgIdx;

		//set SRPC len and CMD ID
		msg[SRPC_CMD_ID_POS] = SRPC_CHANGE_DEVICE_NAME;
		msg[SRPC_CMD_LEN_POS] = (byte) (2 + deviceName.length());
		
		//set ptr to point to data
		msgIdx=2;		

		//set afAddrMode_t nwk address		
		msg[msgIdx++] = (byte) (device.NetworkAddr & 0xFF);
		msg[msgIdx++] = (byte) ((device.NetworkAddr & 0xFF00)>>8);	

		msg[msgIdx++] = (byte) deviceName.length();
		for(int i = 0; i < deviceName.length(); i++)
		{
			msg[msgIdx++] = deviceName.getBytes()[i];
		}
		
		sendSrpc(msg);					
	}	
	
	public static int clientConnect( )
	{
		int Port;
		
		Port = Integer.parseInt(gatewayPort); 				        
/*		
		try { 			 	        
			kkSocket = new Socket(gatewayIp, Port);
			//kkSocket = new Socket(); 
            SocketAddress adr = new InetSocketAddress(gatewayIp,Port);
			kkSocket.
            kkSocket.connect(adr, 5000); 			
		} catch (UnknownHostException e) { 
			e.printStackTrace();
			return 1;
			//errorMessage("Unknown host" + gatewayIp); 
		} catch (IOException e) {
			e.printStackTrace();
			return 1;
	        //errorMessage("Couldn't get I/O for the connection to: " + gatewayIp); 
		}*/
		
        SocketAddress sockaddr = new InetSocketAddress(gatewayIp, Port);
        kkSocket = new Socket();
        try {
			kkSocket.connect(sockaddr, 5000); //5 second connection timeout
		} catch (IOException e1) {
			return 1;
		} 
        
        
        if (kkSocket.isConnected()) { 
		
			ZigbeeAssistant.gateWayConnected = true;
			
			try {
				outStream = kkSocket.getOutputStream();
			} catch (IOException e) {
				e.printStackTrace();
				return 1;
			}
			
			try {
				inStream = kkSocket.getInputStream();
			} catch (IOException e) {
				e.printStackTrace();
				return 1;
			}		
			
			//create Rx thread
			new Thread(new Runnable() 
			{    
				public void run() {   
					Listen();
				}  
			}).start();
			
			return 0;
        }
        
        return 1;
	}
	
	public static void Disconnect()
	{
		if(kkSocket != null)
		{
		try {
				kkSocket.close();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		    ZigbeeAssistant.gateWayConnected = false;
		}
	}
	

	public static void sendSrpc(byte[] msg)
	{

		try{		
			outStream.write( msg );	
		} catch (Exception e){}		
	}
	
	private static void Listen() {
	//listen for Packets
		while(true)
		{		
			byte[] RxBuffer = new byte[1024];
			try {
				int bytesRead = inStream.read(RxBuffer);
				int bytesProcessed = 0;
				while ( bytesRead > bytesProcessed )
				{
					bytesProcessed += rpcsProcessIncoming(RxBuffer, bytesProcessed);
				}
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
	}

	public static String getGatewayIp() {
		return gatewayIp;
	}

	public static void setGatewayIp(String gatewayIp) {
		ZigbeeSrpcClient.gatewayIp = gatewayIp;
	}
}
