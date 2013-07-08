/**************************************************************************************************
  Filename:       groupSelect.java
  Revised:        $$
  Revision:       $$

  Description:    Group UI

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
import com.lightingcontroller.Zigbee.ZigbeeGroup;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.DialogInterface;
import android.os.AsyncTask;
import android.os.Bundle;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.EditText;
import android.widget.RadioButton;
import android.widget.Spinner;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ToggleButton;

public class groupSelect extends Activity {

	private static ZigbeeDevice currDevice;
	private Spinner deviceSpinner;
	private ArrayAdapter<String> deviceSpinnerAdapter;
	private int currentDeviceSpinnerSelection=0;
	
	private static int currGroup = 0;
	private static Spinner groupSpinner;
	private static ArrayAdapter<String> groupSpinnerAdapter;	
	
	ProgressDialog bar;
	
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        this.overridePendingTransition(R.anim.animation_enter_l, R.anim.animation_leave_r);  
                
        setContentView(R.layout.groupview);        

		List<ZigbeeDevice> devList = ZigbeeAssistant.getSwitchable();
        List<String> devNameList = new ArrayList<String>();
        for (int i = 0 ; i < devList.size() ; i++)
        {
        	devNameList.add(devList.get(i).Name);        
        }                 
    	deviceSpinner = (Spinner) findViewById(R.id.deviceSpinner);
    	deviceSpinnerAdapter = new ArrayAdapter<String>(this,
        		android.R.layout.simple_spinner_item, devNameList);
    	deviceSpinnerAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
    	deviceSpinner.setAdapter(deviceSpinnerAdapter);
        
        addItemsOnDeviceSpinner();
        addListenerOnDeviceSpinnerItemSelection(); 
        
        List<ZigbeeGroup> groupList = ZigbeeAssistant.getGroups();
        List<String> groupNameList = new ArrayList<String>();
        for (int i = 0 ; i < groupList.size() ; i++)
        {
        	groupNameList.add(groupList.get(i).getGroupName());        
        }                 
    	groupSpinner = (Spinner) findViewById(R.id.groupSpinner);
    	groupSpinnerAdapter = new ArrayAdapter<String>(this,
        		android.R.layout.simple_spinner_item, groupNameList);
    	groupSpinnerAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        groupSpinner.setAdapter(groupSpinnerAdapter);
        	
        addItemsOnGroupSpinner();
        addListenerOnGroupSpinnerItemSelection();    	     
    
    }    

    // add items into spinner dynamically
    public void addItemsOnDeviceSpinner() {  
    	//add devices
        List<ZigbeeDevice> devList = ZigbeeAssistant.getSwitchable();
        deviceSpinnerAdapter.clear();
        for (int i = 0 ; i < devList.size() ; i++)
        {             	
        	deviceSpinnerAdapter.add(devList.get(i).Name);
        }     
    }
   
    public void addListenerOnDeviceSpinnerItemSelection() {    	
    	deviceSpinner.setOnItemSelectedListener(new OnItemSelectedListener() {
    	    @Override
    	    public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
    	    	List<ZigbeeDevice> devList = ZigbeeAssistant.getSwitchable();        	    	
    	    	currentDeviceSpinnerSelection = position;
    	    	currDevice = devList.get(currentDeviceSpinnerSelection);
    	    }
      	 
      	  @Override
      	  public void onNothingSelected(AdapterView<?> arg0) {
      	  }
    	});
    }
    
    // add items into spinner dynamically
    public void addItemsOnGroupSpinner() {          
        List<ZigbeeGroup> groupList = ZigbeeAssistant.getGroups();
        groupSpinnerAdapter.clear();
        for (int i = 0 ; i < groupList.size() ; i++)
        {        	
        	groupSpinnerAdapter.add(groupList.get(i).getGroupName());
        }
    }
   
    public void addListenerOnGroupSpinnerItemSelection() {
    	groupSpinner.setOnItemSelectedListener(new OnItemSelectedListener() {
    	    @Override
    	    public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
    	    	currGroup = position;
      	  }
      	 
      	  @Override
      	  public void onNothingSelected(AdapterView<?> arg0) {
      		// TODO Auto-generated method stub
      		currGroup = -1;  
      	  }
    	});
    }

    public void deviceChangeNameButton(View view)
    { 
    	String Title = "Set Name";
    	String Msg = "Please Enter the name of the device";
     	final EditText t = new EditText(this);

    	new AlertDialog.Builder(this)
		.setTitle(Title)
		.setMessage(Msg)
		.setView(t)
		.setNegativeButton("CANCEL", null)
		.setPositiveButton("OK",             
		new DialogInterface.OnClickListener()
		{			
			public void onClick(DialogInterface dialoginterface,int i){	
		        ZigbeeAssistant.setDeviceName( currDevice.Name, t.getText().toString());				        
		        addItemsOnDeviceSpinner();    	
  			}		
		})
		.show();     	    		
    }    	
    
    public void newGroupCmdButton(View view) {

    	//de-select button
    	RadioButton newGroupRadio = (RadioButton) findViewById(R.id.newGroupRadio);
    	newGroupRadio.setChecked(false);
    		
    	final EditText t = new EditText(this);

    	new AlertDialog.Builder(this)
		.setTitle("Create Group")
		.setMessage("Enter the new groups name")
		.setView(t)
		.setPositiveButton("OK",          
		new DialogInterface.OnClickListener()
		{			
			public void onClick(DialogInterface dialoginterface,int i){				    				    	
					ZigbeeAssistant.newGroup(t.getText().toString() );							
			    	//set selection to the new group, which will be last as this is a new one						
			    	currGroup = groupSpinnerAdapter.getCount();				    	
			    	new waitRspTask().execute("Create Group");				    	
			    	groupSpinner.setSelection(currGroup);
			    	//update the spinner
			    	addItemsOnGroupSpinner();					    					    
			}		
		})	    	    		
		.setNegativeButton("Cancel", null)
		.show();	    	
	}
         


    class waitRspTask extends AsyncTask<String , Integer, Void>
    {
    	private boolean rspSuccess;
    	String param;
        @Override
        protected void onPreExecute()
        {
            bar = new ProgressDialog(groupSelect.this);
            bar.setMessage("Processing..");
            bar.setIndeterminate(true);
            bar.show();
        } 
        @Override
        protected Void doInBackground(String... params) 
        {
        	param = params[0];
        	List<ZigbeeGroup> groupList = ZigbeeAssistant.getGroups();
        	//currGroup = 0;
            for(int i=0;i<10;i++)
            {
            	if(currGroup < groupList.size())
            	{
	            	//check if group response updated the groupId          
	            	if (groupList.get(currGroup).getStatus() == ZigbeeGroup.groupStatusActive)
	            	{
	            		rspSuccess = true;
	            		return null;
	            	}
            	}
	            	
	            try
	            {
	                Thread.sleep(500);
	            }
	            catch(Exception e)
	            {
	                System.out.println(e);
	            }    
            	            
            }
            					
            rspSuccess = false;    
            return null;
        }
        @Override
        protected void onPostExecute(Void result) 
        {
            bar.dismiss();
            
            if (rspSuccess == false)
        	{
    	    	AlertDialog show = new AlertDialog.Builder(groupSelect.this)
    			.setTitle(param)
    			.setMessage("No response from gateway. " + param + " failed.")
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
        

    public void addToGroupCmdButton(View view) {

    	//de-select button
    	RadioButton addToGroupRadio = (RadioButton) findViewById(R.id.addToGroupRadio);
    	addToGroupRadio.setChecked(false);
    	
    	if((currGroup != -1) && (currDevice != null))
    	{
	    	AlertDialog show = new AlertDialog.Builder(groupSelect.this)
			.setTitle("Add Light to group")
			.setMessage("Add Light Selected Light to Group " + currGroup)
			.setPositiveButton("OK",             
			new DialogInterface.OnClickListener()
			{			
				public void onClick(DialogInterface dialoginterface,int i){				    				    	
			    	ZigbeeAssistant.addGroup(currDevice, groupSpinnerAdapter.getItem(currGroup)); 
				}		
			})	    	    		
			.setNegativeButton("Cancel", null)
			.show();	    	    		
    	}
    	else
    	{
	    	AlertDialog show = new AlertDialog.Builder(groupSelect.this)
			.setTitle("Add Light to group")
			.setMessage("Select device and group")
			.setPositiveButton("OK",             
			new DialogInterface.OnClickListener()
			{			
				public void onClick(DialogInterface dialoginterface,int i){				    	
				}		
			})	    	    		
			.show();	    	    		
    	}    		

    }
    
    public void IdLightCmdButton(View view) {
    	ToggleButton idLightButton = (ToggleButton) findViewById(R.id.IdLightToggle);
    	if (idLightButton.isChecked())
    	{
    		//ZigbeeAssistant.Identify(currDevice, (short) 0x80);
    	}
    	else
    	{
    		//ZigbeeAssistant.Identify(currDevice, (short) 0);    		
    	}    
    }
    
    public void IdGroupCmdButton(View view) {
    	ToggleButton idGroupButton = (ToggleButton) findViewById(R.id.IdGroupToggle);
    	List<ZigbeeGroup> groupList = ZigbeeAssistant.getGroups();
    	ZigbeeGroup group = groupList.get(currGroup);
    			
    	if (idGroupButton.isChecked())
    	{
    		//ZigbeeAssistant.Identify(group, (short) 0xFFFF);
    	}
    	else
    	{
    		//ZigbeeAssistant.Identify(group, (short) 0);    		
    	}     	
    }
    	
}

