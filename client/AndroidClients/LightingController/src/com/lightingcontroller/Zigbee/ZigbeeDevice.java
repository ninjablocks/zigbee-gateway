/**************************************************************************************************
  Filename:       ZigBeeDevice.java
  Revised:        $$
  Revision:       $$

  Description:    ZigBee Device Class

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

import android.content.Context;
import android.widget.TextView;

import com.lightingcontroller.zbLighitngMain;


public class ZigbeeDevice {

	private String TAG = "ZigbeeAssistant";
	public String Name = "Unknown Device";
	public String Type = "Unknown Device";
	public char EndPoint = 0;
	public int ProfileId = 0;
	public int DeviceId = 0;
	public int NetworkAddr = 0;
	public byte Ieee[] = {};
	
	// Prob better to do this as an array of cluster IDS.
	// Never mind.
	public boolean hasColourable = false;
	public boolean hasDimmable = false;
	public boolean hasSwitchable = false;
	public boolean hasThermometer = false;
	public boolean hasPowerUsage = false;
	public boolean hasOutSwitch = false;
	public boolean hasOutLeveL = false;
	public boolean hasOutColor = false;
	public boolean hasOutScene = false;
	public boolean hasOutGroup = false;	
	
	private byte currentState = 0;
	private byte currentLevel = 0;
	private byte currentHue = 0;
	private byte currentSat = 0;
	public byte currentColorAttrsUpdated = 0;
	private static final byte CURRENT_STATE_UPDATED = (byte) 0x01;	
	private static final byte CURRENT_LEVEL_UPDATED = (byte) 0x02;	
	private static final byte CURRENT_HUE_UPDATED = (byte) 0x04;	
	private static final byte CURRENT_SAT_UPDATED = (byte) 0x08;	
	
	// Zigbee Home Automation Profile Identification
	private static final int ZCL_HA_PROFILE_ID = (int) 0x0104;	
	// Zigbee Home Automation Profile Generic Device IDs
	private static final int ZCL_HA_DEVICEID_LEVEL_CONTROL_SWITCH            = (int) 0x0001; 
	private static final int ZCL_HA_DEVICEID_ON_OFF_OUTPUT                   = (int) 0x0002; 
	private static final int ZCL_HA_DEVICEID_LEVEL_CONTROLLABLE_OUTPUT       = (int) 0x0003; 
	private static final int ZCL_HA_DEVICEID_SCENE_SELECTOR                  = (int) 0x0004; 
	private static final int ZCL_HA_DEVICEID_CONFIGURATIOPN_TOOL             = (int) 0x0005; 
	private static final int ZCL_HA_DEVICEID_REMOTE_CONTROL                  = (int) 0x0006; 
	private static final int ZCL_HA_DEVICEID_COMBINED_INETRFACE              = (int) 0x0007; 
	private static final int ZCL_HA_DEVICEID_RANGE_EXTENDER                  = (int) 0x0008; 
	private static final int ZCL_HA_DEVICEID_MAINS_POWER_OUTLET              = (int) 0x0009; 	
	// Zigbee Home Automation Profile  Lighting Device IDs
	private static final int ZCL_HA_DEVICEID_ON_OFF_SWITCH          = (int) 0x0000;	
	private static final int ZCL_HA_DEVICEID_ON_OFF_LIGHT           = (int) 0x0100;
	private static final int ZCL_HA_DEVICEID_DIMMABLE_LIGHT         = (int) 0x0101;
	private static final int ZCL_HA_DEVICEID_COLORED_DIMMABLE_LIGHT = (int) 0x0102;
	private static final int ZCL_HA_DEVICEID_ON_OFF_LIGHT_SWITCH    = (int) 0x0103;
	private static final int ZCL_HA_DEVICEID_DIMMER_SWITCH          = (int) 0x0104;
	private static final int ZCL_HA_DEVICEID_COLOR_DIMMER_SWITCH    = (int) 0x0105;
	private static final int ZCL_HA_DEVICEID_LIGHT_SENSOR           = (int) 0x0106;
	private static final int ZCL_HA_DEVICEID_OCCUPANCY_SENSOR       = (int) 0x0107;	
	
	// Zigbee Light Link Profile Identification
	private static final int  ZLL_PROFILE_ID                        = (int) 0xc05e;
	// ZLL Basic Lighting Device IDs
	private static final int ZLL_DEVICEID_ON_OFF_LIGHT              = (int) 0x0000;
	private static final int ZLL_DEVICEID_ON_OFF_PLUG_IN_UNIT       = (int) 0x0010;
	private static final int ZLL_DEVICEID_DIMMABLE_LIGHT            = (int) 0x0100;
	private static final int ZLL_DEVICEID_DIMMABLE_PLUG_IN_UNIT     = (int) 0x0110;
	// ZLL Color Lighting Device IDs
	private static final int ZLL_DEVICEID_COLOR_LIGHT                = (int) 0x0200;
	private static final int ZLL_DEVICEID_EXTENDED_COLOR_LIGHT       = (int) 0x0210;
	private static final int ZLL_DEVICEID_COLOR_TEMPERATURE_LIGHT    = (int) 0x0220;
	// ZLL Lighting Remotes Device IDs
	private static final int ZLL_DEVICEID_COLOR_CONTORLLER           = (int) 0x0800;
	private static final int ZLL_DEVICEID_COLOR_SCENE_CONTROLLER     = (int) 0x0810;
	private static final int ZLL_DEVICEID_NON_COLOR_CONTORLLER       = (int) 0x0820;
	private static final int ZLL_DEVICEID_NON_COLOR_SCENE_CONTROLLER = (int) 0x0830;
	private static final int ZLL_DEVICEID_CONTROL_BRIDGE             = (int) 0x0840;
	private static final int ZLL_DEVICEID_ON_OFF_SENSOR              = (int) 0x0850;	
	
	ZigbeeDevice() {}
	
	ZigbeeDevice(int _ProfileId, int _DeviceId, int _NetworkAddr, char _EndPoint,  byte _ieee[], String _deviceName, int LightDeviceIdx)
	{
		ProfileId = _ProfileId;
		DeviceId = _DeviceId;
		EndPoint = _EndPoint;
		NetworkAddr = _NetworkAddr;
		Ieee = _ieee;
		
		if( ProfileId == ZCL_HA_PROFILE_ID)
		{			
			// Would be better to get the device name from the device!
			switch (DeviceId)
			{
			
			case (ZCL_HA_DEVICEID_ON_OFF_SWITCH):	
			{
				Type = "Switch";
				
				//Assume for now it is a Cleode switch, later pass the vindor ID down so we can check
		        String SwitchButtonFunction;
		        
		        if(_EndPoint == 1)
		        {
		        	SwitchButtonFunction = " EP:1(DOWN)";
		        }
		        else if(_EndPoint == 2)
		        {
		        	SwitchButtonFunction = " EP:2(LEFT)";
		        }
		        else if(_EndPoint == 3)
		        {
		        	SwitchButtonFunction = " EP:3(UP)";
		        }
		        else if(_EndPoint == 4)
		        {
		        	SwitchButtonFunction = " EP:4(RIGHT)";
		        }            
		        else if(_EndPoint == 5)
		        {
		        	SwitchButtonFunction = " EP:5(CENTER)";
		        }
		        else
		        {
		        	SwitchButtonFunction = " EP:" + Integer.toHexString(EndPoint);
		        }
		        
		        Name = "Switch" + SwitchButtonFunction  /*+ " IEEE:" + Integer.toHexString((int)((long)ZigbeeAssistant.nwrkToIEEE.get((short)_NetworkAddr)))*/;
		        hasOutSwitch = true;
		        break;
			}
	        
			case (ZCL_HA_DEVICEID_MAINS_POWER_OUTLET):	
			{
				Type = "Mains Outlet"; 
				Name= (LightDeviceIdx+1) + ": Smart Plug";	
				hasSwitchable = true;				
				break;
			}
			case (ZCL_HA_DEVICEID_DIMMABLE_LIGHT):	
			{
				Type = "Dimmable Light"; 
				Name= (LightDeviceIdx+1) + ": Light";
				hasSwitchable = true;
				hasDimmable  = true;
				break;
			}
			case (ZCL_HA_DEVICEID_COLORED_DIMMABLE_LIGHT):	
			{
				Type = "Colour Dimmable Light"; 
				Name= (LightDeviceIdx+1) + ": ZLight";	
				hasSwitchable = true;
				hasDimmable  = true;
				hasColourable = true;					
				break;
			}
			case (ZCL_HA_DEVICEID_COLOR_DIMMER_SWITCH):	
			{
				Type = "Colour Dimmer switch"; 
				Name="Colour Dimmer switch";
				hasOutSwitch= true;
				hasOutLeveL = true;
				hasOutColor = true;				
				break;	
			}
			case (ZCL_HA_DEVICEID_OCCUPANCY_SENSOR):	
			{
				Type = "Occumpancy Sensor"; 
				Name="Occumpancy Sensor";
				hasOutSwitch = true;
				break;
			}
			//some manufacturers use HA profile ID and ZLL device ID's... 
			case (ZLL_DEVICEID_EXTENDED_COLOR_LIGHT):	
			{
				Type = "ZLL Light"; 
				Name= (LightDeviceIdx+1) + ": ZLL Light";		
				break;
			}
			default:	Type = "Unknown Device"; Name="Unknown";	
			}			
		}		
		else if( ProfileId == ZLL_PROFILE_ID)
		{
			switch (DeviceId)
			{
			case (ZLL_DEVICEID_ON_OFF_LIGHT):	
			{
				//An On off Light
				Type = "On/Off Light"; 
				//set a default name that can be renamed later and stored in the hub
				Name= (LightDeviceIdx+1) + ": On/Off Light";	
				hasSwitchable = true;
				hasDimmable  = false;
				hasColourable = false;			
				break;
			}
			case (ZLL_DEVICEID_ON_OFF_PLUG_IN_UNIT):	
			{
				//An On Off Plug-In unit
				Type = "On/Off Plug"; 
				//set a default name that can be renamed later and stored in the hub
				Name= (LightDeviceIdx+1) + ": On/Off Plug";	
				hasSwitchable = true;			
				break;			
			}		
			case (ZLL_DEVICEID_DIMMABLE_LIGHT):	
			{
				//A Dimmable Light
				Type = "Dimmable Light"; 
				//set a default name that can be renamed later and stored in the hub
				Name= (LightDeviceIdx+1) + ": Dimmable Light";	
				hasSwitchable = true;
				hasDimmable  = true;
				break;			
			}
			case (0x0110):	
			{
				//A Dimmable Plug-In unit
				Type = "Dimmable Plug"; 
				//set a default name that can be renamed later and stored in the hub
				Name= (LightDeviceIdx+1) + ": Dimmable Plug";	
				hasSwitchable = true;
				hasDimmable  = true;
				break;			
			}		
			case (0x0200):	
			{
				//A Color Light
				Type = "Color Light"; 
				//set a default name that can be renamed later and stored in the hub
				Name= (LightDeviceIdx+1) + ": Color Light";	
				hasSwitchable = true;
				hasDimmable  = true;
				hasColourable = true;	
				break;			
			}
			case (0x0210):	
			{
				//An Extended Color Light
				Type = "Extended Color Light"; 
				//set a default name that can be renamed later and stored in the hub
				Name= (LightDeviceIdx+1) + ": Extended Color Light";	
				hasSwitchable = true;
				hasDimmable  = true;
				hasColourable = true;	
				break;			
			}
			case (0x0220):	
			{
				//An Color Temp Light
				Type = "Color Temp Light"; 
				//set a default name that can be renamed later and stored in the hub
				Name= (LightDeviceIdx+1) + ": Color Temp Light";	
				hasSwitchable = true;
				hasDimmable  = true;
				hasColourable = true;	
				break;			
			}		
			case (0x0800):	
			{
				//A Color Controller
				Type = "Color Controller"; 
				//set a default name that can be renamed later and stored in the hub
				Name= (LightDeviceIdx+1) + ": Color Controller";	
				hasOutSwitch = true;
				hasOutLeveL  = true;
				hasOutColor = true;	
				break;			
			}
			case (0x0810):	
			{
				//A Color Scene Controller
				Type = "Color Scene Controller"; 
				//set a default name that can be renamed later and stored in the hub
				Name= (LightDeviceIdx+1) + ": Color Scene Controller";	
				hasOutSwitch = true;
				hasOutLeveL  = true;
				hasOutColor = true;	
				hasOutScene = true;
				hasOutGroup = true; 
				break;			
			}		
			case (0x0820):	
			{
				//A Dimmable Scene Controller
				Type = "Dimmable Scene Controller"; 
				//set a default name that can be renamed later and stored in the hub
				Name= (LightDeviceIdx+1) + ": Dimmable Scene Controller";	
				hasOutSwitch = true;
				hasOutLeveL  = true;
				hasOutScene = true;
				hasOutGroup = true; 
				break;			
			}
			case (0x0830):	
			{
				//A Dimmable Controller
				Type = "Dimmable Controller"; 
				//set a default name that can be renamed later and stored in the hub
				Name= (LightDeviceIdx+1) + ": Dimmable Controller";	
				hasOutSwitch = true;
				hasOutLeveL  = true;
				break;			
			}
			case (0x0840):	
			{
				//A Control Bridge
				Type = "Control Bridge"; 
				//set a default name that can be renamed later and stored in the hub
				Name= (LightDeviceIdx+1) + ": Control Bridge";	
				hasOutSwitch = true;
				hasOutLeveL  = true;
				hasOutColor = true;	
				hasOutScene = true;
				hasOutGroup = true; 
				break;			
			}	
			case (0x0850):	
			{
				//An On/Off Sensor
				Type = "On/Off Sensor"; 
				//set a default name that can be renamed later and stored in the hub
				Name= (LightDeviceIdx+1) + ": On/Off Sensor";	
				hasOutSwitch = true;
				break;			
			}
			
			default:	Type = "Unknown Device"; Name="Unknown";	
			}
		}

		String notification = "New Device:\n"+Name; 
		ZigbeeNotification.showNotification(notification);
	}

	public byte getCurrentState() {
		return this.currentState;
	}

	public void setCurrentState(byte currentState) {
		this.currentState = currentState;
		this.currentColorAttrsUpdated |= CURRENT_STATE_UPDATED;
	}

	public byte getCurrentLevel() {
		return this.currentLevel;
	}

	public void setCurrentLevel(byte currentLevel) {
		this.currentLevel = currentLevel;
		this.currentColorAttrsUpdated |= CURRENT_LEVEL_UPDATED;
	}

	public byte getCurrentHue() {
		return this.currentHue;
	}

	public void setCurrentHue(byte currentHue) {
		this.currentHue = currentHue;
		this.currentColorAttrsUpdated |= CURRENT_HUE_UPDATED;
	}

	public byte getCurrentSat() {
		return this.currentSat;
	}

	public void setCurrentSat(byte currentSat) {
		this.currentSat = currentSat;
		this.currentColorAttrsUpdated |= CURRENT_SAT_UPDATED;
	}	
	
	public void clearCurrentStateUpdated() {
		currentColorAttrsUpdated &= ~CURRENT_STATE_UPDATED;
	}		
	
	public void clearCurrentLevelUpdated() {
		currentColorAttrsUpdated &= ~CURRENT_LEVEL_UPDATED;
	}
	
	public void clearCurrentHueUpdated() {
		currentColorAttrsUpdated &= ~CURRENT_HUE_UPDATED;
	}
	
	public void clearCurrentSatUpdated() {
		currentColorAttrsUpdated &= ~CURRENT_SAT_UPDATED;
	}	
	
	public boolean getCurrentStateUpdated() {
		boolean rtn = true;
		
		if(hasSwitchable == false)
		{
			rtn = true;			
		}		
		else if ((currentColorAttrsUpdated & CURRENT_STATE_UPDATED) == 0)
		{
			rtn = false;
		}
				
		return rtn;
	}
	
	public boolean getCurrentLevelUpdated() {
		boolean rtn = true;
		
		if(hasDimmable == false)
		{
			rtn = true;			
		}		
		else if ((currentColorAttrsUpdated & CURRENT_LEVEL_UPDATED) == 0)
		{
			rtn = false;
		}		
		
		return rtn;
	}
	
	public boolean getCurrentHueUpdated() {
		boolean rtn = true;
		
		if(this.hasColourable == false)
		{
			rtn = true;			
		}			
		else if ((currentColorAttrsUpdated & CURRENT_HUE_UPDATED) == 0)
		{
			rtn = false;
		}	
		
		return rtn;
	}
	
	public boolean getCurrentSatUpdated() {
		boolean rtn = true;
		
		if(this.hasColourable == false)
		{
			rtn = true;			
		}		
		else if ((currentColorAttrsUpdated & CURRENT_SAT_UPDATED) == 0)
		{
			rtn = false;
		}
		
		return rtn;
	}	
}
