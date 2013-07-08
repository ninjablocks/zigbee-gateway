/**************************************************************************************************
  Filename:       sceneSelect.java
  Revised:        $$
  Revision:       $$

  Description:    Scene UI

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

package com.lightingcontroller;

import java.util.ArrayList;
import java.util.List;

import com.lightingcontroller.R;
import com.lightingcontroller.Zigbee.ZigbeeAssistant;
import com.lightingcontroller.Zigbee.ZigbeeDevice;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.RadioButton;
import android.widget.Spinner;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ToggleButton;

public class bindSelect extends Activity {

	private static ZigbeeDevice currControllingDevice;
	private Spinner controllingDeviceSpinner;
	private ArrayAdapter<String> controllingDeviceSpinnerAdapter;

	private static ZigbeeDevice currControlledDevice;
	private Spinner controlledDeviceSpinner;
	private ArrayAdapter<String> controlledDeviceSpinnerAdapter;
	
	ProgressDialog bar;
	
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        this.overridePendingTransition(R.anim.animation_enter_l, R.anim.animation_leave_r);  
                
        setContentView(R.layout.bindview);        

		List<ZigbeeDevice> controllingDevList = ZigbeeAssistant.getSwitchers();
        List<String> controllingDevNameList = new ArrayList<String>();
        for (int i = 0 ; i < controllingDevList.size() ; i++)
        {
        	controllingDevNameList.add(controllingDevList.get(i).Name);        
        }                 
        controllingDeviceSpinner = (Spinner) findViewById(R.id.controllingDeviceSpinner);
        controllingDeviceSpinnerAdapter = new ArrayAdapter<String>(this,
        		android.R.layout.simple_spinner_item, controllingDevNameList);
        controllingDeviceSpinnerAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        controllingDeviceSpinner.setAdapter(controllingDeviceSpinnerAdapter);
        
        addItemsOnControllingDeviceSpinner();
        addListenerOnControllingDeviceSpinnerItemSelection(); 

		List<ZigbeeDevice> controlledDevList = ZigbeeAssistant.getSwitchable();
        List<String> controlledDevNameList = new ArrayList<String>();
        for (int i = 0 ; i < controlledDevList.size() ; i++)
        {
        	controlledDevNameList.add(controlledDevList.get(i).Name);        
        }                 
        controlledDeviceSpinner = (Spinner) findViewById(R.id.controlledDeviceSpinner);
        controlledDeviceSpinnerAdapter = new ArrayAdapter<String>(this,
        		android.R.layout.simple_spinner_item, controlledDevNameList);
        controlledDeviceSpinnerAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        controlledDeviceSpinner.setAdapter(controlledDeviceSpinnerAdapter);
        
        addItemsOnControlledDeviceSpinner();
        addListenerOnControlledDeviceSpinnerItemSelection();         
    }    

    // add items into spinner dynamically
    public void addItemsOnControllingDeviceSpinner() {  
    	//add devices
        List<ZigbeeDevice> devList = ZigbeeAssistant.getSwitchers();
        controllingDeviceSpinnerAdapter.clear();
        for (int i = 0 ; i < devList.size() ; i++)
        {             	
        	controllingDeviceSpinnerAdapter.add(devList.get(i).Name);
        }     
    }
   
    public void addListenerOnControllingDeviceSpinnerItemSelection() {    	
    	controllingDeviceSpinner.setOnItemSelectedListener(new OnItemSelectedListener() {
    	    @Override
    	    public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
    	    	List<ZigbeeDevice> devList = ZigbeeAssistant.getSwitchers();        	    	
    	    	currControllingDevice = devList.get(position);
    	    }
      	 
      	  @Override
      	  public void onNothingSelected(AdapterView<?> arg0) {
      	  }
    	});
    }        

    // add items into spinner dynamically
    public void addItemsOnControlledDeviceSpinner() {  
    	//add devices
        List<ZigbeeDevice> devList = ZigbeeAssistant.getSwitchable();
        controlledDeviceSpinnerAdapter.clear();
        for (int i = 0 ; i < devList.size() ; i++)
        {             	
        	controlledDeviceSpinnerAdapter.add(devList.get(i).Name);
        }     
    }
   
    public void addListenerOnControlledDeviceSpinnerItemSelection() {    	
    	controlledDeviceSpinner.setOnItemSelectedListener(new OnItemSelectedListener() {
    	    @Override
    	    public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
    	    	List<ZigbeeDevice> devList = ZigbeeAssistant.getSwitchable();        	    	
    	    	currControlledDevice = devList.get(position);
    	    }
      	 
      	  @Override
      	  public void onNothingSelected(AdapterView<?> arg0) {
      	  }
    	});
    }            
    
    public void IdControllingButton(View view) {
    	ToggleButton idDeviceButton = (ToggleButton) findViewById(R.id.IdControllingToggle);
    	if ( (idDeviceButton.isChecked()) && (currControllingDevice != null))
    	{
    		//ZigbeeAssistant.Identify(currControllingDevice, (short) 0x80);
    	}
    	else if(currControllingDevice != null)
    	{
    		//ZigbeeAssistant.Identify(currControllingDevice, (short) 0);    		
    	}    
    }
    
    public void IdControlledButton(View view) {
    	ToggleButton idDeviceButton = (ToggleButton) findViewById(R.id.IdControlledToggle);
    	if ( (idDeviceButton.isChecked()) && (currControlledDevice != null))
    	{
    		//ZigbeeAssistant.Identify(currControlledDevice, (short) 0x80);
    	}
    	else if(currControlledDevice != null)
    	{
    		//ZigbeeAssistant.Identify(currControlledDevice, (short) 0);    		
    	}    
    }
    
    public void bindButton(View view) {

    	//de-select button
    	RadioButton bindRadio = (RadioButton) findViewById(R.id.bindRadio);
    	bindRadio.setChecked(false);
    	
    	if((currControllingDevice != null) && (currControlledDevice != null))
    	{
	    	AlertDialog show = new AlertDialog.Builder(bindSelect.this)
			.setTitle("Bind devices")
			.setMessage("Bind " + currControllingDevice.Name + " to " + currControlledDevice.Name)
			.setPositiveButton("OK",             
			new DialogInterface.OnClickListener()
			{			
				public void onClick(DialogInterface dialoginterface,int i){		
					//only support on/off for now but can expand to others by adding a spinner 
					//to select cluster and populate controlling and controlled spinners accordingly 
			    	ZigbeeAssistant.bindDevices(currControllingDevice, currControlledDevice, 0x0006); 
				}		
			})	    	    		
			.setNegativeButton("Cancel", null)
			.show();	    	    		
    	}
    	else
    	{
	    	AlertDialog show = new AlertDialog.Builder(bindSelect.this)
			.setTitle("Bind devices")
			.setMessage("Select controlling and controlled")
			.setPositiveButton("OK",             
			new DialogInterface.OnClickListener()
			{			
				public void onClick(DialogInterface dialoginterface,int i){				    	
				}		
			})	    	    		
			.show();	    	    		
    	}    		

    }    
    	
}

